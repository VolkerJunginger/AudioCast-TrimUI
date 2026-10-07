/* Public synthetic GBA clock test: contains no commercial ROM data. GPL-2.0-or-later. */
#include "../sync/clock.h"
#include "../sync/mgba-clock.h"
#include <mgba/core/core.h>
#include <mgba/gba/core.h>
#include <mgba/core/log.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/sio.h>
#include <mgba-util/vfs.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
static unsigned edges;
static int high;
static int sampleCycles = 16777;
static uint32_t lastEdge;
static double minInterval, maxInterval;
static unsigned intervals;
static struct mTimingEvent sample;
static void sampling(struct mTiming* t,void* ctx,uint32_t late) {
    struct GBA* g=ctx;
    int v=g->sio.rcnt&1;
    if(v&&!high) {
        uint32_t cycle=(uint32_t)mTimingCurrentTime(t)-late;
        if(lastEdge) {
            double interval=(uint32_t)(cycle-lastEdge)*1000000.0/16777216;
            if(!intervals || interval<minInterval)minInterval=interval;
            if(!intervals || interval>maxInterval)maxInterval=interval;
            intervals++;
        }
        lastEdge=cycle;edges++;
    }
    high=v;
    mTimingSchedule(t,&sample,sampleCycles-(int32_t)late);
}
static void quiet(struct mLogger*l,int cat,enum mLogLevel lev,const char*f,va_list a){(void)l;(void)cat;(void)lev;(void)f;(void)a;}
int main(void) {
    unsetenv("AUDIOCAST_CLOCK_MODE");
    struct mLogger log={.log=quiet};mLogSetDefaultLogger(&log);
    struct mCore* c=GBACoreCreate();assert(c->init(c));mCoreInitConfig(c,NULL);
    mColor* video=calloc(240*160,sizeof(mColor));c->setVideoBuffer(c,video,240);
    uint32_t rom[128]={0xeafffffe}; /* ARM infinite loop, generated here. */
    assert(c->loadROM(c,VFileMemChunk(rom,sizeof(rom))));c->reset(c);
    c->runFrame(c);c->runFrame(c);
    struct GBA* g=c->board;GBASIOWriteRCNT(&g->sio,0x8000);
    sample=(struct mTimingEvent){.context=g,.callback=sampling,.name="SC test sampler",.priority=0x80};
    mTimingSchedule(&g->timing,&sample,16777);
    char path[90];snprintf(path,sizeof(path),"/tmp/ac-core-test-%ld.sock",(long)getpid());
    setenv("AUDIOCAST_CLOCK_SOCKET",path,1);setenv("AUDIOCAST_PPQN","12",1);AudioCastClockAttach(c);
    int fd=socket(AF_UNIX,SOCK_DGRAM,0);assert(fd>=0);
    struct sockaddr_un addr={.sun_family=AF_UNIX};strcpy(addr.sun_path,path);
    struct ACClockSnapshot s={AC_CLOCK_MAGIC,AC_CLOCK_VERSION,1000000,0,400,1,0};
    const int frame_count=18000; /* > 2^32 CPU cycles: test timing-counter wrap. */
    for(int f=0;f<frame_count;f++) {
        int64_t now=1000000+(int64_t)llround(f*280896.0/16777216*1000000);
        s.monotonic_us=now;s.beat=(now-1000000)*400/60000000.0;
        assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));
        AudioCastClockFrameAt(c,now);c->runFrame(c);
    }
    unsigned expected=(unsigned)floor(frame_count*280896.0/16777216*400/60*12);
    printf("12 PPQN / 400 BPM: %u rising edges, expected %u\n",edges,expected);
    assert(edges==expected);
    unsigned before=edges;AudioCastClockFrameAt(c,s.monotonic_us+1000000);c->runFrame(c);assert(edges==before && !(g->sio.rcnt&1));
    AudioCastClockRebase(c);
    mTimingDeschedule(&g->timing,&sample);sampleCycles=1677;
    mTimingSchedule(&g->timing,&sample,sampleCycles);
    lastEdge=0;intervals=0;minInterval=maxInterval=0;edges=0;
    AudioCastClockDetach(c);setenv("AUDIOCAST_PPQN","2",1);AudioCastClockAttach(c);
    int64_t epoch=s.monotonic_us+2000000;
    s.tempo=120;
    for(int f=0;f<3000;f++) {
        /* Scheduling varies by +/-6 ms; snapshots may be 100 ms old.
           Also run the frontend 200 ppm off nominal to check drift correction. */
        int64_t now=epoch+(int64_t)llround(f*280896.0/16777216*1000000*1.0002)+(f%2?6000:0);
        s.monotonic_us=now-(f%7)*16000;
        s.beat=(s.monotonic_us-epoch)*120/60000000.0;
        assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));
        AudioCastClockFrameAt(c,now);c->runFrame(c);
    }
    printf("Jittered frames / 120 BPM / 2 PPQN: %u edges, interval %.1f..%.1f us\n",edges,minInterval,maxInterval);
    assert(edges>=200 && edges<=202);
    assert(intervals>190 && minInterval>249800 && maxInterval<250200);
    /* Small Link phase corrections must not move a pending pulse. Exercise
       both correction directions with much larger video scheduling jitter. */
    epoch=s.monotonic_us+200000;
    double startBeat=ac_beat_at(&s,epoch);
    lastEdge=0;intervals=0;minInterval=maxInterval=0;
    for(int f=0;f<3000;f++) {
        int64_t now=epoch+(int64_t)llround(f*280896.0/16777216*1000000)+(f%2?15000:0);
        s.monotonic_us=now;
        s.beat=startBeat+(now-epoch)*120/60000000.0+(f/300%2?0.04:-0.04);
        assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));
        AudioCastClockFrameAt(c,now);c->runFrame(c);
    }
    printf("Phase corrections / bursty frames: interval %.1f..%.1f us\n",minInterval,maxInterval);
    assert(intervals>190 && minInterval>249800 && maxInterval<250200);
    /* A deliberate tempo change must still arrive immediately. Exclude the
       single transition interval; the following intervals must match 150 BPM. */
    epoch=s.monotonic_us+17000;startBeat=ac_beat_at(&s,epoch);s.tempo=150;
    before=edges;
    for(int f=0;f<1800;f++) {
        int64_t now=epoch+(int64_t)llround(f*280896.0/16777216*1000000);
        s.monotonic_us=now;s.beat=startBeat+(now-epoch)*150/60000000.0;
        if(f==30){lastEdge=0;intervals=0;minInterval=maxInterval=0;}
        assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));
        AudioCastClockFrameAt(c,now);c->runFrame(c);
    }
    printf("Tempo change to 150 BPM: %u edges, interval %.1f..%.1f us\n",edges-before,minInterval,maxInterval);
    assert(edges-before>=149 && edges-before<=152);
    assert(intervals>140 && minInterval>199800 && maxInterval<200200);
    AudioCastClockDetach(c);setenv("AUDIOCAST_CLOCK_MODE","tempo-latch",1);AudioCastClockAttach(c);
    lastEdge=0;intervals=0;minInterval=maxInterval=0;
    epoch=s.monotonic_us+1000000;s.tempo=120;
    for(int f=0;f<18000;f++) {
        int64_t now=epoch+(int64_t)llround(f*280896.0/16777216*1000000*1.0002)+(f%2?15000:0);
        s.monotonic_us=now;
        // Phase steps exceed the old relock threshold, and frontend drift
        // accumulates. Neither may disturb the tempo-only pulse oscillator.
        s.beat=(now-epoch)*120/60000000.0+(f/300%2?0.4:-0.4);
        assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));
        AudioCastClockFrameAt(c,now);c->runFrame(c);
    }
    printf("Tempo latch / five minutes / Link phase steps: interval %.1f..%.1f us\n",minInterval,maxInterval);
    assert(intervals>1100 && minInterval>249900 && maxInterval<250100);
    epoch=s.monotonic_us+17000;startBeat=ac_beat_at(&s,epoch);s.tempo=150;
    for(int f=0;f<1800;f++) {
        int64_t now=epoch+(int64_t)llround(f*280896.0/16777216*1000000);
        s.monotonic_us=now;s.beat=startBeat+(now-epoch)*150/60000000.0;
        if(f==30){lastEdge=0;intervals=0;minInterval=maxInterval=0;}
        assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));
        AudioCastClockFrameAt(c,now);c->runFrame(c);
    }
    printf("Tempo latch / confirmed 150 BPM: interval %.1f..%.1f us\n",minInterval,maxInterval);
    assert(intervals>140 && minInterval>199900 && maxInterval<200100);
    before=edges;s.peers=0;s.monotonic_us+=17000;
    assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));
    AudioCastClockFrameAt(c,s.monotonic_us);c->runFrame(c);assert(edges==before);
    AudioCastClockDetach(c);assert(access(path,F_OK)!=0);close(fd);
    mCoreConfigDeinit(&c->config);c->deinit(c);free(video);
    puts("PASS: CPU-cycle pulses at maximum supported rate, 32-bit timing wrap, 1 ms polling, uneven frames, aged snapshots, drift, phase corrections, tempo changes, tempo-only oscillator, peer loss, stale feed and cleanup");
}
