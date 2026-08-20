$ErrorActionPreference = "Stop"

$pluginSource = Join-Path $env:APPDATA "Elgato\StreamDeck\Plugins\cz.danol.discordmixer.sdPlugin"
$stageRoot = Join-Path $env:TEMP "dvm-release-stage"
$stagePlugin = Join-Path $stageRoot "cz.danol.discordmixer.sdPlugin"
$zipPath = Join-Path $env:TEMP "DiscordVolumeMixer2-Revived-v2.0.2.zip"
$assetPath = Join-Path $env:TEMP "DiscordVolumeMixer2-Revived-v2.0.2.streamDeckPlugin"

Remove-Item -LiteralPath $stageRoot -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $stageRoot | Out-Null
Copy-Item -LiteralPath $pluginSource -Destination $stageRoot -Recurse -Force

$sensitiveFiles = @(
    "clog.txt",
    "log.txt",
    "discordOauth.json",
    "discordOauth.json.bak",
    "bin\clog.txt",
    "bin\log.txt",
    "bin\discordOauth.json"
)

foreach($file in $sensitiveFiles) {
    Remove-Item -LiteralPath (Join-Path $stagePlugin $file) -Force -ErrorAction SilentlyContinue
}

Remove-Item -LiteralPath $zipPath, $assetPath -Force -ErrorAction SilentlyContinue
Compress-Archive -Path $stagePlugin -DestinationPath $zipPath -Force
Rename-Item -LiteralPath $zipPath -NewName (Split-Path $assetPath -Leaf)

$leftovers = Get-ChildItem $stagePlugin -Recurse -Include "*Oauth*", "*log*" -ErrorAction SilentlyContinue
if($leftovers) {
    Write-Error "Release package still contains sensitive/log files: $($leftovers.FullName -join ', ')"
}

Get-Item $assetPath
