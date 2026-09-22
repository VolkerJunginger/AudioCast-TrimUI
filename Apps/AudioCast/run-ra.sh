#!/bin/sh
# Called only by the generated SD launcher copy, with original core/ROM args.
# Protect the original main config even if Save Current Configuration is used.
if [ -z "${AC_RUN:-}" ] || [ -z "${AC_SD:-}" ]; then exit 1; fi
exec "$AC_SD/RetroArch/ra64.trimui" --config "$AC_RUN/ra.cfg" \
  --appendconfig "$AC_RUN/override.cfg" "$@"
