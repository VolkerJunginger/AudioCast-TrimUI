LINK4BRICK v1.0.0 is the stable StockUI release for TrimUI Brick Hammer.

It brings together the device-confirmed FMS GBA audio, live Link tempo following and START queued to the next four-beat “one,” with the final icons and minimal settings page. The working audio and clock implementation is preserved.

- Link Audio to Push as **Brick Out**, alongside normal speaker audio.
- Clock-only mode by switching Link audio OFF.
- FMS GBA at 24 PPQ, FMS Clock at 1/2/3/4/6/8 PPQ, and STEPPER at 4/6/12/24/48/96 PPQ.
- Fixed **65 ms** audio buffer; no delay-compensation or buffer controls.
- One **LINK4BRICK** StockUI app, with no runtime log files or separate FMS launcher.
- Verified Terminal installer with settings preservation, diagnostic-helper cleanup, backups and undo.

Download **LINK4BRICK-StockUI-v1.0.0.zip** and **install_link4brick.py** into the same folder, then follow the [installation guide](https://github.com/VolkerJunginger/LINK4BRICK/blob/main/INSTALL.txt). The source archive includes the pinned Link and mGBA sources and generated menu assets. **SHA256SUMS** verifies all three downloads.

FMS GBA has been confirmed on the user's Brick Hammer and Push. STEPPER's sync path is covered by emulator tests; individual ROM versions need hardware confirmation. Game Boy sync, other firmware and general multiplayer Game Link are outside this release. Compensate transport latency on Push if needed.

SD-card-only and reversible. No ROMs, firmware, RetroArch binary, minarch or global ALSA configuration changes. GPL-2.0-or-later with the private mGBA integration under MPL-2.0.
