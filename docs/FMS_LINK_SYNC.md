# FMS Link Sync — experimental StockUI build

**Superseded prototype.** The dedicated launcher failed to open FMS on the
tested Hammer. The current direction is [normal-game virtual link cable
integration](VIRTUAL_LINK_CABLE.md), with no FMS-launching app. Do not use this
document as instructions for a finished sync release.

This separate app makes FMS's sequencer follow the tempo and pulse grid of the
Ableton Link session while AudioCast sends its sound to **Brick Out**. It targets
**TrimUI Brick Hammer with StockUI**. The existing AudioCast app and ordinary
GB/GBA game launchers remain unchanged.

The adapter has been tested locally against a privately supplied **FMS 1.31** ROM.
It still needs a listening test on the Brick and Push. This is an experimental
build, not a replacement for the stable AudioCast v0.2.2 release.

## Install

1. Extract **AudioCast-StockUI-FMS-Link-Sync-preview.zip** onto the SD card.
   The new app folder is `Apps/AudioCastSync`. No `.pak` installation is used.
   When copying with Finder, copy only `AudioCastSync` into the existing `Apps`
   folder; do not replace the whole `Apps` folder.
2. Edit `Apps/AudioCastSync/rom-path.txt` to contain the path of your own FMS ROM,
   relative to the SD root. The default is `Roms/GBA/fms.gba`. Paths with spaces
   work. FMS is not included.
3. Boot StockUI, enable Wi-Fi, and join the same Link session as Push/Live.
4. Open **FMS Link Sync** from Apps. Select **Brick Out** on the receiver, as with
   regular AudioCast.
5. In FMS's **Data** view, set **SYNC IN**, **CLOCK**, and **PPQ2**.
   FMS 1.31 defaults to SYNC OUT/GBA on a fresh save. Press SELECT for the top menu
   and B on the grid/data icon to open Data. Move down to the SYNC row; B+Up
   changes OUT to IN. Move right to the protocol and B+Up changes GBA to CLOCK.
   This assumes FMS's default B=Edit mapping.
6. Use START in FMS to start/stop its sequencer. Change BPM on Push/Live to test
   tempo following. Keep FMS in CLOCK input mode while using this app.

The preview uses separate saves in `Apps/AudioCastSync/saves`, so the first launch
starts with a fresh FMS setup. The app creates that directory at launch. To use
an existing pattern bank, copy your FMS battery save into this folder as
`<ROM basename>.srm`, keeping your original save. Do not import an old save state
from another core.

## What to expect

- Clock pulses follow Link's shared beat grid. FMS tempo edits do not change Link.
- This first prototype does **not** follow Link Play/Stop or reset a song to a bar
  boundary. START remains under your control. Pulse phase and song/bar position
  are different; a listening test will establish the useful start procedure.
- No Link peers, a stopped sender, or clock data older than 500 ms stops new pulses.
  On reconnect, it resumes on a future grid edge, without a burst of missed pulses.
- Sound pitch and emulator speed stay at their normal values.
- Normal speaker output and the existing 48 kHz stereo S16 AudioCast route remain.
- Closing the app tears down the audio session and temporary clock socket.
- The sync preview uses its own `/tmp/audiocast-fms-sync` directory and
  `/tmp/audiocast-fms-sync.fifo`, separate from regular AudioCast's runtime.
- The app writes no diagnostic log. Battery saves are intentional user data.

`clock-settings.txt` contains two optional settings:

```
PPQN=2
OFFSET_US=0
```

Match PPQN with FMS's PPQ setting. Supported preview values are 1, 2, 4, 8 and 12.
`OFFSET_US` is a signed timing adjustment, from -250000 to 250000 microseconds.
A positive value advances pulse injection relative to the host clock; a negative
value delays it. Begin with 0. Audio/emulator/Wi-Fi receiver buffering has not yet
been measured on hardware, so audible alignment and long-session drift are not
claimed as verified.

For the first listening test, program a simple repeated click, compare it with a
Push reference beat at 120 BPM, then change to 90 and 150 BPM. Check both the
speaker and Push output. Also pause in the RetroArch menu, resume, and reconnect
Wi-Fi. Avoid fast-forward, rewind and run-ahead; the app disables these in its
private temporary configuration. Report whether the beat holds steady and any
consistent audible lead or lag.

## Reversal and boundaries

To uninstall, close FMS Link Sync and remove `Apps/AudioCastSync` from the SD card.
Keep its `saves` folder if you want your new patterns. Nothing is written to
`/etc/asound.conf`, firmware, the installed emulator cores, RetroArch's binary or
minarch. Runtime ALSA/RetroArch configuration and IPC live only under `/tmp`.
The custom mGBA core is inside this app, selected only when launching this profile.
If audio preflight fails, the app falls back to the installed GBA launcher; that
fallback provides ordinary playback and does not provide Link clock sync.

This preview covers FMS only. DMGo needs its own Game Boy link-protocol adapter.
CrossMix, NextUI and Knulli integration is not included.

## Build and evidence

- Ableton Link commit: `902aef95bf94af49746fdda5369b42cdcfa1e6d2`.
- mGBA commit: `3a5e34be33dc7f8f707e5bc9db69e8a430046f21`.
- Optional clock packets come from the **existing AudioCast Link participant**.
  Beat anchors are translated to `CLOCK_MONOTONIC` timestamps in that process;
  the core does not create a separate Link clock.
- The GBA peripheral drives SC only when it is a GPIO input. The stock core is
  never patched in place. The adapter schedules rising and falling edges with
  mGBA's CPU-cycle event queue; a frame refresh corrects host-time mapping.
- FMS 1.31 CLOCK input polls SC approximately every millisecond and counts rising
  edges. A 2 ms high pulse was validated through its normal input mode.
- Local private test: no pulses -> no sequencer clock advances; at PPQ2,
  300 emulated frames produce 10/20/30/40 pulses at 60/120/180/240 BPM.
  Stale snapshots and peer loss stop new pulses and return SC low.
- Clock math tests verify packet validity, missed-pulse skipping, duplicate
  protection and 10,000-edge deadline drift within one microsecond.
- Synthetic core test: all 24,109 edges arrive at 12 PPQN / 400 BPM, including
  multiple clock edges inside one video frame and a 32-bit cycle-counter wrap,
  with a simulated 1 ms SC poll over five emulated minutes.
- Actual two-participant Link test: the existing sender exports the peer's
  90/150 BPM tempo even while PCM input is idle; receiver loss is harmless.
- Hardware performance, audible offset and long-session alignment remain to test.

GitHub Actions builds and checks the additive ARM64 StockUI ZIP plus corresponding
source (excluding upstream ROM/save test fixtures). It does not publish a stable release or include an FMS ROM/save. The
optional `tests/fms_probe.c` needs your privately supplied FMS 1.31 ROM. It reads
version-specific runtime counters only for assertions; production code never
patches ROM contents, settings, or sequencer memory.

Useful sources: [FMS guide](https://lo-bit.club/fms/guide#ext-sync),
[mGBA SIO interface](https://github.com/mgba-emu/mgba/blob/3a5e34be33dc7f8f707e5bc9db69e8a430046f21/include/mgba/gba/interface.h),
[Link API](https://github.com/Ableton/link/blob/902aef95bf94af49746fdda5369b42cdcfa1e6d2/include/ableton/Link.hpp).
