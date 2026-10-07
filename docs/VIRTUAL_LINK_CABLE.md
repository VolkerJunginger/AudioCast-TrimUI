# Virtual link cable — experimental integration

The intended workflow is **AudioCast ON → open a ROM from its normal game list →
select external sync in that program**. AudioCast supplies the virtual cable in
the emulator. No app launches FMS or another music program on the user's behalf.

The latest diagnostic tests FMS's native GBA serial protocol: a 24-PPQN clock
with explicit START and STOP. Brick START queues the next four-beat Link boundary;
a second press cancels a queued start or stops playback. Live tempo changes update
the tick period without repeating START. The previous change-triggered tempo
latch is disabled in this test. Timing offset is zero; delay compensation is
outside this iteration. The working 64 ms audio buffer and temporary CPU policy
are preserved. This remains a hardware test build, not a stable sync release.

## Signal path and supported protocol

Push/Live Link beat timeline → the AudioCast sender's optional local clock
snapshots → emulator virtual link port → ROM external-sync input.

The first backend is **`gba-clock`**, implemented as rising edges on the GBA
link port's SC pin when the emulated program selects GPIO clock input. It uses
the emulator's CPU-cycle scheduler, preserves normal audio/pitch, and rejects
stale clock data. It contains no FMS ROM-name check or FMS RAM patch. Programs
using this same pin/edge convention can consume the signal; their pulse resolution
must match `PPQN`.

The new **`fms-gba`** backend completes external-clock, normal 8-bit serial
transfers in the emulated GBA link port. FMS defines a 24-PPQN TICK byte (`0x01`),
START (`0x02`) and STOP (`0x03`): [FMS developer protocol](https://github.com/ess-m/fms-docs/blob/main/sync.md).
Select **SYNC IN / GBA** in FMS. The GBA protocol fixes its pulse rate at 24 PPQN;
the CLOCK/PPQ setting belongs to the separate GPIO mode. This is FMS's GBA serial
protocol, not MIDI, and does not implement every serial cable protocol.

In this mode, Brick START is intercepted only while the program selects normal
8-bit external serial input. It requests a sequence start on the next multiple
of four Link beats. That serial START supplies the sequence reset that GPIO
pulses alone cannot express. The deadline follows tempo changes while queued.
Once playing, tick deadlines advance on emulated CPU cycles; new live tempo and
bounded phase corrections apply between ticks. Ordinary frame wakeups do not
reschedule a pending tick. No backlog is replayed after pauses or missing data.
Peer loss or stale data sends STOP where the receiver is ready, and rejoining
requires another Brick START. Link Play/Stop does not control this transport.
Four-beat phase alignment follows Link's [quantized launching model](https://ableton.github.io/link/).

`gba-clock` remains available for GPIO receivers: select **SYNC IN / CLOCK /
PPQ2** in FMS and use its normal START control. It carries tempo pulses without
an explicit sequence/bar reset. Neither mode implements standard MIDI messages.
FMS's cable modes are distinct: [FMS external-sync guide](https://lo-bit.club/fms/guide#ext-sync).

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

For the serial transport diagnostic use:

```
PROTOCOL=fms-gba
PPQN=24
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

A separate synthetic serial receiver verifies the actual received bytes: START
on beat 4 at zero offset, no ticks before START, 24-PPQN timing at 120 BPM,
a live change to 150 BPM without another START, cancellation, STOP, peer loss,
stale data and 32-bit CPU counter wrap. START routing passes through unchanged
outside external serial receive mode. These are emulator transport tests;
they do not measure audible alignment or Push playback latency.


Synthetic GBA tests verify pin-level rising edges without a game ROM, including
maximum pulse rate, counter wrap, stale input, peer loss and uneven frame timing.
The clock advances on emulated CPU time and gradually corrects its mapping to
Link's timeline, instead of copying every frame's wall-clock jitter into pulses.
A scheduled pulse now retains its deadline across ordinary frame updates.
A pulse oscillator applies phase corrections only between edges, bounded to
400 parts per million (100 microseconds per pulse at 120 BPM / PPQ2).
Explicit tempo changes can retime the next edge immediately. Synthetic tests
cover larger frame jitter, small phase steps and a 120-to-150 BPM tempo change;
these measurements are emulated pin timing, not proof of audible FMS stability.
Long pauses and large phase changes reacquire the current grid without replaying a
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

## Historical two-second tempo sampling experiment (disabled)

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

## Historical change-triggered tempo latch experiment (disabled)

`AUDIOCAST_CLOCK_MODE=tempo-latch` opts into a tempo-only local clock in both
sender and private core. On peer connection it takes Link's tempo and initial
phase. The sender checks tempo in the background every 500 ms. An actual change
opens a one-second window with 100 ms checks; a moving candidate restarts the
window and a return to the held tempo cancels it. Once the new tempo stays stable
for one second, the sender updates the rate once, preserving local beat position.
An unchanged tempo never reanchors the oscillator. Small numeric noise is ignored.

The core holds an exact CPU-cycle pulse period with zero ongoing phase correction.
A newly confirmed tempo takes effect after the pending pulse, avoiding a shortened
or doubled pulse interval. This follows tempo without continuously pulling notes
back onto Link's grid; hardware clock drift can accumulate between connections.
Peer loss or stale heartbeats stop clock output. Reconnection and long frontend
pauses align again rather than replaying a backlog. Link Play/Stop is not transport.

Fresh local heartbeats remain at 100 ms for the existing 500 ms stale-feed guard.
The audio sender's PCM path, 48 kHz rate, timestamp recovery, channel and participant
are unchanged. The diagnostic patch retains the current 64 ms buffer, CPU policy,
icons and normal game launchers and produces the requested SD-root log. Log lines
show held tempo, background checks, listening windows and actual rate latches;
the core identifies `scheduler=tempo-latch` with zero edge correction. Tests cover
five minutes at constant tempo despite phase steps, moving/reversed candidates,
confirmed changes, disconnect/rejoin, stale data and real Link IPC. Audible FMS
stability remains a hardware test requirement.
