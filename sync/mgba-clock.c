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
#include <errno.h>
#include <stdio.h>
#include "mgba-gb-clock.c"
#define AC_GBA_HZ 16777216.0
#define AC_PULSE_CYCLES 33554 /* 2 ms; FMS 1.31 polls SC every ~1 ms. */
struct ACDriver {
    struct GBASIODriver d;
    struct mTimingEvent rise, fall;
    struct ACClockSnapshot clock;
    int fd, ppqn, high, active, ready, tempoLatch, serial;
    int wanted, serialPlaying, startKey, quantum, stepper, fmsClock;
    double startBeat;
    unsigned starts, stops, missed;
    int64_t offset, wall, last, pending;
    double host;
    unsigned frames, pulses, relocks, expired;
    uint32_t edgeCycle;
    double edgeTempo, scheduledTempo, intervalMin, intervalMax, errorMax, correctionMax;
    int diagnostics;
    uint32_t cycles;
    char socket_path[sizeof(((struct sockaddr_un*)0)->sun_path)];
};
/* Libretro owns one core per process. */
static struct ACDriver ac = {.fd = -1};
static bool handles(struct GBASIODriver* d, enum GBASIOMode m) { (void)d; return ac.serial ? m == GBA_SIO_NORMAL_8 : m == GBA_SIO_GPIO; }
static int connected(struct GBASIODriver* d) { (void)d; return ac.serial ? 1 : 0; }
static bool serialTransfer(struct GBASIODriver* d) { return !(ac.serial && d->p->mode == GBA_SIO_NORMAL_8); }
static uint16_t serialControl(struct GBASIODriver* d, uint16_t value) { (void)d; return value; }
static int serialReceiver(struct ACDriver* a) {
    return a->d.p && a->d.p->mode == GBA_SIO_NORMAL_8 &&
        !(a->d.p->siocnt & 1) && (a->d.p->siocnt & 0x4000);
}
static int serialReady(struct ACDriver* a) {
    return serialReceiver(a) && (a->d.p->siocnt & 0x80);
}
static int serialSend(struct ACDriver* a, uint8_t message, uint32_t late) {
    if (!serialReady(a)) { a->missed++; return 0; }
    GBASIONormal8FinishTransfer(a->d.p, message, late);
    return 1;
}
uint16_t AudioCastClockInput(struct mCore* c, uint16_t keys) {
    if (c->platform(c) == mPLATFORM_GB) return ACGBClockInput(c, keys);
    if (ac.fd < 0 || (!ac.serial && !ac.stepper && !ac.fmsClock) || c->platform(c) != mPLATFORM_GBA) return keys;
    int pressed = !!(keys & 8);
    int receiver = ac.fmsClock ? ac.d.p && ac.d.p->mode == GBA_SIO_GPIO && !(ac.d.p->rcnt & 0x10) : ac.stepper ? ac.d.p && ac.d.p->mode == GBA_SIO_GPIO &&
        (ac.d.p->rcnt & 0x100) && !(ac.d.p->rcnt & 0x40) : serialReceiver(&ac);
    /* SELECT+START saves STEPPER's bank; it is not a transport request. */
    if (ac.stepper && (keys & 4)) receiver = 0;
    if (receiver && pressed && !ac.startKey) {
        ac.wanted = !ac.wanted;
        if (!ac.wanted) ac.startBeat = NAN;
        if (ac.diagnostics) fprintf(stderr, "AUDIOCAST_TRANSPORT request=%s quantum=%d advance_us=%lld\n",
            ac.wanted ? "queue-next-one" : "stop", ac.quantum, (long long)ac.offset);
    }
    ac.startKey = pressed;
    return receiver && ac.serial ? keys & ~8 : keys;
}
static uint16_t writeRCNT(struct GBASIODriver* d, uint16_t v) {
    struct ACDriver* a = (struct ACDriver*)d;
    int mask = a->stepper ? 4 : 1, direction = mask << 4;
    int value = a->stepper ? (!a->high ? 4 : 0) : a->high;
    if (d->p->mode == GBA_SIO_GPIO && !(v & direction)) v = (v & ~mask) | value;
    return v;
}
static void levelAt(struct ACDriver* a, int high, uint32_t late) {
    a->high = high;
    struct GBASIO* sio = a->d.p;
    int mask = a->stepper ? 4 : 1, direction = mask << 4;
    int value = a->stepper ? (!high ? 4 : 0) : high;
    if (sio->mode == GBA_SIO_GPIO && !(sio->rcnt & direction)) {
        int previous = sio->rcnt & mask;
        sio->rcnt = (sio->rcnt & ~mask) | value;
        /* GBA general-purpose mode interrupts on SI high -> low.
           STEPPER installs its clock handler here, not on polled SC. */
        if (a->stepper && previous && !value && (sio->rcnt & 0x100))
            GBARaiseIRQ(sio->p, GBA_IRQ_SIO, late);
    }
}
static void level(struct ACDriver* a, int high) { levelAt(a, high, 0); }
static void falling(struct mTiming* t, void* ctx, uint32_t late) {
    (void)t; (void)late; level(ctx, 0);
}
static void serialRising(struct mTiming* t, struct ACDriver* a, uint32_t late) {
    if (!a->active || !a->wanted) return;
    uint32_t elapsed = (uint32_t)mTimingCurrentTime(t) - a->cycles;
    int64_t host = (int64_t)llround(a->host + elapsed * 1000000.0 / AC_GBA_HZ);
    if (!a->serialPlaying) {
        if (!serialSend(a, 0x02, late)) {
            // Never start late off the one. Wait for another complete bar.
            a->startBeat = NAN; a->pending = a->last = INT64_MIN; return;
        }
        a->serialPlaying = 1; a->starts++;
        a->last = (int64_t)llround(a->startBeat * 24);
        if (a->diagnostics) fprintf(stderr, "AUDIOCAST_TRANSPORT START beat=%.6f quantum=%d advance_us=%lld\n",
            a->startBeat, a->quantum, (long long)a->offset);
        if (a->diagnostics) {
            int64_t delivered = ac_monotonic_us();
            if (ac_clock_valid(&a->clock, delivered)) {
                double beat = ac_beat_at(&a->clock, delivered);
                double error = (beat - a->startBeat) * 60000000.0 / a->clock.tempo;
                fprintf(stderr, "AUDIOCAST_START_TIMING target_beat=%.6f delivery_beat=%.6f delivery_error_us=%.1f tempo=%.3f measurement=host-callback audio_latency_ms=65\n",
                    a->startBeat, beat, error, a->clock.tempo);
            }
        }
    } else {
        a->last = a->pending;
        if (!serialSend(a, 0x01, late)) { a->wanted = 0; return; }
        uint32_t cycle = (uint32_t)mTimingCurrentTime(t) - late;
        if (a->pulses && a->edgeTempo == a->clock.tempo) {
            double interval = (uint32_t)(cycle - a->edgeCycle) * 1000000.0 / AC_GBA_HZ;
            if (!a->intervalMin || interval < a->intervalMin) a->intervalMin = interval;
            if (interval > a->intervalMax) a->intervalMax = interval;
        }
        a->edgeCycle = cycle; a->edgeTempo = a->clock.tempo; a->pulses++;
    }
    // Live phase and tempo, with bounded corrections between 24-PPQN ticks.
    // No delayed ticks are replayed and no START is repeated during tempo edits.
    double period = 60000000.0 / (a->clock.tempo * 24);
    a->pending = a->last + 1;
    double error = (a->pending / 24.0 - ac_beat_at(&a->clock, host)) * 60000000.0 / a->clock.tempo - period;
    double correction = error / 16.0, limit = period * 0.02;
    if (correction > limit) correction = limit;
    if (correction < -limit) correction = -limit;
    if (fabs(correction) > a->correctionMax) a->correctionMax = fabs(correction);
    int64_t delay = (int64_t)llround((period + correction) * AC_GBA_HZ / 1000000.0) - late;
    if (delay < 1) delay = 1;
    if (delay <= INT32_MAX) mTimingSchedule(t, &a->rise, (int32_t)delay);
}
static void rising(struct mTiming* t, void* ctx, uint32_t late) {
    struct ACDriver* a = ctx;
    if (a->serial) { serialRising(t, a, late); return; }
    a->last = a->pending;
    if (!a->active || ((a->stepper || a->fmsClock) && !a->wanted)) return;
    if ((a->stepper || a->fmsClock) && !a->serialPlaying) {
        a->serialPlaying = 1; a->starts++;
        if (a->diagnostics) fprintf(stderr, "AUDIOCAST_PULSE START beat=%.6f ppqn=%d quantum=4 protocol=%s\n", a->startBeat, a->ppqn, a->stepper ? "stepper-gba" : "fms-clock");
    }
    /* A tempo edit can advance the next edge into the previous pulse's
       active interval. Never enqueue the same timing event twice. */
    mTimingDeschedule(t, &a->fall);
    if (a->stepper) level(a, 0);
    levelAt(a, 1, late);
    /* At 96 PPQN / 400 BPM a full period is only 1.56 ms. Leave a
       complete idle interval; never merge two SI interrupt pulses. */
    uint32_t width = AC_PULSE_CYCLES;
    if (a->stepper) {
        uint32_t quarter = (uint32_t)llround(AC_GBA_HZ * 60 / (a->clock.tempo * a->ppqn * 4));
        if (quarter < width) width = quarter;
    }
    mTimingSchedule(t, &a->fall, late < width ? width - late : 1);
    uint32_t cycle = (uint32_t)mTimingCurrentTime(t) - late;
    if (a->pulses && a->edgeTempo == a->clock.tempo) {
        double interval = (uint32_t)(cycle - a->edgeCycle) * 1000000.0 / AC_GBA_HZ;
        if (!a->intervalMin || interval < a->intervalMin) a->intervalMin = interval;
        if (interval > a->intervalMax) a->intervalMax = interval;
    }
    a->edgeCycle = cycle; a->edgeTempo = a->clock.tempo; a->pulses++;
    uint32_t elapsed = (uint32_t)mTimingCurrentTime(t) - a->cycles;
    int64_t host = a->host + (int64_t)llround(elapsed * 1000000.0 / AC_GBA_HZ);
    /* Keep a steady oscillator in emulated CPU time. Correct phase only at
       edges, never by moving the pending edge on each video-frame wakeup.
       At most 400 ppm (100 us per 250 ms pulse) avoids catch-up bursts.
       Tempo-latch mode uses the exact period with no phase correction. */
    double period = 60000000.0 / (a->clock.tempo * a->ppqn);
    double error = (a->last / (double)a->ppqn - ac_beat_at(&a->clock, host)) *
        60000000.0 / a->clock.tempo;
    double correction = a->tempoLatch ? 0.0 : error / ((a->stepper || a->fmsClock) ? 16.0 : 64.0);
    double limit = period * ((a->stepper || a->fmsClock) ? 0.02 : 0.0004);
    if (correction > limit) correction = limit;
    if (correction < -limit) correction = -limit;
    if (fabs(correction) > a->correctionMax) a->correctionMax = fabs(correction);
    a->pending = a->last + 1;
    a->scheduledTempo = a->clock.tempo;
    int64_t delay = (int64_t)llround((period + correction) * AC_GBA_HZ / 1000000.0) - late;
    if (delay < 1) delay = 1;
    if (delay <= INT32_MAX) mTimingSchedule(t, &a->rise, (int32_t)delay);
}
void AudioCastClockRebase(struct mCore* c) {
    if (c->platform(c) == mPLATFORM_GB) { ACGBClockRebase(c); return; }
    if (ac.fd < 0 || c->platform(c) != mPLATFORM_GBA) return;
    struct mTiming* t = &((struct GBA*)c->board)->timing;
    mTimingDeschedule(t, &ac.rise); mTimingDeschedule(t, &ac.fall);
    ac.wanted = ac.serialPlaying = ac.startKey = 0; ac.startBeat = NAN;
    ac.last = INT64_MIN; ac.active = 0; ac.edgeTempo = 0; level(&ac, 0);
}
void AudioCastClockFrame(struct mCore* c) { AudioCastClockFrameAt(c, ac_monotonic_us()); }
/* One-byte first-frame handshake, not a diagnostic log. The launch session
   owns this temporary path and removes it during cleanup. It distinguishes
   a core that actually ran from a loader/initialization failure. */
