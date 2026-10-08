/* Generated GB loop and serial receiver; no game ROM. GPL-2.0-or-later. */
#include "../sync/clock.h"
#include "../sync/mgba-clock.h"
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/gb/core.h>
#include <mgba/internal/gb/gb.h>
#include <mgba/internal/gb/sio.h>
#include <mgba/internal/gb/io.h>
#include <mgba-util/vfs.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
static struct mTimingEvent sample;
static unsigned ticks;static uint32_t first,last;static double low,high;
static void sampling(struct mTiming* t,void* ctx,uint32_t late){
    struct GB* g=ctx;
    if(!(g->memory.io[GB_REG_SC]&0x80)){
        assert(g->memory.io[GB_REG_SB]==0xf8);assert(g->memory.io[GB_REG_IF]&(1<<GB_IRQ_SIO));
        uint32_t cycle=(uint32_t)mTimingCurrentTime(t)-late;
        if(!ticks)first=cycle;
        if(last){double interval=(uint32_t)(cycle-last)*1000000.0/8388608;
            if(!low||interval<low)low=interval;if(interval>high)high=interval;}
        last=cycle;ticks++;g->memory.io[GB_REG_SC]=0x80;GBSIOWriteSC(&g->sio,0x80);
    }
    mTimingSchedule(t,&sample,838-(int32_t)(late<838?late:837));
}
static void quiet(struct mLogger*l,int c,enum mLogLevel v,const char*f,va_list a){(void)l;(void)c;(void)v;(void)f;(void)a;}
static void press(struct mCore*c){assert(AudioCastClockInput(c,9)==9);assert(AudioCastClockInput(c,9)==9);AudioCastClockInput(c,0);}
int main(void){
    struct mLogger log={.log=quiet};mLogSetDefaultLogger(&log);struct mCore*c=GBCoreCreate();assert(c->init(c));mCoreInitConfig(c,NULL);
    mColor* video=calloc(160*144,sizeof(mColor));c->setVideoBuffer(c,video,160);
    uint8_t rom[32768]={0};rom[0x100]=0xc3;rom[0x101]=0;rom[0x102]=1;
    assert(c->loadROM(c,VFileMemChunk(rom,sizeof(rom))));c->reset(c);c->runFrame(c);
    struct GB*g=c->board;char path[90];snprintf(path,sizeof(path),"/tmp/ac-gb-%ld.sock",(long)getpid());
    setenv("AUDIOCAST_LINK_PROTOCOL","dmgo-gb",1);setenv("AUDIOCAST_CLOCK_SOCKET",path,1);AudioCastClockAttach(c);
    g->memory.io[GB_REG_SC]=0x80;GBSIOWriteSC(&g->sio,0x80);
    sample=(struct mTimingEvent){.context=g,.callback=sampling,.name="Generated GB receiver",.priority=0x80};mTimingSchedule(&g->timing,&sample,838);
    int fd=socket(AF_UNIX,SOCK_DGRAM,0);assert(fd>=0);struct sockaddr_un addr={.sun_family=AF_UNIX};strcpy(addr.sun_path,path);
    struct ACClockSnapshot s={AC_CLOCK_MAGIC,AC_CLOCK_VERSION,1000000,0,120,1,0};
    uint32_t base=(uint32_t)mTimingCurrentTime(&g->timing);double frameUs=70224.0/4194304*1000000;
    for(int f=0;f<700;f++){
        int64_t now=1000000+(int64_t)llround(f*frameUs);
        s.monotonic_us=now;s.beat=(now-1000000)*120/60000000.0;
        if(f>=500){int64_t change=1000000+(int64_t)llround(500*frameUs);s.tempo=150;
            s.beat=(change-1000000)*120/60000000.0+(now-change)*150/60000000.0;}
        if(f==10)press(c);
        if(f==118)assert(!ticks);
        if(f==121){assert(ticks==1);double start=(uint32_t)(first-base)*1000000.0/8388608;
            printf("GB beat-4 first byte at %.1f us (serial shift takes ~1 ms)\n",start);assert(fabs(start-2001000)<250);}
        if(f==490){assert(ticks>195&&low>30900&&high<31600);low=high=0;last=0;}
        if(f==530){low=high=0;last=0;}
        assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));AudioCastClockFrameAt(c,now);c->runFrame(c);
    }
    printf("GB 150 BPM / 16 PPQN: %u F8 bytes, %.1f..%.1f us\n",ticks,low,high);assert(low>24400&&high<25600);
    unsigned before=ticks;press(c);AudioCastClockFrameAt(c,s.monotonic_us+17000);c->runFrame(c);assert(ticks==before);
    press(c);press(c);AudioCastClockFrameAt(c,s.monotonic_us+34000);c->runFrame(c);assert(ticks==before);
    press(c);s.monotonic_us+=50000;s.peers=0;assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));AudioCastClockFrameAt(c,s.monotonic_us);c->runFrame(c);assert(ticks==before);
    AudioCastClockDetach(c);assert(access(path,F_OK));close(fd);mTimingDeschedule(&g->timing,&sample);mCoreConfigDeinit(&c->config);c->deinit(c);free(video);
    puts("PASS: actual GB serial shifts/IRQ, bar queue, held START, stop/cancel, tempo change, peer loss and cleanup");
}
