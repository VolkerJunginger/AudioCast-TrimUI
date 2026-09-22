#!/bin/sh
# AudioCast v0.2b managed wrapper; original remains beside this file.
SD="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)" || exit 1
if [ -x "$SD/Apps/AudioCast/run.sh" ] && [ -f "$SD/Apps/AudioCast/enabled" ]; then
  exec "$SD/Apps/AudioCast/run.sh" "$0.audiocast-run" "$@"
fi
exec /bin/sh "$0.audiocast-original" "$@"
