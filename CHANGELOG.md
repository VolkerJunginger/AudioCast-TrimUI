# Release history

## 0.2.2 — StockUI GB/GBA

- Added ON/OFF artwork following the app's activation marker.
- Verified audio playback and icon switching on the TrimUI Brick Hammer with Push.
- Published the tested implementation in its own AudioCast-TrimUI repository, with installation, recovery and build documentation.
- No new audio-path changes for the repository move.

## 0.2.1

- Shortened the app label to AudioCast.
- Added custom handheld/link artwork.
- Removed AudioCast file logging and disabled RetroArch file logging during casting.

## 0.2

- Stable StockUI release for existing GB/GBA menus.
- Local speaker plus 48 kHz stereo Link Audio output.
- Bundled checksum verification and reversible launcher restoration.

## Development milestones

- **0.2b.1:** bundled a checksum helper for firmware without `cksum`; verified restore and error handling.
- **0.2b:** GB/GBA launcher integration, supervised FIFO relay and game-session cleanup.
- **0.2a:** live ALSA speaker/FIFO duplication test; successful playback on Brick and Push.
- **0.1:** earlier NextUI experiment. It used a different integration and is not part of the StockUI installer.

The original development work remains in Testing-Github. This repository contains the supported StockUI implementation, not firmware images, ROMs, user logs or SD-card snapshots.
