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
static struct mTimingEvent sample;
static void sampling(struct mTiming* t,void* ctx,uint32_t late) {
    struct GBA* g=ctx;
    int v=g->sio.rcnt&1;if(v&&!high)edges++;high=v;
    mTimingSchedule(t,&sample,16777-(int32_t)late);
}
static void quiet(struct mLogger*l,int cat,enum mLogLevel lev,const char*f,va_list a){(void)l;(void)cat;(void)lev;(void)f;(void)a;}
int main(void) {
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
    AudioCastClockRebase(c);AudioCastClockDetach(c);assert(access(path,F_OK)!=0);close(fd);
    mCoreConfigDeinit(&c->config);c->deinit(c);free(video);
    puts("PASS: CPU-cycle pulses at maximum supported rate, 32-bit timing wrap, 1 ms polling, stale feed and cleanup");
}
