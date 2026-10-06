#!/bin/sh
# Dedicated FMS profile. Installed game launchers are never edited.
umask 077
exec >/dev/null 2>&1
AC_SYNC="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)" || exit 1
AC_SD="$(CDPATH= cd -- "$AC_SYNC/../.." && pwd)" || exit 1
IFS= read -r ROM < "$AC_SYNC/rom-path.txt" || exit 1
case "$ROM" in /*) ;; *) ROM="$AC_SD/$ROM" ;; esac
[ -r "$ROM" ] || exit 1
case "$ROM" in *.gba|*.GBA) ;; *) exit 1 ;; esac
[ -x "$AC_SYNC/cores/mgba_libretro.so" ] || exit 1
PPQN=2
OFFSET_US=0
# Plain numbers only. This configuration is never executed as shell code.
if [ -r "$AC_SYNC/clock-settings.txt" ]; then
  while IFS='=' read -r name value; do
    case "$name" in
      PPQN) case "$value" in 1|2|4|8|12) PPQN="$value" ;; *) exit 1 ;; esac ;;
      OFFSET_US)
        magnitude=${value#-}
        case "$magnitude" in ''|*[!0-9]*) exit 1 ;; esac
        [ "$value" -ge -250000 ] 2>/dev/null || exit 1
        [ "$value" -le 250000 ] 2>/dev/null || exit 1
        OFFSET_US="$value" ;;
      ''|'#'*) ;;
      *) exit 1 ;;
    esac
  done < "$AC_SYNC/clock-settings.txt"
fi
mkdir -p "$AC_SYNC/saves" || exit 1
export AUDIOCAST_CLOCK_SOCKET=/tmp/audiocast-v0.2b/clock.sock
export AUDIOCAST_PPQN="$PPQN" AUDIOCAST_OFFSET_US="$OFFSET_US"
exec /bin/sh "$AC_SYNC/run.sh" "$AC_SYNC/fms.audiocast-run" "$ROM"
