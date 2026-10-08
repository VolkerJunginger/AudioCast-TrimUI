/* DMGo-compatible Game Boy serial clock. MPL-2.0.
 * Included by mgba-clock.c; independent of the validated GBA driver.
 * No ROM identification, memory patch, MIDI port or multiplayer emulation. */
#include <mgba/internal/gb/gb.h>
#include <mgba/internal/gb/sio.h>
#include <mgba/internal/gb/io.h>
struct ACGBDriver {
    struct GBSIODriver d;
    struct mCore* core;
    struct mTimingEvent tick;
    struct ACClockSnapshot clock;
    int fd, active, wanted, playing, startKey, diagnostics;
    int64_t wall, last;
    double host, startBeat, hz;
    uint32_t cycles, edgeCycle;
    unsigned ticks, missed, starts, stops;
    double edgeTempo, intervalMin, intervalMax;
    char path[sizeof(((struct sockaddr_un*)0)->sun_path)];
};
static struct ACGBDriver acgb = {.fd=-1};
static int gbReceiver(void) {
    return acgb.d.p && !(acgb.d.p->p->memory.io[GB_REG_SC]&1);
}
static void gbSB(struct GBSIODriver* d,uint8_t value){(void)d;(void)value;}
static uint8_t gbSC(struct GBSIODriver* d,uint8_t value){(void)d;return value;}
/* A byte takes eight genuine serial shift events, then mGBA raises its normal
   serial interrupt. Never complete an internal-clock transfer or bypass ROM IO. */
