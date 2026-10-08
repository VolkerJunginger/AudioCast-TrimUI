/* Generated GPIO interrupt receiver, no music ROM. GPL-2.0-or-later. */
#include "../sync/clock.h"
#include "../sync/mgba-clock.h"
#include <mgba/core/core.h>
#include <mgba/gba/core.h>
#include <mgba/core/log.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/io.h>
#include <mgba/internal/gba/sio.h>
#include <mgba-util/vfs.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned ticks;
static uint32_t first,last;
static double minimum,maximum;
static struct mTimingEvent sample;
static void receive(struct mTiming* t,void* ctx,uint32_t late) {
    struct GBA* g=ctx;
    if(g->memory.io[GBA_REG(IF)] & (1<<GBA_IRQ_SIO)) {
        uint32_t cycle=(uint32_t)mTimingCurrentTime(t)-late;
        /* SI is active low at the actual interrupt; unrelated pins stay zero. */
        assert(!(g->sio.rcnt&15));
        if(!ticks)first=cycle;
        if(last){double interval=(uint32_t)(cycle-last)*1000000.0/16777216;
            if(!minimum||interval<minimum)minimum=interval;
            if(interval>maximum)maximum=interval;}
        ticks++;last=cycle;
        GBAIOWrite(g,GBA_REG_IF,1<<GBA_IRQ_SIO);
    }
    mTimingSchedule(t,&sample,late<335?335-late:1); /* 20 us sampler */
}
static void quiet(struct mLogger*l,int cat,enum mLogLevel lev,const char*f,va_list a){(void)l;(void)cat;(void)lev;(void)f;(void)a;}
static void start(struct mCore* c){assert(AudioCastClockInput(c,9)==9);assert(AudioCastClockInput(c,9)==9);assert(AudioCastClockInput(c,0)==0);}
int main(void) {
    setbuf(stdout,NULL);
    struct mLogger log={.log=quiet};mLogSetDefaultLogger(&log);
    struct mCore*c=GBACoreCreate();assert(c->init(c));mCoreInitConfig(c,NULL);
    mColor*video=calloc(240*160,sizeof(mColor));c->setVideoBuffer(c,video,240);
    uint32_t rom[128]={0xeafffffe};assert(c->loadROM(c,VFileMemChunk(rom,sizeof(rom))));c->reset(c);c->runFrame(c);
    struct GBA*g=c->board;sample=(struct mTimingEvent){.context=g,.callback=receive,.name="SI IRQ receiver",.priority=0x80};
    mTimingSchedule(&g->timing,&sample,335);
    char path[90];snprintf(path,sizeof(path),"/tmp/ac-stepper-%ld.sock",(long)getpid());
    setenv("AUDIOCAST_CLOCK_SOCKET",path,1);setenv("AUDIOCAST_LINK_PROTOCOL","stepper-gba",1);setenv("AUDIOCAST_OFFSET_US","0",1);
    int fd=socket(AF_UNIX,SOCK_DGRAM,0);assert(fd>=0);struct sockaddr_un addr={.sun_family=AF_UNIX};strcpy(addr.sun_path,path);
    int rates[]={4,6,12,24,48,96};const double frameUs=280896.0/16777216*1000000;
    for(unsigned r=0;r<sizeof(rates)/sizeof(rates[0]);r++) {
        char rate[8];snprintf(rate,sizeof(rate),"%d",rates[r]);setenv("AUDIOCAST_PPQN",rate,1);AudioCastClockAttach(c);
        GBASIOWriteRCNT(&g->sio,0x8100);assert(g->sio.rcnt==0x8104);
        ticks=0;first=last=0;minimum=maximum=0;
        uint32_t base=(uint32_t)mTimingCurrentTime(&g->timing);
        struct ACClockSnapshot s={AC_CLOCK_MAGIC,AC_CLOCK_VERSION,1000000,0,120,1,0};
        for(int f=0;f<720;f++) {
            int64_t now=1000000+(int64_t)llround(f*frameUs);s.monotonic_us=now;s.beat=(now-1000000)*120/60000000.0;
            if(f==4){assert(AudioCastClockInput(c,12)==12);AudioCastClockInput(c,0);} /* settings shortcut */
            if(f==10)start(c);
            if(f==119)assert(ticks==0);
            assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));AudioCastClockFrameAt(c,now);c->runFrame(c);
        }
        unsigned expected=(unsigned)floor((720*frameUs-2000000)*rates[r]*120/60000000.0)+1;
        assert(ticks==expected);
        assert(fabs((uint32_t)(first-base)*1000000.0/16777216-2000000)<30);
        double period=60000000.0/(120*rates[r]);assert(minimum>period-40&&maximum<period+40);
        printf("%d PPQ / 120 BPM: %u SI IRQs; next-one start; interval %.1f..%.1f us\n",rates[r],ticks,minimum,maximum);
        unsigned before=ticks;start(c);AudioCastClockFrameAt(c,s.monotonic_us+17000);c->runFrame(c);assert(ticks==before&&(g->sio.rcnt&4));
        start(c);start(c); /* cancel queued start */
        for(int f=0;f<130;f++){s.monotonic_us+=16743;s.beat+=16743*120/60000000.0;assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));AudioCastClockFrameAt(c,s.monotonic_us);c->runFrame(c);}assert(ticks==before);
        start(c);
        for(int f=0;f<200;f++){s.monotonic_us+=16743;s.beat+=16743*120/60000000.0;assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));AudioCastClockFrameAt(c,s.monotonic_us);c->runFrame(c);}assert(ticks>before);
        /* Tempo change with frame jitter; edge count keeps increasing, no restart. */
        for(int f=0;f<(rates[r]==96?15000:900);f++){
            s.monotonic_us+=16743;s.beat+=16743*400/60000000.0;s.tempo=400;
            if(f==30){minimum=maximum=0;last=0;}
            assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));AudioCastClockFrameAt(c,s.monotonic_us+(f%2?6000:0));c->runFrame(c);
        }
        period=60000000.0/(400*rates[r]);assert(minimum>period*.97-40&&maximum<period*1.03+40);
        printf("%d PPQ / 400 BPM: interval %.1f..%.1f us\n",rates[r],minimum,maximum);
        /* GPIO output directions must never be forced by the peripheral. */
        before=ticks;GBASIOWriteRCNT(&g->sio,0x8140);AudioCastClockFrameAt(c,s.monotonic_us+17000);c->runFrame(c);assert(ticks==before&&!(g->sio.rcnt&4));
        GBASIOWriteRCNT(&g->sio,0x8100);start(c);
        for(int f=0;f<80;f++){s.monotonic_us+=16743;s.beat+=16743*400/60000000.0;assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));AudioCastClockFrameAt(c,s.monotonic_us);c->runFrame(c);}
        before=ticks;s.peers=0;s.monotonic_us+=17000;assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));AudioCastClockFrameAt(c,s.monotonic_us);c->runFrame(c);assert(ticks==before&&(g->sio.rcnt&4));
        s.peers=1;s.monotonic_us+=17000;assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));AudioCastClockFrameAt(c,s.monotonic_us);c->runFrame(c);assert(ticks==before);
        start(c);for(int f=0;f<80;f++){s.monotonic_us+=16743;s.beat+=16743*400/60000000.0;assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));AudioCastClockFrameAt(c,s.monotonic_us);c->runFrame(c);}
        before=ticks;AudioCastClockFrameAt(c,s.monotonic_us+1000000);c->runFrame(c);assert(ticks==before&&(g->sio.rcnt&4));
        AudioCastClockDetach(c);assert(access(path,F_OK)!=0);
    }
    mTimingDeschedule(&g->timing,&sample);close(fd);mCoreConfigDeinit(&c->config);c->deinit(c);free(video);
    puts("PASS: all STEPPER input rates, SI falling-edge IRQ, idle high, sub-frame pulses, queued start, native controls, cancellation, tempo changes, output isolation, stale/rejoin and cleanup");
}
