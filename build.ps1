# StreamDeck Discord Volume Mixer - Clean Release Build & Deploy Script

param(
    [switch]$SkipDeploy,
    [switch]$KeepBuildArtifacts
)

$ErrorActionPreference = "Stop"
$projectRoot = $PSScriptRoot
$buildDir = Join-Path $projectRoot "build"
$binDir = Join-Path $projectRoot "bin"
$releaseDir = Join-Path $projectRoot "release"
$manifestPath = Join-Path $projectRoot "src\plugin\manifest.json"

if (!(Test-Path -LiteralPath $manifestPath)) {
    throw "Plugin manifest not found: $manifestPath"
}

$manifestVersion = (Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json).Version
$releaseVersion = $manifestVersion -replace '\.0$', ''

function Assert-NativeCommandSucceeded([string]$stepName) {
    if ($LASTEXITCODE -ne 0) {
        throw "$stepName failed with exit code $LASTEXITCODE."
    }
}

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Building StreamDeck Discord Volume Mixer " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Clean release: v$releaseVersion" -ForegroundColor Cyan

# Always start clean. release/ contains generated output only, so replacing the
# directory cannot remove any source assets.
Write-Host "`n[1/5] Removing previous generated files..." -ForegroundColor Yellow
foreach ($generatedDir in @($buildDir, $binDir, $releaseDir)) {
    if (Test-Path -LiteralPath $generatedDir) {
        Remove-Item -LiteralPath $generatedDir -Recurse -Force
    }
}
New-Item -ItemType Directory -Path $releaseDir -Force | Out-Null

# 1. Setup Toolchain Paths
$qtRoot = "C:\Users\Thomas\Documents\Codex\2026-08-25\c-users-thomas-downloads-streamdeck-discordvolumemixer2\work\qt\6.8.3\mingw_64"
$qtBin = "C:\Users\Thomas\Documents\Codex\2026-08-25\c-users-thomas-downloads-streamdeck-discordvolumemixer2\work\qt\Tools\mingw1310_64\bin"
$cmakeBin = "C:\Users\Thomas\Documents\Codex\2026-08-25\c-users-thomas-downloads-streamdeck-discordvolumemixer2\work\qt\Tools\CMake_64\bin"

if (Test-Path $qtBin) { $env:PATH = "$qtBin;$cmakeBin;" + $env:PATH }

# 2. Configure a fresh Release build
Write-Host "`n[2/5] Configuring CMake Release..." -ForegroundColor Yellow
cmake -S $projectRoot -B $buildDir -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$qtRoot"
Assert-NativeCommandSucceeded "CMake configuration"

# 3. Compile
Write-Host "`n[3/5] Compiling C++ binary..." -ForegroundColor Yellow
cmake --build $buildDir --config Release -j8
Assert-NativeCommandSucceeded "Compilation"

# Sign the executable before packaging. A production release should set
# DVM_CODE_SIGNING_CERT_THUMBPRINT to a publicly trusted/WDAC-approved signer.
$defaultDevelopmentThumbprint = "EF02B8D0092AFBE08C4307158F60FAA78499028F"
$signingThumbprint = if ($env:DVM_CODE_SIGNING_CERT_THUMBPRINT) {
    $env:DVM_CODE_SIGNING_CERT_THUMBPRINT
} else {
    $defaultDevelopmentThumbprint
}
$signingCertificatePath = "Cert:\CurrentUser\My\$signingThumbprint"
$builtExecutable = "$PSScriptRoot\bin\Release\com.thomast.discordmixer.sdPlugin\bin\streamdeck-discordmixer.exe"

