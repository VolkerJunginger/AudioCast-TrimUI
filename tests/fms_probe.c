/* Private, optional compatibility test. No FMS ROM or save is distributed.
 * Checks FMS 1.31 using its normal controls; runtime addresses are test-only.
 * GPL-2.0-or-later. */
#include "../sync/clock.h"
#include "../sync/mgba-clock.h"
#include <mgba/core/core.h>
#include <mgba/gba/core.h>
#include <mgba/core/log.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/sio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
static void quiet(struct mLogger* l,int cat,enum mLogLevel level,const char* f,va_list a) {
    (void)l;(void)cat;(void)level;(void)f;(void)a;
}
static void frames(struct mCore* c,int n,int keys) {
    c->setKeys(c,keys);while(n--)c->runFrame(c);
}
static void tap(struct mCore* c,int keys) {frames(c,2,keys);frames(c,10,0);}
static unsigned ticks(struct GBA* g) {return *(uint32_t*)((char*)g->memory.iwram+0x6d40);}
int main(int argc,char** argv) {
    if(argc!=2){fprintf(stderr,"Usage: fms-probe /private/path/to/fms.gba (FMS 1.31)\n");return 2;}
    struct mLogger log={.log=quiet};mLogSetDefaultLogger(&log);
    struct mCore* c=GBACoreCreate();assert(c->init(c));mCoreInitConfig(c,NULL);
    mColor* video=calloc(240*160,sizeof(mColor));c->setVideoBuffer(c,video,240);
    assert(mCoreLoadFile(c,argv[1]));c->reset(c);struct GBA* g=c->board;
    frames(c,180,0);tap(c,8); /* Boot and start. */
    tap(c,4);tap(c,2); /* Top menu -> Data. */
    for(int i=0;i<5;i++)tap(c,0x80); /* Sync direction. */
    tap(c,0x42); /* B+Up: OUT -> IN. */
    tap(c,0x10);tap(c,0x42); /* Protocol: GBA -> CLOCK (default PPQ2). */
    assert(((unsigned char*)g->memory.iwram)[0x769f]==2);
    assert(((unsigned char*)g->memory.iwram)[0x74e9]==1);
    assert(g->sio.rcnt==0x8000 && ticks(g)==0);
    frames(c,180,0);assert(ticks(g)==0);
    char path[90];snprintf(path,sizeof(path),"/tmp/ac-fms-test-%ld.sock",(long)getpid());
    setenv("AUDIOCAST_CLOCK_SOCKET",path,1);setenv("AUDIOCAST_PPQN","2",1);
    AudioCastClockAttach(c);
    int fd=socket(AF_UNIX,SOCK_DGRAM,0);assert(fd>=0);
    struct sockaddr_un addr={.sun_family=AF_UNIX};strcpy(addr.sun_path,path);
    int64_t epoch=1000000,now=epoch;double beat=0;
    struct ACClockSnapshot s={AC_CLOCK_MAGIC,AC_CLOCK_VERSION,epoch,0,120,1,0};
    for(int bpm=60;bpm<=240;bpm+=60) {
        unsigned before=ticks(g);double initial=beat;
        for(int f=0;f<300;f++) {
            s.monotonic_us=now;s.beat=beat;s.tempo=bpm;
            assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));
            AudioCastClockFrameAt(c,now);c->runFrame(c);
            beat+=280896.0/16777216*bpm/60;
            now=epoch+(int64_t)llround((f+1)*280896.0/16777216*1000000);
        }
        unsigned pulseCount=(ticks(g)-before)/12;
        unsigned expected=(unsigned)floor(beat*2)-(unsigned)floor(initial*2);
        printf("%d BPM: %u pulses, expected %u\n",bpm,pulseCount,expected);
        assert(pulseCount==expected);
        epoch=now;
    }
    unsigned before=ticks(g);
    // Stale feed and peer loss: no new clock edges and SC returns low.
    AudioCastClockFrameAt(c,now+1000000);frames(c,60,0);assert(ticks(g)==before && !(g->sio.rcnt&1));
    now+=2000000;s.monotonic_us=now;s.beat=beat;s.peers=0;
    assert(sendto(fd,&s,sizeof(s),0,(struct sockaddr*)&addr,sizeof(addr))==sizeof(s));
    AudioCastClockFrameAt(c,now);frames(c,60,0);assert(ticks(g)==before);
    AudioCastClockDetach(c);assert(access(path,F_OK)!=0);close(fd);
    mCoreConfigDeinit(&c->config);c->deinit(c);free(video);
    puts("PASS: FMS CLOCK IN, PPQ2, 60/120/180/240 BPM, no-clock hold, stale/peer-loss stop, socket cleanup");
}