static int gbSend(uint8_t message) {
    struct GBSIO* sio=acgb.d.p;
    if(!sio || (sio->p->memory.io[GB_REG_SC]&0x81)!=0x80 || mTimingIsScheduled(&sio->p->timing,&sio->event))return 0;
    sio->pendingSB=message;sio->remainingBits=8;sio->period=512;
    mTimingSchedule(&sio->p->timing,&sio->event,sio->period*(2-sio->p->doubleSpeed));return 1;
}
static void gbTick(struct mTiming* t,void* ctx,uint32_t late) {
    struct ACGBDriver* a=ctx;
    if(!a->active || !a->wanted || !gbReceiver())return;
    if(!gbSend(0xf8)){a->missed++;a->playing=0;a->startBeat=NAN;return;}
    uint32_t cycle=(uint32_t)mTimingCurrentTime(t)-late;
    if(!a->playing){a->playing=1;a->starts++;a->last=(int64_t)llround(a->startBeat*16);
        if(a->diagnostics)fprintf(stderr,"AUDIOCAST_GB START beat=%.6f quantum=4 ppqn=16\n",a->startBeat);
    }else a->last++;
    if(a->ticks && a->edgeTempo==a->clock.tempo){
        double interval=(uint32_t)(cycle-a->edgeCycle)*1000000.0/a->hz;
        if(!a->intervalMin||interval<a->intervalMin)a->intervalMin=interval;
        if(interval>a->intervalMax)a->intervalMax=interval;
    }
    a->ticks++;a->edgeCycle=cycle;a->edgeTempo=a->clock.tempo;
    int64_t host=(int64_t)llround(a->host+((uint32_t)mTimingCurrentTime(t)-a->cycles)*1000000.0/a->hz);
    double period=60000000.0/(a->clock.tempo*16);
    double error=((a->last+1)/16.0-ac_beat_at(&a->clock,host))*60000000.0/a->clock.tempo-period;
    double correction=error/16.0,limit=period*.02;
    if(correction>limit)correction=limit;if(correction<-limit)correction=-limit;
    int64_t delay=(int64_t)llround((period+correction)*a->hz/1000000.0)-late;
    if(delay<1)delay=1;if(delay<=INT32_MAX)mTimingSchedule(t,&a->tick,(int32_t)delay);
}
static uint16_t ACGBClockInput(struct mCore* c,uint16_t keys) {
    if(acgb.fd<0 || c!=acgb.core)return keys;
    int pressed=!!(keys&8);
    if(gbReceiver() && (acgb.d.p->p->memory.io[GB_REG_SC]&0x80) && pressed && !acgb.startKey){acgb.wanted=!acgb.wanted;acgb.startBeat=NAN;
        if(!acgb.wanted){acgb.playing=0;acgb.stops++;mTimingDeschedule(&acgb.d.p->p->timing,&acgb.tick);}
        if(acgb.diagnostics)fprintf(stderr,"AUDIOCAST_GB request=%s\n",acgb.wanted?"queue-next-one":"stop");
    }
    acgb.startKey=pressed;
    // DMGo's own START toggles its local transport; it waits for the first F8.
    // Preserve normal controls, including START for INTERNAL / LINK OUT modes.
    return keys;
}
static void ACGBClockRebase(struct mCore* c) {
    if(acgb.fd<0 || c!=acgb.core)return;
    mTimingDeschedule(&acgb.d.p->p->timing,&acgb.tick);
    mTimingDeschedule(&acgb.d.p->p->timing,&acgb.d.p->event);
    acgb.wanted=acgb.playing=acgb.startKey=acgb.active=0;acgb.startBeat=NAN;
}
static void ACGBClockFrameAt(struct mCore* c,int64_t now) {
    if(acgb.fd<0 || c!=acgb.core)return;
    struct GB* g=c->board;struct ACClockSnapshot s;int jump=0;
    for(int i=0;i<64;i++){char packet[sizeof(s)+1];ssize_t n=recv(acgb.fd,packet,sizeof(packet),0);
        if(n<0)break;if(n!=sizeof(s))continue;memcpy(&s,packet,sizeof(s));
        if(!ac_clock_valid(&s,now)||s.monotonic_us<acgb.clock.monotonic_us)continue;
        if(acgb.active&&fabs(s.beat-ac_beat_at(&acgb.clock,s.monotonic_us))>.25)jump=1;acgb.clock=s;
    }
    int was=acgb.active;acgb.active=ac_clock_valid(&acgb.clock,now)&&acgb.clock.peers>0;
    if(!acgb.active){mTimingDeschedule(&g->timing,&acgb.tick);if(was){acgb.playing=0;acgb.startBeat=NAN;}return;}
    uint32_t cycles=(uint32_t)mTimingCurrentTime(&g->timing);
    double predicted=acgb.host+(uint32_t)(cycles-acgb.cycles)*1000000.0/acgb.hz,error=now-predicted;
    int relock=!was||now-acgb.wall>250000||jump||fabs(error)>250000;
    if(relock){acgb.host=now;mTimingDeschedule(&g->timing,&acgb.tick);
        // Pause and requeue the next bar. This protocol has no serial STOP;
        // the ROM retains its local transport until the user presses START.
        if(was){acgb.playing=0;acgb.startBeat=NAN;}
    }else {double correction=error/240.0;if(correction>100)correction=100;if(correction<-100)correction=-100;acgb.host=predicted+correction;}
    acgb.cycles=cycles;acgb.wall=now;
    if(!gbReceiver()){mTimingDeschedule(&g->timing,&acgb.tick);acgb.wanted=acgb.playing=0;acgb.startBeat=NAN;return;}
    if(!acgb.wanted)return;
    if(!acgb.playing){int64_t host=(int64_t)llround(acgb.host);double beat=ac_beat_at(&acgb.clock,host);
        if(!isfinite(acgb.startBeat)||acgb.startBeat<=beat)acgb.startBeat=ac_next_bar(&acgb.clock,host+5000,4);
        int64_t delay=(int64_t)llround((acgb.startBeat-beat)*60000000.0/acgb.clock.tempo*acgb.hz/1000000.0);
        mTimingDeschedule(&g->timing,&acgb.tick);if(delay>0&&delay<=INT32_MAX)mTimingSchedule(&g->timing,&acgb.tick,(int32_t)delay);
    }else if(!mTimingIsScheduled(&g->timing,&acgb.tick)){acgb.wanted=acgb.playing=0;}
}
static void ACGBClockAttach(struct mCore* c) {
    memset(&acgb,0,sizeof(acgb));acgb.fd=-1;
    const char* protocol=getenv("AUDIOCAST_LINK_PROTOCOL"),*path=getenv("AUDIOCAST_CLOCK_SOCKET");
    if(!protocol||strcmp(protocol,"dmgo-gb")||c->platform(c)!=mPLATFORM_GB||!path||strlen(path)>=sizeof(acgb.path))return;
    acgb.fd=socket(AF_UNIX,SOCK_DGRAM,0);if(acgb.fd<0)return;
    fcntl(acgb.fd,F_SETFL,O_NONBLOCK);fcntl(acgb.fd,F_SETFD,FD_CLOEXEC);
    struct sockaddr_un addr;memset(&addr,0,sizeof(addr));addr.sun_family=AF_UNIX;strcpy(addr.sun_path,path);
    if(bind(acgb.fd,(struct sockaddr*)&addr,sizeof(addr))<0){close(acgb.fd);acgb.fd=-1;return;}
    strcpy(acgb.path,path);acgb.core=c;acgb.hz=c->timingFrequency(c);acgb.startBeat=NAN;
    const char* diagnostic=getenv("AUDIOCAST_CLOCK_DIAGNOSTICS");acgb.diagnostics=diagnostic&&!strcmp(diagnostic,"1");
    acgb.d.writeSB=gbSB;acgb.d.writeSC=gbSC;GBSIOSetDriver(&((struct GB*)c->board)->sio,&acgb.d);
    acgb.tick=(struct mTimingEvent){.context=&acgb,.callback=gbTick,.name="AudioCast GB serial tick",.priority=0x70};
}
static void ACGBClockDetach(struct mCore* c) {
    if(acgb.fd<0||c!=acgb.core)return;
    if(acgb.diagnostics)fprintf(stderr,"AUDIOCAST_GB ticks=%u starts=%u stops=%u missed=%u interval_us=%.1f..%.1f scheduler=bar-gb-serial-16\n",acgb.ticks,acgb.starts,acgb.stops,acgb.missed,acgb.intervalMin,acgb.intervalMax);
    ACGBClockRebase(c);GBSIOSetDriver(&((struct GB*)c->board)->sio,NULL);close(acgb.fd);acgb.fd=-1;unlink(acgb.path);
}