if ((Test-Path -LiteralPath $signingCertificatePath) -and (Test-Path -LiteralPath $builtExecutable)) {
    Write-Host "Signing plugin executable..." -ForegroundColor Yellow
    $signingCertificate = Get-Item -LiteralPath $signingCertificatePath
    if (!$signingCertificate.HasPrivateKey) {
        throw "Code-signing certificate has no accessible private key: $signingThumbprint"
    }

    $signature = Set-AuthenticodeSignature -LiteralPath $builtExecutable `
        -Certificate $signingCertificate -HashAlgorithm SHA256 -IncludeChain All
    if (!$signature.SignerCertificate -or $signature.SignerCertificate.Thumbprint -ne $signingThumbprint) {
        throw "Failed to apply the expected Authenticode signature."
    }

    Write-Host "Signed with certificate: $signingThumbprint ($($signature.Status))" -ForegroundColor Cyan
} else {
    Write-Host "[WARNING] Code-signing certificate not found; package will be unsigned." -ForegroundColor Yellow
}

# 4. Install & Deploy Qt runtime to bin/Release bundle
Write-Host "`n[4/5] Assembling plugin bundle and Qt runtime..." -ForegroundColor Yellow
cmake --install $buildDir --config Release
Assert-NativeCommandSucceeded "Bundle installation"

# 5. Create GitHub Release Package (.streamDeckPlugin & .zip) in release/
$sourceBundle = "$PSScriptRoot\bin\Release\com.thomast.discordmixer.sdPlugin"
$streamDeckPluginFile = "$releaseDir\com.thomast.discordmixer.streamDeckPlugin"
$zipReleaseFile = "$releaseDir\StreamDeck-DiscordVolumeMixer-v$releaseVersion-Windows-x64.zip"

Write-Host "`n[5/5] Creating official Elgato release package..." -ForegroundColor Yellow
npx -y @elgato/cli pack "$sourceBundle" --output "$releaseDir" -f
Assert-NativeCommandSucceeded "Elgato packaging"
Copy-Item "$streamDeckPluginFile" -Destination "$zipReleaseFile" -Force
Write-Host "Created: release\com.thomast.discordmixer.streamDeckPlugin" -ForegroundColor Cyan
Write-Host "Created: release\StreamDeck-DiscordVolumeMixer-v$releaseVersion-Windows-x64.zip" -ForegroundColor Cyan

# 6. Copy plugin bundle to Stream Deck AppData (for local testing)
$pluginsDir = "$env:APPDATA\Elgato\StreamDeck\Plugins"
$pluginDestination = Join-Path $pluginsDir "com.thomast.discordmixer.sdPlugin"
$streamDeckExecutable = Join-Path $env:ProgramFiles "Elgato\StreamDeck\StreamDeck.exe"

if (!$SkipDeploy -and (Test-Path $sourceBundle)) {
    Write-Host "`nDeploying to Stream Deck AppData..." -ForegroundColor Green
    $streamDeckWasRunning = [bool](Get-Process "StreamDeck" -ErrorAction SilentlyContinue)
    try {
        if ($streamDeckWasRunning) {
            Write-Host "Temporarily stopping Stream Deck to release plugin files..." -ForegroundColor Yellow
            Stop-Process -Name "StreamDeck" -Force -ErrorAction Stop
        }

        Stop-Process -Name "streamdeck-discordmixer" -Force -ErrorAction SilentlyContinue
        Start-Sleep -Milliseconds 750

        if (Test-Path -LiteralPath $pluginDestination) {
            Remove-Item -LiteralPath $pluginDestination -Recurse -Force
        }
        Copy-Item -LiteralPath $sourceBundle -Destination $pluginDestination -Recurse -Force

        Write-Host "`n=========================================" -ForegroundColor Green
        Write-Host " BUILD, RELEASE & DEPLOY COMPLETE!       " -ForegroundColor Green
        Write-Host "=========================================" -ForegroundColor Green
    }
    catch {
        Write-Host "`n[ERROR] Live plugin deployment failed: $($_.Exception.Message)" -ForegroundColor Red
        throw
    }
    finally {
        if ($streamDeckWasRunning -and (Test-Path -LiteralPath $streamDeckExecutable)) {
            Write-Host "Restarting Stream Deck..." -ForegroundColor Yellow
            Start-Process -FilePath $streamDeckExecutable
            Start-Sleep -Seconds 2
            if (!(Get-Process "StreamDeck" -ErrorAction SilentlyContinue)) {
                throw "Stream Deck did not restart after deployment."
            }
        }
    }
}

if (!$KeepBuildArtifacts) {
    Write-Host "`nRemoving temporary build and bin folders..." -ForegroundColor Yellow
    Remove-Item -LiteralPath $buildDir, $binDir -Recurse -Force
}

Write-Host "`nRelease build finished successfully." -ForegroundColor Green
