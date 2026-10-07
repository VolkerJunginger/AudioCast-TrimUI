# Virtual link cable — experimental integration

The intended workflow is **AudioCast ON → open a ROM from its normal game list →
select external sync in that program**. AudioCast supplies the virtual cable in
the emulator. No app launches FMS or another music program on the user's behalf.

FMS now receives clock pulses through the normal GBA launcher on the Hammer.
The latest device test reports acceptable clock timing but audio on Push can
become silent until a restart. Successful local commits do not confirm remote
playback. The next diagnostic iteration reduces clock work;
it remains a hardware test build, not a stable sync release.

## Signal path and supported protocol

Push/Live Link beat timeline → the AudioCast sender's optional local clock
snapshots → emulator virtual link port → ROM external-sync input.

The first backend is **`gba-clock`**, implemented as rising edges on the GBA
link port's SC pin when the emulated program selects GPIO clock input. It uses
the emulator's CPU-cycle scheduler, preserves normal audio/pitch, and rejects
stale clock data. It contains no FMS ROM-name check or FMS RAM patch. Programs
using this same pin/edge convention can consume the signal; their pulse resolution
must match `PPQN`.

For FMS, select **SYNC IN / CLOCK / PPQ2** and use START for transport. This first
backend does not implement GBA-to-GBA serial handshakes, MIDI, or Link Play/Stop.
FMS's several cable protocols are distinct: [FMS external-sync guide](https://lo-bit.club/fms/guide#ext-sync).

