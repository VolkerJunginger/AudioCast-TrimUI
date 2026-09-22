# Troubleshooting

## Brick Out does not appear

1. Quit the game, reboot the Brick, and reconnect it to the same local network as Push.
2. Check that AudioCast shows the turquoise ON icon. Launch the app once if it is OFF.
3. Start a `.gb` or `.gba` game from its normal StockUI entry. AudioCast does not advertise a game channel while sitting in the menu.
4. Refresh/reselect **Brick Out** on Push. Each game session creates a new channel.
5. If the app will not turn ON after an upgrade, check for missing activation records as described below.

The ON icon indicates activation, not an established network connection. Other emulators and menu sounds are not supported by this release.

## The icon does not refresh

Leave and reopen Apps. If needed, reboot. Icon switching was confirmed on the tested Hammer, but StockUI may retain cached artwork.

## Incomplete upgrade or missing activation records

If `Apps/AudioCast/enabled` and `launchers.list` are missing, but GB/GBA launchers still have adjacent `.audiocast-original` and `.audiocast-run` files, the app folder may have been replaced while casting was enabled. The launcher wrappers fall back to the originals, while activation refuses to overwrite existing backups.

**Do not delete the backups to force activation.** They are the original launchers you need for recovery.

1. Shut down the Brick and back up `Apps/AudioCast`, `Emus/GB` and `Emus/GBA` to your computer.
2. For each affected launcher, inspect the current file. An AudioCast wrapper starts with `# AudioCast v0.2b managed wrapper` and calls AudioCast's `run.sh` or the adjacent `.audiocast-original`.
3. Restore only confirmed AudioCast wrappers from their **matching** `.audiocast-original` files. If an original is missing, edited or uncertain, stop and seek help. Never substitute another emulator's launcher.
4. Verify each restored file is byte-for-byte identical to its backup. Only then remove that launcher's `.audiocast-original` and `.audiocast-run` companions from the SD card. Retain the computer backup.
5. Install the latest app with casting OFF, reboot, then enable it once and start a game.

On macOS, copying Mac-specific file flags to an SD filesystem can fail with “Invalid argument.” Use a file-content-only copy when restoring; Mac metadata is unnecessary for these launchers. Do not run the Brick's app scripts on your Mac.

The development repair was specific to one verified card and is deliberately not shipped as a universal recovery script.

## Turn off and uninstall

Quit the game and launch AudioCast again. OFF restores known original launcher bytes. Only after successful restoration should you delete `Apps/AudioCast`.

An OFF icon means the activation marker is absent. If restoration encountered an edited launcher or damaged backup, casting is disabled but those recovery files are retained. Preserve them and inspect before uninstalling.

## Audio drops out

Check Wi-Fi signal and local network connectivity. Start with the direct Push Wi-Fi setup used in hardware testing. AudioCast preserves the speaker path if the network sender stalls, but this cannot guarantee uninterrupted wireless playback.

## Reporting a problem

Include the device and firmware, AudioCast version, GB or GBA core, whether the Brick speaker works, icon state, and whether **Brick Out** appears on Push. Mention whether this followed an upgrade.

Current releases do not write logs. Please do not upload ROMs, saves, Wi-Fi credentials or full SD-card backups. Older test logs may contain game paths; review them before sharing.
