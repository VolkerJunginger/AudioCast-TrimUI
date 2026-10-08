<p align="center"><img src="docs/images/icon-on.png" width="160" alt="LINK4BRICK"></p>

# LINK4BRICK

**Ableton Link audio and clock sync for the TrimUI Brick Hammer.**

Play FMS or STEPPER from the normal StockUI GBA game list. Hear audio on the Brick and stream it to Push as **Brick Out**, or switch Link audio off and use clock sync alone.

[Download the stable release](https://github.com/VolkerJunginger/LINK4BRICK/releases/latest) · [Install](INSTALL.txt) · [Sync modes](docs/VIRTUAL_LINK_CABLE.md) · [Troubleshooting](docs/TROUBLESHOOTING.md)

[![Build and release](https://github.com/VolkerJunginger/LINK4BRICK/actions/workflows/release.yml/badge.svg)](https://github.com/VolkerJunginger/LINK4BRICK/actions/workflows/release.yml)

## What it does

- Keeps the Brick speaker playing while sending 48 kHz stereo audio over Link Audio.
- Follows live Link tempo changes through a virtual GBA cable.
- In **FMS GBA** mode, pressing Brick START queues playback to the next four-beat “one.”
- Offers separate Link audio and sync settings, with matching PPQ choices for FMS Clock and STEPPER.
- Uses the final ON/OFF icons and a minimal settings page. The audio buffer is fixed at **65 ms**.
- Installs on the SD card, with reversible launcher routing and no runtime log files.

**Tested setup:** TrimUI Brick Hammer, StockUI, FMS GBA and Ableton Push. The user confirmed stable audio, tempo following and queued starts in FMS GBA. STEPPER has its own tested emulator sync implementation; performance with individual ROM versions still needs hardware confirmation. Other firmware and Game Boy sync are not supported by this release. This is a music sync adapter, not a general multiplayer Game Link emulator.

## Install from Terminal

Download **LINK4BRICK-StockUI-v1.0.0.zip** and **install_link4brick.py** from the [release page](https://github.com/VolkerJunginger/LINK4BRICK/releases/latest), placing them in the same folder. Close the game, shut down the Brick and connect its SD card to your computer. With Python 3 installed, run:

```sh
python3 ~/Downloads/install_link4brick.py --card /Volumes/128GBRICK
```

Use your actual card path if it differs. The installer verifies the package, keeps existing settings and launcher backups, and saves an undo journal on your computer. Upgrading LINK4BRICK preserves its enabled state. If upgrading from the old **AudioCast** app, first turn that app OFF on the Brick; it must restore its launchers before installing LINK4BRICK.

Eject the card, reboot the Brick and open **Apps → LINK4BRICK**. Enable it in settings, then open your ROM from **Games → GBA** as usual. You do not need a separate FMS launcher. On Push, enable Link and choose **Brick Out** after opening the game. Both devices must share a network that permits Link discovery.

## Settings

| Setting | Choices |
| --- | --- |
| Enabled | ON wraps compatible StockUI GBA launchers; OFF restores their originals |
| Link audio | ON streams to Push; OFF keeps speaker audio and clock sync |
| Sync mode | Off, FMS GBA, FMS Clock, STEPPER |
| PPQ | FMS GBA: 24; FMS Clock: 1, 2, 3, 4, 6, 8; STEPPER: 4, 6, 12, 24, 48, 96 |

Start with **FMS GBA / 24 PPQ** and set FMS to **SYNC IN / GBA**. Press START on the Brick to queue the next “one.” Match the program's sync input and PPQ when using FMS Clock or STEPPER. Change settings with the game closed.

The “one” is a four-beat Link phase boundary. Audio transport adds latency; compensate for incoming audio on Push as appropriate. LINK4BRICK does not apply extra delay compensation. The icon reflects whether routing is enabled, rather than whether a peer is connected.

## Reversible by design

Turn **Enabled OFF** before removing the app or editing emulator launch scripts. Original launchers and checksum records stay on the card until restoration succeeds. Normal ROMs, battery saves, emulator binaries, firmware and global ALSA configuration are unchanged. A private patched mGBA core is used only for supported sync modes; its manual save states have a separate folder.

The Terminal installer reports a local `installation.json` path. To undo that update:

```sh
python3 ~/Downloads/install_link4brick.py --card /Volumes/128GBRICK --undo /path/to/installation.json
```

Undo checks both installed files and backups before restoring them. If a managed file has subsequently changed, it stops and preserves the edit. Keep your backup journals. Old diagnostic helpers and the dedicated BrickTools cable checker are backed up and removed during this update; existing test logs are left for you to keep or delete.

## Build and license

The [build guide](docs/DEVELOPMENT.md) and automated checks cover routing, recovery, settings, live Link tempo, GBA serial sync, STEPPER interrupts, fixed-rate audio, ARM64 execution and the reversible installer. Each release includes the installable **StockUI ZIP**, corresponding source, installer and SHA-256 checksums. No ROMs or `.pak` files are distributed.

Project code is **GPL-2.0-or-later**. The private mGBA integration is **MPL-2.0**; see [third-party notices](THIRD_PARTY.md). LINK4BRICK is independent of Ableton and TrimUI.
