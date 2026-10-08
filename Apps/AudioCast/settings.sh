#!/bin/sh
# Data-only settings. Applies to the next normal game session.
set -u
umask 077
APP="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)" || exit 1
AUDIO=on
if [ -f "$APP/settings.txt" ] && [ ! -L "$APP/settings.txt" ]; then
  while IFS='=' read -r name value; do
    case "$name:$value" in LINK_AUDIO:on) AUDIO=on;; LINK_AUDIO:off) AUDIO=off;; esac
  done < "$APP/settings.txt"
fi
CLOCK=off
if [ -f "$APP/cable/config.txt" ] && [ ! -L "$APP/cable/config.txt" ]; then
  while IFS='=' read -r name value; do
    case "$name:$value" in PROTOCOL:off|PROTOCOL:fms-gba|PROTOCOL:gba-clock|PROTOCOL:dmgo-gb) CLOCK=$value;; esac
  done < "$APP/cable/config.txt"
fi
case "${1:-show}" in
  show) printf 'LINK_AUDIO=%s\nPROTOCOL=%s\n' "$AUDIO" "$CLOCK"; exit 0;;
  audio-value) [ "$AUDIO" != off ] && echo 1 || echo 0; exit 0;;
  get-audio) echo "$AUDIO"; exit 0;;
  get-clock) echo "$CLOCK"; exit 0;;
  set-audio) case "${2:-}" in on|off) target="$APP/settings.txt";; *) exit 2;; esac;;
  set-clock) case "${2:-}" in off|fms-gba|gba-clock|dmgo-gb) target="$APP/cable/config.txt";; *) exit 2;; esac;;
  *) echo 'usage: settings.sh show|get-audio|get-clock|audio-value|set-audio on|off|set-clock off|fms-gba|gba-clock' >&2; exit 2;;
esac
[ ! -d /tmp/audiocast-v0.2b ] || { echo 'Close the running game first.' >&2; exit 1; }
[ ! -L "$target" ] && { [ ! -e "$target" ] || [ -f "$target" ]; } || exit 1
LOCK=/tmp/audiocast-settings.lock
mkdir "$LOCK" 2>/dev/null || exit 1
TEMP="$target.tmp.$$"
OWN_TEMP=0
cleanup() { [ "$OWN_TEMP" = 0 ] || rm -f "$TEMP"; rmdir "$LOCK" 2>/dev/null || :; }
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM HUP
if [ "$1" = set-audio ]; then
  (set -C; printf 'LINK_AUDIO=%s\n' "$2" > "$TEMP") || exit 1
else
  [ -d "$APP/cable" ] || exit 1
  ppqn=2
  [ "$2" != fms-gba ] || ppqn=24
  [ "$2" != dmgo-gb ] || ppqn=16
  (set -C; printf 'PROTOCOL=%s\nPPQN=%s\nOFFSET_US=0\n' "$2" "$ppqn" > "$TEMP") || exit 1
fi
OWN_TEMP=1
chmod 644 "$TEMP" && mv -f "$TEMP" "$target" || exit 1
