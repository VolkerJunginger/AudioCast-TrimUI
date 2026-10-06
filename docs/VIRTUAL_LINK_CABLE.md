# Virtual link cable — experimental integration

The intended workflow is **AudioCast ON → open a ROM from its normal game list →
select external sync in that program**. AudioCast supplies the virtual cable in
the emulator. No app launches FMS or another music program on the user's behalf.

The standalone FMS Sync and Launch Test apps failed to open FMS on the tested
Hammer. Normal FMS playback and AudioCast casting still work. Their failure is
not yet diagnosed on the device; successful host tests do not establish that the
private core runs on StockUI. Those apps are not the intended interface and are
not included in the integrated virtual-cable package.

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
No diagnostic log is created. ALSA/configuration/socket/handshake files live under
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

Synthetic GBA tests verify pin-level rising edges without any FMS ROM, including
maximum pulse rate, counter wrap and stale-input behavior. Normal-launcher tests
verify core-based selection, numeric configuration, unchanged unrelated emulators
and fallback before a first frame. A private FMS 1.31 host test verifies actual
sequencer tick increments. Hardware Link synchronization and the earlier startup
failure remain unverified. DMGo is not implemented yet.
