<div align="center">
  <img src="docs/images/icon-on.png" width="128" alt="LINK4BRICK logo">
  <h1>LINK4BRICK</h1>
  <p><strong>GBA music, connected to your Link session.</strong></p>
  <p>Stream audio from TrimUI Brick Hammer to Push and follow Ableton Link tempo with FMS or STEPPER.</p>
</div>

LINK4BRICK runs from the SD card on **StockUI**. Open your music program from the normal **Games → GBA** menu. Audio continues through the Brick speaker; **Link audio** can be enabled for streaming to the **Brick Out** channel on Push, or disabled for clock-only use.

This development preview follows the user's successful FMS tempo and queued-start tests. Playback buffering is now fixed at the user-selected 65 ms. The next Brick test checks FMS GBA START alignment with measured delivery timing. The existing published audio-only release remains separate.

## Settings

The grayscale settings menu uses smooth Inter typography, your current logo and a simple selection row.

Use **Up / Down** to select a row, **Left / Right** to adjust, **A** to change and **B** to return. Close the game first. Preferences apply to the next game session.

| Setting | Choices |
|---|---|
| Enabled | On / Off; Off restores managed GBA launchers |
| Link audio | On / Off; Off keeps local sound and Link clock active |
| Sync mode | Off, FMS - GBA, STEPPER, FMS - Clock |
| Pulses per beat | FMS GBA: fixed 24; STEPPER: 4 / 6 / 12 / 24 / 48 / 96; FMS Clock: 1 / 2 / 3 / 4 / 6 / 8 |

RetroArch playback buffering is fixed at **65 ms** for normal and sync launches. The buffer option has been removed; older saved buffer values are ignored. No delay compensation is applied; compensate incoming audio on Push as needed.

GB/DMGo sync is retired from this menu. This preview manages GBA launchers only. FMS and STEPPER use different GameLink protocols; this is not universal multiplayer, trading or MIDI support.

## FMS and STEPPER

- **FMS - GBA:** FMS **SYNC IN → GBA**; fixed 24 PPQ serial clock.
- **FMS - Clock:** FMS **SYNC IN → CLOCK**; match its PPQ to LINK4BRICK.
- **STEPPER:** **LINK IN**; match its BPQ to LINK4BRICK. The incoming rates are 4, 6, 12, 24, 48 and 96. STEPPER's 2 BPQ output mode is not supported as a clock input.

Brick **START queues the next four-beat Link “one”**. Press again to stop or cancel. Subsequent ticks follow live Link tempo changes. Existing sync timing and audio transport code are unchanged by this UI update.

## Installation and migration

Use the supplied **Terminal installer** for an existing installation. It verifies files, preserves launcher originals, settings, icons and private saves, and migrates the app to **`Apps/LINK4BRICK`**. It restores any previously managed GB launchers and retains GBA routing when enabled. Use the same installer with `--undo` to return to the previous folder and files.

Do not extract the new folder beside an enabled old installation: its wrappers still point to the old folder. For a fresh installation, extract the StockUI ZIP at the SD-card root, safely eject and reboot, then open **Apps → LINK4BRICK** and enable it. No `.pak` is used.

The new ON/OFF artwork supplied by the maintainer is packaged unchanged. `icon.png` initially uses the ON artwork; the icon then follows the enabled state. Internal executable names, environment variables, diagnostic names and backup suffixes retain their existing names for compatibility during this **folder-first migration**.

## Safety and development

Only SD-card app files and reversible launchers change. Firmware, `/etc/asound.conf`, RetroArch binaries, ROMs, saves and normal core settings remain intact. Audio configuration, clock sockets and temporary RetroArch overrides live in `/tmp`.

The standard ZIP discards diagnostics. The tailored test installer preserves the existing bounded diagnostic log at **`AudioCast-Link-Sync-test.log`**.

See the [sync guide](docs/VIRTUAL_LINK_CABLE.md), [build instructions](docs/DEVELOPMENT.md), [troubleshooting](docs/TROUBLESHOOTING.md) and [third-party notices](THIRD_PARTY.md). Main code is [GPL-2.0-or-later](LICENSE); mGBA is MPL-2.0 and Inter is SIL OFL 1.1. This is an independent project, not an official Ableton or TrimUI product.
