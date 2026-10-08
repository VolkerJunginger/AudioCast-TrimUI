#!/bin/sh
# Temporary governor only; restore the previous policy at session exit.
POLICY=/sys/devices/system/cpu/cpu0/cpufreq
old=$(cat "$POLICY/scaling_governor" 2>/dev/null) || old=
changed=0
child=
cleanup() {
  if [ "$changed" = 1 ] && [ "$(cat "$POLICY/scaling_governor" 2>/dev/null)" = performance ]; then
    { printf '%s\n' "$old" > "$POLICY/scaling_governor"; } 2>/dev/null || :
  fi
  if [ -n "$child" ]; then kill -TERM "$child" 2>/dev/null || :; wait "$child" 2>/dev/null || :; fi
}
trap cleanup EXIT
trap 'exit 143' TERM HUP
trap 'exit 130' INT
case "$old" in
  ''|*[!a-zA-Z0-9_-]*) ;;
  *)
    available=$(cat "$POLICY/scaling_available_governors" 2>/dev/null) || available=
    case " $available " in
      *' performance '*)
        if [ "$old" != performance ] && [ -w "$POLICY/scaling_governor" ]; then
          if { printf '%s\n' performance > "$POLICY/scaling_governor"; } 2>/dev/null; then changed=1; fi
        fi;;
    esac;;
esac
"$@" &
child=$!
wait "$child"
result=$?
child=
exit "$result"
