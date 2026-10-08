# LINK4BRICK: GBA audio and clock sync

Open **LINK4BRICK**, enable it, select FMS or STEPPER sync, then launch the ROM normally from **Games → GBA**. The private mGBA core supplies the matching virtual GameLink connection while the ALSA tee preserves Brick speaker audio and optionally sends PCM to Push.

## Modes and rates

| Menu mode | Program setting | PPQ |
|---|---|---|
| Off | Internal clock | No external clock |
| FMS - GBA | SYNC IN / GBA | Fixed 24 |
| FMS - Clock | SYNC IN / CLOCK | 1, 2, 3, 4, 6, 8 |
| STEPPER | LINK IN (matching BPQ) | 4, 6, 12, 24, 48, 96 |

FMS GBA uses its documented serial START/TICK/STOP bytes. FMS Clock supplies polled SC pulses. STEPPER supplies SI falling edges and the GBA serial IRQ. Match the program's rate to LINK4BRICK. Neither mode depends on a ROM filename or writes game RAM. No ROM or BIOS is included.

START queues the next four-beat Link boundary. A second START cancels or stops. STEPPER SELECT+START retains its native save-bank action. Native program pause/reset behavior remains the program's responsibility. Peer loss or invalid clock input idles the cable; press START again after reconnection.

GB/DMGo and generic GPIO presets are no longer exposed or enabled. Upstream mGBA GB capability and historical regression tests remain in source; existing internal names are retained during the user-requested folder-first migration.

## Audio buffer

RetroArch ALSA playback buffering is fixed at **65 ms**, with clock on or off and Link audio on or off. The menu has no buffer control; obsolete saved buffer values are ignored. The private sync override and fallback retain 65 ms. This does not change Link network packets or Push's receiver buffer.

FMS GBA retains the previously tested clock scheduler and four-beat queued start. Diagnostic builds now record `AUDIOCAST_START_TIMING`: target beat, actual host callback beat and signed delivery error in microseconds. This measures virtual serial START delivery, not when the first note becomes audible. Hardware results are needed to distinguish clock alignment from playback buffering.

## Migration

The Terminal installer migrates `Apps/AudioCast` to `Apps/LINK4BRICK` using verified local backups. It updates launcher folder references, carries private states without altering their contents, and restores managed GB launchers. The original launcher backup suffixes, binary names and environment variable names are retained. The menu contains only the supported GBA modes. If the old preference was DMGo or generic GPIO, the installer selects FMS GBA (24 PPQ); Off and valid FMS/STEPPER choices are retained.

Use `--undo` from the same installer directory to restore the old app folder and launcher files. Changed menu preferences are archived before undo. Exit games before changing settings. Safely eject and reboot after installing or undoing.

## Validation and limits

- Real menu actions, removed buffer controls, reverse adjustment, PPQ choices and settings persistence are checked.
- Launcher tests check the new folder paths, mGBA selection, rejection of unsupported modes, fallback, fixed 65 ms overrides and rejection of obsolete buffer values and preservation of spaced ROM arguments.
- Existing FMS serial timing and generated STEPPER/FMS-clock pulse tests are retained, including tempo changes and queued starts.
- ARM64 builds and private-core lifecycle tests run in CI; packages are checked for ZIP integrity, executable architecture, licenses, assets and absence of ROMs/saves/logs/`.pak`.
- The next audible Brick/Push test must check queued START alignment. Previous FMS sync was confirmed by the user; automated tests do not prove audible bar alignment.

Normal packages discard output. The tailored diagnostic installer preserves `AudioCast-Link-Sync-test.log` and its bounded output capture. The logger always drains frontend output even if it cannot write the SD log, keeping logging outside audio flow control.
