#!/bin/sh
# Normal launcher integration: choose by core/protocol, never by ROM filename.
selected=0
remaining=$#
while [ "$remaining" -gt 0 ]; do
  argument="$1"; shift; remaining=$((remaining - 1))
  if [ "$argument" = -L ] && [ "$remaining" -gt 0 ]; then
    core="$1"; shift; remaining=$((remaining - 1))
    case "$core" in
      */mgba_libretro.so|*/gambatte_gb_libretro.so|*/gambatte_libretro.so)
        case "$AUDIOCAST_LINK_PROTOCOL:$core" in
          dmgo-gb:*/gambatte_gb_libretro.so|dmgo-gb:*/gambatte_libretro.so|dmgo-gb:*/mgba_libretro.so|stepper-gba:*/mgba_libretro.so|fms-gba:*/mgba_libretro.so|gba-clock:*/mgba_libretro.so) ;;
          *) set -- "$@" -L "$core"; continue;;
        esac
        set -- "$@" -L "$AC_APP/cores/mgba-link_libretro.so"
        selected=1
        original_core="$core";;
      *) set -- "$@" -L "$core";;
    esac
  else
    set -- "$@" "$argument"
  fi
done
if [ "$selected" = 0 ]; then
  unset AUDIOCAST_CLOCK_SOCKET AUDIOCAST_PPQN AUDIOCAST_OFFSET_US AUDIOCAST_LINK_PROTOCOL AUDIOCAST_CABLE_ACTIVE AUDIOCAST_CLOCK_DIAGNOSTICS
  exec "$AC_SD/RetroArch/ra64.trimui" --config "$AC_RUN/ra.cfg" --appendconfig "$AC_RUN/override.cfg" "$@"
fi
mkdir -p "$AC_APP/cable/states" || exit 1
ac_before_private_override=$(cat "$AC_RUN/override.cfg") || exit 1
cat >> "$AC_RUN/override.cfg" <<CFG
savestate_directory = "$AC_APP/cable/states"
savestate_auto_load = "false"
savestate_auto_save = "false"
video_shader_enable = "false"
rewind_enable = "false"
run_ahead_enabled = "false"
preemptive_frames_enable = "false"
fastforward_ratio = "1.0"
video_threaded = "false"
libretro_log_level = "2"
audio_sync = "true"
video_vsync = "false"
audio_rate_control = "false"
audio_max_timing_skew = "0.0"
audio_latency = "64"
CFG
/bin/sh "$AC_APP/cable/runtime-performance.sh" "$AC_SD/RetroArch/ra64.trimui" --config "$AC_RUN/ra.cfg" --appendconfig "$AC_RUN/override.cfg" "$@"
result=$?
# A crash/error before the private core completes a frame falls back once to
# the installed core. A failed test must not make normal FMS unlaunchable.
if [ ! -f "$AC_RUN/clock.sock.ready" ]; then
  remaining=$#
  while [ "$remaining" -gt 0 ]; do
    argument="$1"; shift; remaining=$((remaining - 1))
    if [ "$argument" = -L ] && [ "$remaining" -gt 0 ]; then
      core="$1"; shift; remaining=$((remaining - 1))
      [ "$core" != "$AC_APP/cores/mgba-link_libretro.so" ] || core="$original_core"
      set -- "$@" -L "$core"
    else set -- "$@" "$argument"; fi
  done
  printf '%s\n' "$ac_before_private_override" > "$AC_RUN/override.cfg" || exit 1
  unset AUDIOCAST_CLOCK_SOCKET AUDIOCAST_PPQN AUDIOCAST_OFFSET_US AUDIOCAST_LINK_PROTOCOL AUDIOCAST_CABLE_ACTIVE AUDIOCAST_CLOCK_DIAGNOSTICS
  exec "$AC_SD/RetroArch/ra64.trimui" --config "$AC_RUN/ra.cfg" --appendconfig "$AC_RUN/override.cfg" "$@"
fi
exit "$result"
