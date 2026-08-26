# Changelog

Notable changes to Stream Deck Discord Volume Mixer. Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/);
versions follow `manifest.json`.

## [2.0.3.0] — 2026-08-25

### Fixed
- **ERR 8 Handshake Failures**: Added multi-pipe retry logic across all Discord IPC endpoints (`discord-ipc-0` through `discord-ipc-9`). If an orphaned/stale pipe rejects the handshake, the plugin automatically tries the next pipe instead of failing.
- **Handshake Timeouts**: Increased default handshake timeout from 10s to 20s to reliably support cold starts and high system load.
- **Repeated Authorization on Restart**: Resolved token persistence failure by using absolute AppData paths and Windows Registry dual-storage (`HKCU\Software\Elgato Stream Deck Plugin\com.thomast.discordmixer\discordOauth`).
- **Submodule Architecture**: Integrated `qtdiscordipc` and `qtstreamdeck2` directly into the repository, eliminating git detached head and submodule synchronization issues.

### Added
- **Human-Readable Error Diagnostics**: Replaced cryptic numerical error codes (`ERR 0`–`ERR 8`, `RPC 4000`–`RPC 4010`) with descriptive status strings (`NO DISCORD`, `BAD CLIENT`, `TOKEN FAIL`, `AUTH DENIED`, `CLOSED`).
- **Modern UI & Discord Colors**: Added vibrant glowing Discord-green (`#23A55A`) speaking rings and red (`#ED4245`) muted badges.
- **Stream Deck + Dial Enhancements**: Antialiased circular avatar clipping and dynamic color rings for rotary encoder LCD screens.
- **Fast-Path Reconnection**: Immediate 5ms local authentication with cached access tokens.

- **3D Apple Squircle Icon Redesign**: Complete visual overhaul of all 25 stream deck icons with modern Apple squircle 3D styling, glass hairline highlights, and specular reflections.
- **Deep Obsidian Black Contrast**: Specially tuned deep pitch-black gradients (`#18191C` ➔ `#0A0B0C`) for maximum contrast and zero backlight bleed on Stream Deck LCD screens.
- **Smart Nickname Padding & Formatting**: Automatic name elision with ellipsis for long usernames (`karaflo…`), preventing text clipping on button borders.
- **Glowing Speaking Deco**: Refined multi-layer green neon aura with soft ambient bloom for active speakers.
- **One-Click Release Packaging**: Automated `build.ps1` producing `.streamDeckPlugin` and standalone zip packages ready for GitHub releases.

### Changed
- Upgraded build architecture to **Qt 6.8+ LTS** and **C++20**.
- Modernized documentation with custom animated SVGs, specification cards, and clean troubleshooting guides.
