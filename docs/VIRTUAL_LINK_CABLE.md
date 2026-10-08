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

**Audio buffer** requests RetroArch ALSA playback latency from 0 to 150 ms in steps of 10. The default is 60 ms, the closest offered value to the previous 64 ms override. It applies with clock on or off and with Link audio on or off. The private sync override uses the same saved value. Fallback to the installed core restores the base temporary configuration, including that value.

Zero is a request for the driver minimum, not a guarantee of zero latency. Hardware may round the value. The setting does not resize Link network packets or directly control Push's receiver buffer. Speaker output is retained. Changes take effect at the next game launch.

## Migration

The Terminal installer migrates `Apps/AudioCast` to `Apps/LINK4BRICK` using verified local backups. It updates launcher folder references, carries private states without altering their contents, and restores managed GB launchers. The original launcher backup suffixes, binary names and environment variable names are retained. The menu contains only the supported GBA modes. If the old preference was DMGo or generic GPIO, the installer selects FMS GBA (24 PPQ); Off and valid FMS/STEPPER choices are retained.

Use `--undo` from the same installer directory to restore the old app folder and launcher files. Changed menu preferences are archived before undo. Exit games before changing settings. Safely eject and reboot after installing or undoing.

## Validation and limits

- Real menu actions, all 16 buffer choices, reverse adjustment, PPQ choices and settings persistence are checked.
- Launcher tests check the new folder paths, mGBA selection, rejection of unsupported modes, fallback, exact buffer override values and preservation of spaced ROM arguments.
- Existing FMS serial timing and generated STEPPER/FMS-clock pulse tests are retained, including tempo changes and queued starts.
- ARM64 builds and private-core lifecycle tests run in CI; packages are checked for ZIP integrity, executable architecture, licenses, assets and absence of ROMs/saves/logs/`.pak`.
- The next audible Brick/Push test must validate the new menu and buffer values. Previous FMS sync was confirmed by the user; automated tests are not proof of Wi-Fi or speaker performance at every buffer setting.

Normal packages discard output. The tailored diagnostic installer preserves `AudioCast-Link-Sync-test.log` and its bounded output capture. The logger always drains frontend output even if it cannot write the SD log, keeping logging outside audio flow control.
