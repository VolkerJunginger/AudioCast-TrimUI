/* AudioCast virtual GBA clock peripheral.
 * This Source Code Form is subject to the Mozilla Public License, v. 2.0.
 * See https://mozilla.org/MPL/2.0/. */
#include "mgba-clock.h"
#include "clock.h"
#include <mgba/core/core.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/sio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#define AC_GBA_HZ 16777216.0
#define AC_PULSE_CYCLES 33554 /* 2 ms; FMS 1.31 polls SC every ~1 ms. */
struct ACDriver {
    struct GBASIODriver d;
    struct mTimingEvent rise, fall;
    struct ACClockSnapshot clock;
    int fd, ppqn, high, active;
    int64_t offset, host, last, pending;
    uint32_t cycles;
    char socket_path[sizeof(((struct sockaddr_un*)0)->sun_path)];
};
/* Libretro owns one core per process. */
static struct ACDriver ac;
static bool handles(struct GBASIODriver* d, enum GBASIOMode m) { (void)d; return m == GBA_SIO_GPIO; }
static int connected(struct GBASIODriver* d) { (void)d; return 0; }
static uint16_t writeRCNT(struct GBASIODriver* d, uint16_t v) {
    struct ACDriver* a = (struct ACDriver*)d;
    if (d->p->mode == GBA_SIO_GPIO && !(v & 0x10)) v = (v & ~1) | a->high;
    return v;
}
static void level(struct ACDriver* a, int high) {
    a->high = high;
    struct GBASIO* sio = a->d.p;
    if (sio->mode == GBA_SIO_GPIO && !(sio->rcnt & 0x10))
        sio->rcnt = (sio->rcnt & ~1) | high;
}
static void falling(struct mTiming* t, void* ctx, uint32_t late) {
    (void)t; (void)late; level(ctx, 0);
}
static void rising(struct mTiming* t, void* ctx, uint32_t late) {
    struct ACDriver* a = ctx;
    a->last = a->pending;
    if (!a->active) return;
    level(a, 1);
    mTimingSchedule(t, &a->fall, AC_PULSE_CYCLES);
    uint32_t elapsed = (uint32_t)mTimingCurrentTime(t) - a->cycles;
    int64_t host = a->host + (int64_t)llround(elapsed * 1000000.0 / AC_GBA_HZ);
    int64_t due = ac_next_pulse(&a->clock, host, a->ppqn, a->last, &a->pending);
    int64_t delay = (int64_t)llround((due - host) * AC_GBA_HZ / 1000000.0);
    if (delay > 0 && elapsed + delay <= 280896)
        mTimingSchedule(t, &a->rise, (int32_t)delay);
    (void)late;
}
void AudioCastClockRebase(struct mCore* c) {
    if (ac.fd < 0 || c->platform(c) != mPLATFORM_GBA) return;
    struct mTiming* t = &((struct GBA*)c->board)->timing;
    mTimingDeschedule(t, &ac.rise); mTimingDeschedule(t, &ac.fall);
    ac.last = INT64_MIN; ac.active = 0; level(&ac, 0);
}
void AudioCastClockFrame(struct mCore* c) { AudioCastClockFrameAt(c, ac_monotonic_us()); }
void AudioCastClockFrameAt(struct mCore* c, int64_t now) {
    if (ac.fd < 0 || c->platform(c) != mPLATFORM_GBA) return;
    struct GBA* g = c->board;
    struct ACClockSnapshot s;
    /* Bounded, nonblocking read. Invalid/truncated packets are discarded. */
    for (int i = 0; i < 64; ++i) {
        char packet[sizeof(s)+1];
        ssize_t n = recv(ac.fd, packet, sizeof(packet), 0);
        if (n < 0) break;
        if (n != sizeof(s)) continue;
        memcpy(&s, packet, sizeof(s));
        if (!ac_clock_valid(&s, now) || s.monotonic_us < ac.clock.monotonic_us) continue;
        if (ac.active && fabs(s.beat - ac_beat_at(&ac.clock, s.monotonic_us)) > 0.25)
            ac.last = INT64_MIN; /* A changed Link phase must not stall behind the old grid. */
        ac.clock = s;
    }
    bool pending = mTimingIsScheduled(&g->timing, &ac.rise);
    int64_t pendingIndex = ac.pending;
    mTimingDeschedule(&g->timing, &ac.rise);
    ac.active = ac_clock_valid(&ac.clock, now) && ac.clock.peers > 0;
    if (!ac.active) { mTimingDeschedule(&g->timing, &ac.fall); level(&ac, 0); ac.last = INT64_MIN; return; }
    ac.host = now + ac.offset;
    /* masterCycles is always maintained; globalCycles is debugger-only upstream.
       Unsigned subtraction handles the 32-bit timing counter wrapping. */
    ac.cycles = (uint32_t)mTimingCurrentTime(&g->timing);
    /* A deadline can straddle the video-frame boundary by a few CPU cycles.
       Retain that one edge within 1 ms, even if rounding placed it
       just outside the previous frame's planning window; never catch up a backlog of missed edges. */
    if ((pending || (ac.last != INT64_MIN && pendingIndex == ac.last + 1)) &&
        pendingIndex > ac.last) {
        double delta = (pendingIndex / (double)ac.ppqn - ac_beat_at(&ac.clock, ac.host)) *
            60000000.0 / ac.clock.tempo;
        if (fabs(delta) <= 1000) {
            ac.pending = pendingIndex;
            int32_t cycles = delta > 0 ? (int32_t)llround(delta * AC_GBA_HZ / 1000000.0) : 1;
            mTimingSchedule(&g->timing, &ac.rise, cycles > 0 ? cycles : 1);
            return;
        }
    }
    int64_t due = ac_next_pulse(&ac.clock, ac.host, ac.ppqn, ac.last, &ac.pending);
    int64_t delay = (int64_t)llround((due - ac.host) * AC_GBA_HZ / 1000000.0);
    /* Plan only the next video frame. Later frames can refresh tempo and phase. */
    if (delay > 0 && delay <= 280896)
        mTimingSchedule(&g->timing, &ac.rise, (int32_t)delay);
}
void AudioCastClockAttach(struct mCore* c) {
    memset(&ac, 0, sizeof(ac)); ac.fd = -1;
    const char* path = getenv("AUDIOCAST_CLOCK_SOCKET");
    if (!path || c->platform(c) != mPLATFORM_GBA || strlen(path) >= sizeof(ac.socket_path)) return;
    const char* p = getenv("AUDIOCAST_PPQN");
    ac.ppqn = p ? atoi(p) : 2;
    /* Clock-only prototype: <= 12 PPQN leaves enough low time per frame. */
    if (ac.ppqn != 1 && ac.ppqn != 2 && ac.ppqn != 4 && ac.ppqn != 8 && ac.ppqn != 12) return;
    const char* offset = getenv("AUDIOCAST_OFFSET_US");
    char* end = NULL;
    ac.offset = offset ? strtoll(offset, &end, 10) : 0;
    if (offset && (!*offset || !end || *end)) return;
    if (ac.offset < -250000 || ac.offset > 250000) return;
    ac.fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (ac.fd < 0) return;
    fcntl(ac.fd, F_SETFL, O_NONBLOCK); fcntl(ac.fd, F_SETFD, FD_CLOEXEC);
    struct sockaddr_un addr; memset(&addr, 0, sizeof(addr)); addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, path);
    /* Do not take over another process's socket; session cleanup owns removal. */
    if (bind(ac.fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { close(ac.fd); ac.fd = -1; return; }
    strcpy(ac.socket_path, path); ac.last = INT64_MIN;
    ac.d.handlesMode = handles; ac.d.connectedDevices = connected; ac.d.writeRCNT = writeRCNT;
    ac.rise = (struct mTimingEvent){.context=&ac, .callback=rising, .name="AudioCast SC rise", .priority=0x70};
    ac.fall = (struct mTimingEvent){.context=&ac, .callback=falling, .name="AudioCast SC fall", .priority=0x70};
    c->setPeripheral(c, mPERIPH_GBA_LINK_PORT, &ac.d);
}
void AudioCastClockDetach(struct mCore* c) {
    if (ac.fd < 0) return;
    AudioCastClockRebase(c); c->setPeripheral(c, mPERIPH_GBA_LINK_PORT, NULL);
    close(ac.fd); ac.fd = -1; unlink(ac.socket_path);
}
