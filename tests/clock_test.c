#include "../sync/clock.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>
int main(void) {
    struct ACClockSnapshot s = {AC_CLOCK_MAGIC, AC_CLOCK_VERSION, 1000000, 0, 120, 1, 0};
    assert(sizeof(s) == 40);
    assert(ac_clock_valid(&s, 1000000));
    assert(!ac_clock_valid(&s, 999999));
    assert(!ac_clock_valid(&s, 1500001));
    s.tempo = NAN; assert(!ac_clock_valid(&s, 1000000)); s.tempo = 120;
    s.beat = INFINITY; assert(!ac_clock_valid(&s, 1000000)); s.beat = 0;
    s.version = 2; assert(!ac_clock_valid(&s, 1000000)); s.version = 1;
    int64_t i;
    assert(ac_next_pulse(&s, 1000000, 2, INT64_MIN, &i) == 1250000 && i == 1);
    assert(ac_next_pulse(&s, 1500000, 2, 1, &i) == 1750000 && i == 3);
    // Delays skip missed pulses. Previously emitted grid positions are not duplicated.
    assert(ac_next_pulse(&s, 51000000, 2, 1, &i) == 51250000 && i == 201);
    assert(ac_next_pulse(&s, 1100000, 2, 1, &i) == 1500000 && i == 2);
    for (int bpm = 30; bpm <= 300; bpm += 15) {
        s.tempo = bpm;
        for (int ppq = 1; ppq <= 12; ppq *= 2) {
            int64_t last = INT64_MIN, now = s.monotonic_us;
            for (int n = 1; n <= 10000; ++n) {
                int64_t due = ac_next_pulse(&s, now, ppq, last, &i);
                assert(i == n && due > now);
                double ideal = s.monotonic_us + n * 60000000.0 / (bpm * ppq);
                assert(fabs(due - ideal) <= 1.1);
                now = due; last = i;
            }
        }
    }
    puts("PASS: clock validation, future grid, missed pulses, duplicate protection, 10,000-edge drift");
}
