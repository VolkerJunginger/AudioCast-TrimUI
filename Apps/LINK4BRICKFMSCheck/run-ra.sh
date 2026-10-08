#!/bin/sh
# This check targets the verified stock mGBA invocation, not arbitrary launchers.
[ -n "${AC_FMS_CHECK:-}" ] && [ -n "${AC_SD:-}" ] && [ -n "${AC_RUN:-}" ] || exit 1
[ "${1:-}" != -v ] || shift
[ "$#" = 3 ] && [ "$1" = -L ] || exit 1
[ "$2" = "$AC_SD/RetroArch/.retroarch/cores/mgba_libretro.so" ] || exit 1
exec /bin/sh "$AC_SD/Apps/LINK4BRICK/run-ra.sh" -v \
  --save "$AC_FMS_CHECK/saves" --savestate "$AC_FMS_CHECK/saves" \
  -L "$AC_FMS_CHECK/cores/mgba_libretro.so" "$3"