void AudioCastClockReady(struct mCore* c) {
    (void)c;
    if (ac.ready) return;
    const char* path = getenv("AUDIOCAST_CLOCK_SOCKET");
    if (!path || strlen(path) >= sizeof(ac.socket_path)) return;
    char marker[sizeof(ac.socket_path) + sizeof(".ready")];
    snprintf(marker, sizeof(marker), "%s.ready", path);
    int fd = open(marker, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) { if (errno == EEXIST) ac.ready = 1; return; }
    ac.ready = write(fd, "1", 1) == 1;
    close(fd);
    if (!ac.ready) unlink(marker);
}
static void report(void) {
    if (!ac.diagnostics) return;
    fprintf(stderr, "AUDIOCAST_CLOCK frames=%u pulses=%u relocks=%u expired=%u interval_us=%.1f..%.1f phase_error_max_us=%.1f scheduler=%s edge_correction_max_us=%.1f starts=%u stops=%u missed=%u advance_us=%lld quantum=%d\n",
        ac.frames, ac.pulses, ac.relocks, ac.expired, ac.intervalMin, ac.intervalMax, ac.errorMax, ac.serial ? "bar-serial-24" : ac.stepper ? "stepper-si-irq" : ac.fmsClock ? "fms-sc-clock" : ac.tempoLatch ? "tempo-latch" : "pulse-pll", ac.correctionMax, ac.starts, ac.stops, ac.missed, (long long)ac.offset, ac.quantum);
}
void AudioCastClockFrameAt(struct mCore* c, int64_t now) {
    if (c->platform(c) == mPLATFORM_GB) { ACGBClockFrameAt(c, now); return; }
    if (ac.fd < 0 || c->platform(c) != mPLATFORM_GBA) return;
    struct GBA* g = c->board;
    struct ACClockSnapshot s;
    bool phaseJump = false;
    /* Bounded, nonblocking read. Invalid/truncated packets are discarded. */
    for (int i = 0; i < 64; ++i) {
        char packet[sizeof(s)+1];
        ssize_t n = recv(ac.fd, packet, sizeof(packet), 0);
        if (n < 0) break;
        if (n != sizeof(s)) continue;
        memcpy(&s, packet, sizeof(s));
        if (!ac_clock_valid(&s, now) || s.monotonic_us < ac.clock.monotonic_us) continue;
        if (ac.active && fabs(s.beat - ac_beat_at(&ac.clock, s.monotonic_us)) > 0.25)
            phaseJump = true;
        ac.clock = s;
    }
    bool wasActive = ac.active;
    ac.active = ac_clock_valid(&ac.clock, now) && ac.clock.peers > 0;
    if (!ac.active) {
        if (ac.serial && ac.serialPlaying && serialSend(&ac, 0x03, 0)) { ac.serialPlaying = 0; ac.stops++; }
        if ((ac.serial || ac.stepper || ac.fmsClock) && wasActive) {
            ac.wanted = 0; ac.startBeat = NAN;
            /* Preserve the serial STOP retry if FMS was not yet armed. */
            if (!ac.serial) ac.serialPlaying = 0;
        }
        if (wasActive) ac.expired++;
        mTimingDeschedule(&g->timing, &ac.rise);
        mTimingDeschedule(&g->timing, &ac.fall); level(&ac, 0);
        ac.last = INT64_MIN; ac.edgeTempo = 0;
        return;
    }
    uint32_t cycles = (uint32_t)mTimingCurrentTime(&g->timing);
    double target = now + ac.offset;
    /* Advance on emulated CPU time. Slowly discipline this mapping against
       Link's monotonic clock in the phase-tracking mode. Tempo-latch keeps
       this mapping free-running too. Unsigned subtraction handles timer wrap. */
    double predicted = ac.host + (uint32_t)(cycles - ac.cycles) * 1000000.0 / AC_GBA_HZ;
    double error = target - predicted;
    bool relock = !wasActive || now - ac.wall > 250000 ||
        (!ac.tempoLatch && (phaseJump || fabs(error) > 250000));
    if (relock) {
        ac.host = target; ac.last = INT64_MIN; ac.edgeTempo = 0; ac.relocks++;
        mTimingDeschedule(&g->timing, &ac.fall); level(&ac, 0);
    } else {
        if (fabs(error) > ac.errorMax) ac.errorMax = fabs(error);
        double correction = error / 240.0;
        if (correction > 100) correction = 100;
        if (correction < -100) correction = -100;
        ac.host = predicted + (ac.tempoLatch ? 0.0 : correction);
    }
    ac.cycles = cycles; ac.wall = now;
    ac.frames++;
    if (ac.frames % 600 == 0) report();
    if (ac.serial) {
        if (!serialReceiver(&ac)) {
            mTimingDeschedule(&g->timing, &ac.rise);
            ac.wanted = ac.serialPlaying = 0; ac.startBeat = NAN;
            ac.last = ac.pending = INT64_MIN;
            return;
        }
        if (!ac.wanted) {
            mTimingDeschedule(&g->timing, &ac.rise);
            if (ac.serialPlaying && serialSend(&ac, 0x03, 0)) { ac.serialPlaying = 0; ac.stops++; }
            ac.last = ac.pending = INT64_MIN;
            return;
        }
        if (relock && ac.serialPlaying) {
            // A long pause or timeline jump stops rather than replaying clocks.
            if (serialSend(&ac, 0x03, 0)) ac.stops++;
            ac.serialPlaying = 0; ac.startBeat = NAN;
        }
        if (!ac.serialPlaying) {
            int64_t host = (int64_t)llround(ac.host);
            double beat = ac_beat_at(&ac.clock, host);
            if (!isfinite(ac.startBeat) || ac.startBeat <= beat)
                ac.startBeat = ac_next_bar(&ac.clock, host + 5000, ac.quantum);
            double delta = (ac.startBeat - beat) * 60000000.0 / ac.clock.tempo;
            int64_t delay = (int64_t)llround(delta * AC_GBA_HZ / 1000000.0);
            mTimingDeschedule(&g->timing, &ac.rise);
            if (delay > 0 && delay <= INT32_MAX) mTimingSchedule(&g->timing, &ac.rise, (int32_t)delay);
        } else if (!mTimingIsScheduled(&g->timing, &ac.rise)) {
            // No catch-up stream if the receiver stopped accepting bytes.
            ac.wanted = 0;
        }
        return;
    }
    if (ac.stepper || ac.fmsClock) {
        int receiver = ac.fmsClock ? g->sio.mode == GBA_SIO_GPIO && !(g->sio.rcnt & 0x10) : g->sio.mode == GBA_SIO_GPIO && (g->sio.rcnt & 0x100) && !(g->sio.rcnt & 0x40);
        if (!receiver || !ac.wanted || (relock && ac.serialPlaying)) {
            mTimingDeschedule(&g->timing, &ac.rise); mTimingDeschedule(&g->timing, &ac.fall);
            if (ac.serialPlaying) ac.stops++;
            ac.serialPlaying = 0; ac.startBeat = NAN; ac.last = ac.pending = INT64_MIN;
            level(&ac, 0);
            if (!receiver || relock) ac.wanted = 0;
            return;
        }
        if (!ac.serialPlaying) {
            int64_t host = (int64_t)llround(ac.host);
            double beat = ac_beat_at(&ac.clock, host);
            if (!isfinite(ac.startBeat) || ac.startBeat <= beat)
                ac.startBeat = ac_next_bar(&ac.clock, host + 5000, ac.quantum);
            ac.pending = (int64_t)llround(ac.startBeat * ac.ppqn);
            ac.scheduledTempo = ac.clock.tempo;
            int64_t delay = (int64_t)llround((ac.startBeat-beat) * 60 * AC_GBA_HZ / ac.clock.tempo);
            mTimingDeschedule(&g->timing, &ac.rise);
            if (delay > 0 && delay <= INT32_MAX) mTimingSchedule(&g->timing, &ac.rise, (int32_t)delay);
            return;
        }
    }
    /* Retain a scheduled edge across ordinary frame/Link updates. Explicit
       tempo changes, reconnects and discontinuities may retime it once. */
    /* In tempo-latch mode even a confirmed tempo change leaves the pending
       edge intact. Its callback applies the new period to the following edge. */
    if (!relock && ac.pending > ac.last &&
        (ac.tempoLatch || ac.scheduledTempo == ac.clock.tempo)) return;
    mTimingDeschedule(&g->timing, &ac.rise);
    ac.scheduledTempo = ac.clock.tempo;
    int64_t delay;
    if (!relock && ac.pending > ac.last) {
        /* Keep the next edge even if a tiny phase adjustment crosses its
           deadline. Never drop a beat at a video-frame boundary. */
        double delta = (ac.pending / (double)ac.ppqn - ac_beat_at(&ac.clock, (int64_t)llround(ac.host))) *
            60000000.0 / ac.clock.tempo;
        delay = (int64_t)llround(delta * AC_GBA_HZ / 1000000.0);
        if (delay < 1) delay = 1;
    } else {
        int64_t host = (int64_t)llround(ac.host);
        int64_t due = ac_next_pulse(&ac.clock, host, ac.ppqn, ac.last, &ac.pending);
        delay = (int64_t)llround((due - host) * AC_GBA_HZ / 1000000.0);
    }
    if (delay > 0 && delay <= INT32_MAX)
        mTimingSchedule(&g->timing, &ac.rise, (int32_t)delay);
}
void AudioCastClockAttach(struct mCore* c) {
    if (c->platform(c) == mPLATFORM_GB) { ACGBClockAttach(c); return; }
    memset(&ac, 0, sizeof(ac)); ac.fd = -1;
    const char* protocol = getenv("AUDIOCAST_LINK_PROTOCOL");
    ac.serial = protocol && !strcmp(protocol, "fms-gba");
    ac.fmsClock = protocol && !strcmp(protocol, "fms-clock");
    ac.stepper = protocol && !strcmp(protocol, "stepper-gba");
    if (protocol && strcmp(protocol, "gba-clock") && !ac.serial && !ac.stepper && !ac.fmsClock) return;
    ac.quantum = 4; ac.startBeat = NAN;
    const char* path = getenv("AUDIOCAST_CLOCK_SOCKET");
    if (!path || c->platform(c) != mPLATFORM_GBA || strlen(path) >= sizeof(ac.socket_path)) return;
    const char* p = getenv("AUDIOCAST_PPQN");
    ac.ppqn = ac.serial ? 24 : p ? atoi(p) : 2;
    if (ac.stepper) {
        if (ac.ppqn != 4 && ac.ppqn != 6 && ac.ppqn != 12 && ac.ppqn != 24 && ac.ppqn != 48 && ac.ppqn != 96) return;
    } else if (ac.fmsClock) {
        if (ac.ppqn != 1 && ac.ppqn != 2 && ac.ppqn != 3 && ac.ppqn != 4 && ac.ppqn != 6 && ac.ppqn != 8) return;
    } else if (!ac.serial && ac.ppqn != 1 && ac.ppqn != 2 && ac.ppqn != 4 && ac.ppqn != 8 && ac.ppqn != 12) return;
    const char* offset = getenv("AUDIOCAST_OFFSET_US");
    char* end = NULL;
    ac.offset = offset ? strtoll(offset, &end, 10) : 0;
    if (offset && (!*offset || !end || *end)) return;
    if (ac.offset < -250000 || ac.offset > 250000) return;
    const char* mode = getenv("AUDIOCAST_CLOCK_MODE");
    ac.tempoLatch = !ac.serial && !ac.stepper && !ac.fmsClock && mode && !strcmp(mode, "tempo-latch");
    ac.fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (ac.fd < 0) return;
    fcntl(ac.fd, F_SETFL, O_NONBLOCK); fcntl(ac.fd, F_SETFD, FD_CLOEXEC);
    struct sockaddr_un addr; memset(&addr, 0, sizeof(addr)); addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, path);
    /* Do not take over another process's socket; session cleanup owns removal. */
    if (bind(ac.fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { close(ac.fd); ac.fd = -1; return; }
    strcpy(ac.socket_path, path); ac.last = INT64_MIN;
    const char* diagnostics = getenv("AUDIOCAST_CLOCK_DIAGNOSTICS");
    ac.diagnostics = diagnostics && !strcmp(diagnostics, "1");
    ac.d.start = serialTransfer; ac.d.writeSIOCNT = serialControl;
    ac.d.handlesMode = handles; ac.d.connectedDevices = connected; ac.d.writeRCNT = writeRCNT;
    ac.rise = (struct mTimingEvent){.context=&ac, .callback=rising, .name="AudioCast SC rise", .priority=0x70};
    ac.fall = (struct mTimingEvent){.context=&ac, .callback=falling, .name="AudioCast SC fall", .priority=0x70};
    c->setPeripheral(c, mPERIPH_GBA_LINK_PORT, &ac.d);
}
void AudioCastClockDetach(struct mCore* c) {
    if (c->platform(c) == mPLATFORM_GB) { ACGBClockDetach(c); return; }
    if (ac.fd < 0) return;
    if (ac.serial && ac.serialPlaying) serialSend(&ac, 0x03, 0);
    report();
    AudioCastClockRebase(c); c->setPeripheral(c, mPERIPH_GBA_LINK_PORT, NULL);
    close(ac.fd); ac.fd = -1; unlink(ac.socket_path);
}
