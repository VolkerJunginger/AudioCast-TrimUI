## AudioCast v0.2.2 — StockUI for TrimUI Brick Hammer

**Game Boy audio on your Brick and Push, together.**

This is the first release from AudioCast's dedicated repository. It contains the tested StockUI GB/GBA implementation with updated documentation and licensing; the game-audio code is unchanged.

### Included

- GB `.gb` and GBA `.gba` casting from the normal StockUI menus.
- 48 kHz stereo Link Audio channel **Brick Out**, with local speaker playback.
- **ON:** turquoise link and filled dot. **OFF:** gray link and hollow dot.
- SD-card-only installation, verified launcher restoration and no runtime log files.
- Installation/recovery guide, dependency notices and corresponding source bundle.

### Download

Choose **AudioCast-StockUI-v0.2.2-GB-GBA.zip** for installation. `SHA256SUMS` verifies the installer and source bundle. The source bundle is for developers; it is not an SD-card installer.

### Install or upgrade

1. Quit your game and switch AudioCast **OFF before upgrading**.
2. Extract the installer at the SD-card root, merging `Apps/AudioCast`.
3. Safely eject the card, reboot and connect the Brick and Push to the same local Wi-Fi network.
4. Launch **AudioCast** once to enable it, then start a GB/GBA game.
5. Select **Brick Out** on Push. Reselect after changing games if needed.

See the [README](https://github.com/VolkerJunginger/AudioCast-TrimUI#installation) and [recovery guide](https://github.com/VolkerJunginger/AudioCast-TrimUI/blob/main/docs/TROUBLESHOOTING.md) for details. Preserve launcher backups if an upgrade was performed while AudioCast was ON.

### Verified

GB/GBA playback, speaker output and icon switching were confirmed on the maintainer's Brick Hammer with Push. CI checks ALSA/FIFO routing, sender stalls, cleanup, checksums, reversible launchers, no-log behavior, state icons and the ARM64 ZIP contents.

The icon indicates activation, not receiver connectivity. Other emulators and menu sounds are outside this release's scope. Automatic RetroArch core/game configuration overrides are temporarily disabled during casting; their files remain unchanged.
