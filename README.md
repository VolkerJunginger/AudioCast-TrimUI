<div align="center">
  <img src="docs/images/icon-on.png" width="144" alt="AudioCast: a handheld connected to an audio link">
  <h1>LINK4BRICK</h1>
  <p><strong>Your Game Boy audio. On the Brick and on Push.</strong></p>
  <p>Stream GB and GBA game audio from a TrimUI Brick Hammer running StockUI to Ableton Link Audio over Wi-Fi, while keeping the Brick speaker playing.</p>
  <p>
    <a href="https://github.com/VolkerJunginger/AudioCast-TrimUI/releases/latest">Download the latest release</a> ·
    <a href="#installation">Install</a> ·
    <a href="docs/TROUBLESHOOTING.md">Troubleshooting</a>
  </p>
  <p><img src="https://github.com/VolkerJunginger/AudioCast-TrimUI/actions/workflows/release.yml/badge.svg" alt="Build and verification status"></p>
</div>

## What it does

Experimental clock-sync development now targets a [virtual link cable through
normal game launches](docs/VIRTUAL_LINK_CABLE.md), governed by AudioCast ON/OFF.
The dedicated FMS launch/test apps failed on the Hammer and are superseded.
The stable release remains audio casting only; FMS tempo and queued downbeat starts have been confirmed on one Brick/Push setup.
This branch adds settings and a DMGo Game Boy adapter, awaiting device validation.

- Casts **Game Boy `.gb` and Game Boy Advance `.gba`** audio through the existing StockUI game menus.
- Sends **48 kHz stereo audio** to the Link Audio channel **Brick Out**.
- Keeps normal Brick speaker output.
- Opens an on-device settings menu for enable/disable, Link audio and cable protocol.
- Can follow Link clock without advertising an audio channel; the Brick speaker keeps playing.
- Preserves the matching ON/OFF status icon.
- Runs from the **SD card**, with temporary audio configuration in `/tmp`.
- Creates **no AudioCast log files**.
- Restores the original GB/GBA launchers when switched OFF.

**Tested on a TrimUI Brick Hammer with StockUI and Push 3.** Game audio, restoration and the ON/OFF icon changes have been confirmed on the device. Other firmware, emulators and receiver combinations have not been validated.

## Installation

1. Download **`AudioCast-StockUI-v0.2.2-GB-GBA.zip`** from [Releases](https://github.com/VolkerJunginger/AudioCast-TrimUI/releases/latest). Choose the installer ZIP, not GitHub's source-code archive.
2. If upgrading, **quit your game and switch AudioCast OFF first**. Keep a backup of your SD card.
3. Extract the ZIP at the **SD-card root**, merging the `Apps` directory. The app should end up at `Apps/AudioCast/launch.sh`.
4. Safely eject the card and reboot the Brick.
5. Connect the Brick and Push to the same local Wi-Fi network. The tested setup uses the Push Wi-Fi network.
6. In this experimental settings build, open **Apps → LINK4BRICK**, set **ENABLED: ON**, and press **B** to return. The published v0.2.2 still toggles directly.
7. Start a GB or GBA game normally. On Push, select **Brick Out** from the Link Audio sources. The peer is **TrimUI Brick Hammer**.

The channel exists during a game session and is recreated for each game. You may need to select it again after changing games.

## ON and OFF

| ON | OFF |
|:--:|:---:|
| <img src="docs/images/icon-on.png" width="112" alt="ON: turquoise link and filled dot"> | <img src="docs/images/icon-off.png" width="112" alt="OFF: gray link and hollow dot"> |
| Casting enabled | Casting disabled |

Quit the game, then open **LINK4BRICK** and set **ENABLED: OFF** to restore the original launchers. In published v0.2.2, opening AudioCast again toggles it OFF. The icon represents **enabled/disabled**, not whether Push is connected. If StockUI shows an old icon, leave and reopen Apps or reboot.

**Always switch OFF before updating or deleting the app.** Replacing the app folder while it is ON can remove its activation records while leaving launcher wrappers behind. See [recovery instructions](docs/TROUBLESHOOTING.md#incomplete-upgrade-or-missing-activation-records) if this has happened.

## Scope and behavior

| Item | Behavior |
|---|---|
| Games | Existing GB/GBA RetroArch launchers; Gambatte, mGBA and gpSP variants matching StockUI's launcher format |
| Other systems and menu sounds | Outside this release's scope |
| Permanent changes | SD-card app files and reversible GB/GBA launcher wrappers |
| Firmware and global ALSA configuration | Unchanged |
| Game audio | Duplicated to the local speaker and Link Audio |
| Network failure | The relay keeps draining so a stalled sender does not block the speaker path |
| Audio setup failure | Falls back to the original launcher |
| Saves and core options | Stay in their normal locations |
| RetroArch configuration overrides | Automatic core/game overrides and save-on-exit are disabled only while casting; existing files are preserved |
| Logging | App output is discarded; RetroArch file logging is disabled during casting; old logs are left untouched |

## Help and development

- [Troubleshooting and recovery](docs/TROUBLESHOOTING.md)
- [Build instructions and architecture](docs/DEVELOPMENT.md)
- [Release history](CHANGELOG.md)
- [Report a problem](https://github.com/VolkerJunginger/AudioCast-TrimUI/issues/new/choose)

AudioCast uses [Ableton Link](https://github.com/Ableton/link) and the [tg5040 toolchain](https://github.com/loveretro/tg5040-toolchain). The current ON/OFF icons were supplied by the project maintainer and are packaged unchanged. This is an independent project, not an official Ableton or TrimUI product. See [third-party notices](THIRD_PARTY.md).

## License

[GPL-2.0-or-later](LICENSE), matching the open-source license used for Ableton Link. See [third-party notices](THIRD_PARTY.md) for dependency attribution.

The preview is named **LINK4BRICK**. Its SD-card folder remains `Apps/AudioCast` to preserve existing installations and launcher backups. The published v0.2.2 still uses the AudioCast name.

## Experimental clock and settings build

Open **Apps → LINK4BRICK**. Use **UP/DOWN** to select a row, **A** to change it and **B** to return.

| Setting | Choices |
|---|---|
| Enabled | ON / OFF; retains reversible launchers and changing icon |
| Link audio | ON / OFF; OFF leaves the local speaker and Link clock active |
| Clock | OFF / FMS GBA / DMGo Game Boy / GBA pulse |

Settings are saved on the SD card and apply to the next game. Close the game before changing them. **LINK AUDIO: OFF** intentionally removes **Brick Out** from the network; enable it again when you want to stream audio.

For **FMS**, choose **FMS / GBA**, launch it from Games → GBA, and set **SYNC IN / GBA**. Brick START queues the next four-beat “one”; press again to stop or cancel. This behavior passed the user's Brick/Push test. The tested 64 ms local audio buffer and temporary CPU policy are retained. Compensate incoming-audio delay on Push; AudioCast applies no delay offset.

For **DMGo**, choose **DMGO / GAME BOY**, launch it from Games → GB, and select **SETUP → SYNC: LINK IN**. START queues the first external clock to the next “one”. The adapter has passed emulator tests with the developer's DMGo v1 ROM, but has not yet been tested on the Brick. DMGo is obtained separately; no ROM is included.

This is **not a universal Game Link implementation**. Each program needs its own protocol. FMS native serial, GBA GPIO pulses and DMGo serial clock are the implemented modes; trading, multiplayer, LSDJ and general MIDI are not supported. See [setup, validation and limits](docs/VIRTUAL_LINK_CABLE.md).
