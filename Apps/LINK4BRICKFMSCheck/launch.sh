#!/bin/sh
# Compatibility check only; the clock bridge is deliberately inactive.
umask 077
exec >/dev/null 2>&1
AC_FMS_CHECK="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)" || exit 1
AC_SD="$(CDPATH= cd -- "$AC_FMS_CHECK/../.." && pwd)" || exit 1
IFS= read -r ROM < "$AC_FMS_CHECK/rom-path.txt" || exit 1
case "$ROM" in /*) ;; *) ROM="$AC_SD/$ROM" ;; esac
case "$ROM" in *.gba|*.GBA) ;; *) exit 1 ;; esac
[ -r "$ROM" ] || exit 1
[ -x "$AC_FMS_CHECK/cores/mgba_libretro.so" ] || exit 1
[ -f "$AC_SD/Apps/LINK4BRICK/enabled" ] || exit 1
[ -r "$AC_SD/Emus/GBA/launch.sh.audiocast-run" ] || exit 1
[ -r "$AC_SD/Apps/LINK4BRICK/run.sh" ] || exit 1
mkdir -p "$AC_FMS_CHECK/saves" || exit 1
unset AUDIOCAST_CLOCK_SOCKET AUDIOCAST_PPQN AUDIOCAST_OFFSET_US
export AC_FMS_CHECK
exec /bin/sh "$AC_SD/Apps/LINK4BRICK/run.sh" "$AC_FMS_CHECK/fms.audiocast-run" "$ROM"
