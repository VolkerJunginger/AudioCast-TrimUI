#!/bin/sh
exec >/dev/null 2>&1
APP="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)" || exit 1
exec /bin/sh "$APP/control.sh" toggle