Game Boy serial-link input, including DMGo's LINK IN, needs a separate bit/byte
adapter and protocol validation. The GBA GPIO clock adapter is not a universal
Game Boy cable: [DMGo description](https://audiowanderer.com/AW/youtube/dmgo-is-out-new-music-program-for-the-good-old-game-boy/).

## Normal launcher integration

`Apps/AudioCast/cable/config.txt` contains data, never executable shell code:

```
PROTOCOL=gba-clock
PPQN=2
OFFSET_US=0
```

`PROTOCOL=off` retains casting alone and is the source default. The experimental
package enables `gba-clock`. Regular AudioCast ON/OFF governs the normal game
wrappers as before. The optional bridge starts with the game session and ends
with it; no always-running FMS launcher is installed.

The normal mGBA core argument is redirected to a private core stored inside
AudioCast. Other core arguments are preserved. The installed core and RetroArch
binary remain untouched. An external process cannot present a cable to an
unmodified emulator that exposes no link-port interface; this private core supplies
that missing interface. Current StockUI integration covers mGBA, not gPSP or
Gambatte.

Before selection, a helper checks that the private core can be loaded and exports
the required APIs. A one-byte first-frame handshake under `/tmp` distinguishes
successful emulation from an initialization failure. If the private core fails
before completing a frame, the normal launcher retries once using the installed
core and ordinary casting. That fallback supplies **no sync**. A successful
library check or a visible AudioCast ON icon does not prove that sync is active.

Battery saves retain the normal game's path. Private-core save states go under
`Apps/AudioCast/cable/states`; old auto-loaded states are disabled for this mode.
The preview itself creates no diagnostic log. The separate reversible diagnostic
patch requested for device testing records the normal game launch at the SD-card
root as `AudioCast-Link-Sync-test.log`. ALSA/configuration/socket/handshake files live under
the existing process-owned `/tmp/audiocast-v0.2b` session and are cleaned up when it
ends. Nothing changes `/etc/asound.conf`, firmware, RetroArch's binary or minarch.

## Install and undo

This remains an experimental build until normal game launch and actual clock input
are tested successfully on the Hammer.

1. Quit the game and turn the **working AudioCast OFF**. Verify its gray OFF icon.
2. Back up `Apps/AudioCast` to the Mac. Keep the known-working installer available.
3. Copy only the new `AudioCast` folder into the existing `Apps` folder; do not
   replace the whole `Apps` folder. Existing ROMs and game lists are unchanged.
4. Safely eject, reboot, turn AudioCast ON, then launch FMS from the **normal GBA
   game list using mGBA**. Configure its clock input as above.
5. Verify local/Push sound and that changing Link BPM changes the sequence rate.
   If sound works but no clock arrives, treat this as a failed sync test, not proof
   of cable support. The original-core fallback can keep ordinary sound working.

To return to casting alone, set `PROTOCOL=off`. To revert the application, close
the game, turn AudioCast OFF, and restore the backed-up AudioCast folder. Its OFF
action restores the original launcher bytes using the existing checksum manifest.

## Evidence

Synthetic GBA tests verify pin-level rising edges without a game ROM, including
maximum pulse rate, counter wrap, stale input, peer loss and uneven frame timing.
The clock advances on emulated CPU time and gradually corrects its mapping to
Link's timeline, instead of copying every frame's wall-clock jitter into pulses.
Long pauses and phase changes reacquire the current grid without replaying a
backlog. CLOCK mode still uses the ROM's START/STOP controls.

The private GBA core converts all four emulated DAC rates to fixed 48 kHz stereo
using mGBA's sinc resampler. It avoids frontend AV/audio reinitialization during
load or SOUNDBIAS changes; the device log showed those reopens losing the exclusive
speaker with `Device or resource busy`. The temporary ALSA hardware tee remains
the same. There is no dmix dependency or global ALSA configuration change.
Private video uses synchronous rendering because the device's threaded GL path
failed before the first frame. Core logging suppresses the DMA informational
flood. Both settings are restored on fallback to the installed core.

Generated sine tests verify sample count, pitch and stereo across 32768, 65536,
131072 and 262144 Hz DAC rates. Libretro lifecycle tests check fixed-rate audio
and no AV reopen during load/reset/run, including ARM64 execution in CI.
Normal-launcher tests verify selection, configuration and fallback.
Private FMS tests use the user's local ROM and BIOS; neither is distributed.
Device audio, audible clock stability and latency require the next Hammer test.
DMGo is not implemented yet.

The sender supports an opt-in diagnostic recovery test using
`AUDIOCAST_AUDIO_RECOVERY=1` and `AUDIOCAST_AUDIO_DIAGNOSTICS=1`. The ordinary
sender mode remains the default. Repeated short PCM pauses can leave a sample
counter's timestamps increasingly in the past despite successful audio commits.
Recovery reacquires current time when audio is more than 64 ms late. Its commits
align with the pinned Link encoder's 125-frame stereo PCM packet, so the encoder
does not extend cached old time across the recovery. Audio remains S16 stereo at
48 kHz; neither PCM samples nor the Link participant, channel identity, tempo or
virtual cable are changed. The clock's working playback settings are preserved
by the separate diagnostic installer. This does not replace missing PCM or
prove Wi-Fi delivery to Push.

Deterministic timeline tests cover exact sample continuity, slow sources,
repeated short stalls, long pauses and disconnected sinks. A real Link Audio
receiver compares legacy and recovery timestamps under delayed, fragmented PCM
input and checks non-silent audio through a tempo change. Timing diagnostics in
the requested device log report timestamp lag, input gaps and recovery counts.
Brick/Push performance still requires hardware validation.

## Two-second tempo sampling test

Set `AUDIOCAST_CLOCK_REFRESH_MS=2000` to opt into a local clock. At first peer
connection it aligns with Link's beat, then advances continuously using that
initial tempo. Every two seconds it samples Link's tempo and updates the local
rate without jumping its beat position. A peer disconnect stops external clock;
a new connection aligns again. Tempo changes can take up to two seconds to arrive.
This mode follows tempo; it does not continuously correct phase to Link's grid.

The sender publishes small local socket heartbeats every 100 ms with extrapolated
beat positions. Those fresh heartbeats keep the existing core's 500 ms stale-clock
protection active. They do not fetch Link's session state each time. The emulator
core and its playback pacing are unchanged. The sender is built in Release mode
for this diagnostic iteration. `tools/prepare_link.py link` initializes unused
SDK buffer placeholder metadata before optimized constructor copies; normal
commits still supply the actual packet fields. CI retains strict warnings.

This reduces local clock processing and IPC, not audio bandwidth. Stereo S16 PCM
still streams continuously at 48 kHz. The mode is optional; without the variable
the existing clock behavior is preserved. Deterministic tests cover sampling,
beat continuity, TTL, disconnect and rejoin. Native two-peer tests verify tempo
changes and sparse fresh heartbeats while idle and during actual Link Audio
streaming. Push dropout recovery still requires a hardware test.
