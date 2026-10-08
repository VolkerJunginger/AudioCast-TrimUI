#!/bin/sh
# Sourced by the audio session before forking the sender and normal game.
# Config contains data only; it is never sourced or executed.
ac_cable_prepare() {
  unset AUDIOCAST_CLOCK_SOCKET AUDIOCAST_PPQN AUDIOCAST_OFFSET_US AUDIOCAST_LINK_PROTOCOL AUDIOCAST_CABLE_ACTIVE
  [ -r "$AC_APP/cable/config.txt" ] || return 0
  ac_cable_protocol=off
  ac_cable_ppqn=2
  ac_cable_offset=0
  while IFS='=' read -r ac_cable_name ac_cable_value; do
    case "$ac_cable_name" in
      PROTOCOL) case "$ac_cable_value" in off|gba-clock|fms-gba|dmgo-gb|stepper-gba) ac_cable_protocol="$ac_cable_value";; *) return 0;; esac;;
      PPQN) case "$ac_cable_value" in 1|2|4|6|8|12|16|24|48|96) ac_cable_ppqn="$ac_cable_value";; *) return 0;; esac;;
      OFFSET_US)
        ac_cable_magnitude=${ac_cable_value#-}
        case "$ac_cable_magnitude" in ''|*[!0-9]*) return 0;; esac
        [ "$ac_cable_value" -ge -250000 ] 2>/dev/null && [ "$ac_cable_value" -le 250000 ] 2>/dev/null || return 0
        ac_cable_offset="$ac_cable_value";;
      ''|'#'*) ;;
      *) return 0;;
    esac
  done < "$AC_APP/cable/config.txt"
  [ "$ac_cable_protocol" != off ] || return 0
  case "$ac_cable_protocol:$ac_cable_ppqn" in
    stepper-gba:4|stepper-gba:6|stepper-gba:12|stepper-gba:24|stepper-gba:48|stepper-gba:96|fms-gba:24|dmgo-gb:16|gba-clock:1|gba-clock:2|gba-clock:4|gba-clock:8|gba-clock:12) ;;
    *) return 0;;
  esac
  [ -x "$AC_APP/bin/audiocast-core-probe" ] && [ -r "$AC_APP/cores/mgba-link_libretro.so" ] || return 0
  # A failing loader leaves casting on the original core. This is not a game
  # compatibility test; successful frame execution has a separate handshake.
  "$AC_APP/bin/audiocast-core-probe" "$AC_APP/cores/mgba-link_libretro.so" || return 0
  AUDIOCAST_LINK_PROTOCOL="$ac_cable_protocol"
  AUDIOCAST_CLOCK_SOCKET="$AC_RUN/clock.sock"
  AUDIOCAST_PPQN="$ac_cable_ppqn"
  AUDIOCAST_OFFSET_US="$ac_cable_offset"
  AUDIOCAST_CABLE_ACTIVE=1
  export AUDIOCAST_LINK_PROTOCOL AUDIOCAST_CLOCK_SOCKET AUDIOCAST_PPQN AUDIOCAST_OFFSET_US AUDIOCAST_CABLE_ACTIVE
}
ac_cable_prepare
unset ac_cable_protocol ac_cable_ppqn ac_cable_offset ac_cable_magnitude ac_cable_name ac_cable_value
