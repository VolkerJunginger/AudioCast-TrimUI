/* AudioCast optional local clock IPC; GPL-2.0-or-later. */
#ifndef AUDIOCAST_CLOCK_H
#define AUDIOCAST_CLOCK_H
#include <stdint.h>
#include <math.h>
#include <time.h>
#define AC_CLOCK_MAGIC UINT32_C(0x41434c4b)
#define AC_CLOCK_VERSION 1
#define AC_CLOCK_TTL_US 500000
struct ACClockSnapshot {
    uint32_t magic, version;
    int64_t monotonic_us;
    double beat, tempo;
    uint32_t peers, playing;
};
static inline int64_t ac_monotonic_us(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (int64_t)t.tv_sec * 1000000 + t.tv_nsec / 1000;
}
static inline int ac_clock_valid(const struct ACClockSnapshot* s, int64_t now) {
    return s->magic == AC_CLOCK_MAGIC && s->version == AC_CLOCK_VERSION &&
        isfinite(s->beat) && fabs(s->beat) < 1e10 &&
        isfinite(s->tempo) && s->tempo >= 20 && s->tempo <= 400 &&
        s->playing <= 1 && s->monotonic_us <= now &&
        now - s->monotonic_us <= AC_CLOCK_TTL_US;
}
static inline double ac_beat_at(const struct ACClockSnapshot* s, int64_t us) {
    return s->beat + (us - s->monotonic_us) * s->tempo / 60000000.0;
}
/* Find a future grid edge, skipping missed pulses rather than replaying them. */
static inline int64_t ac_next_pulse(const struct ACClockSnapshot* s, int64_t us,
                                  int ppqn, int64_t last, int64_t* index) {
    double beat = ac_beat_at(s, us);
    int64_t i = (int64_t)floor(beat * ppqn) + 1;
    if (i <= last) i = last + 1;
    *index = i;
    return us + (int64_t)ceil((i / (double)ppqn - beat) * 60000000.0 / s->tempo);
}
#endif
