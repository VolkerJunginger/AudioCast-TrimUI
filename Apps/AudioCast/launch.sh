#!/bin/sh
exec >/dev/null 2>&1
APP="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)" || exit 1
cd "$APP" || exit 1
/bin/sh "$APP/control.sh" refresh-icon || :
exec "$APP/bin/audiocast-settings"
