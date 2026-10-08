/* Generated ARM-loop cartridge; no game ROM or BIOS is included. GPL-2.0-or-later. */
#include "libretro.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
static unsigned frames;
static size_t audio_frames;
static unsigned av_reopens;
static bool environment(unsigned cmd,void* data) {
  switch(cmd) {
  case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO: av_reopens++; return true;
  case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: *(const char**)data="/tmp";return true;
  case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: *(bool*)data=false;return true;
  case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION: *(unsigned*)data=2;return true;
  case RETRO_ENVIRONMENT_GET_LANGUAGE: *(unsigned*)data=RETRO_LANGUAGE_ENGLISH;return true;
  case RETRO_ENVIRONMENT_GET_VARIABLE: {
    struct retro_variable* v=data;
    if(!strcmp(v->key,"mgba_use_bios")){v->value="OFF";return true;}
    if(!strcmp(v->key,"mgba_skip_bios")){v->value="ON";return true;}
    return false;
  }
  case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: return *(unsigned*)data==RETRO_PIXEL_FORMAT_RGB565;
  case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
  case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:
  case RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS:
  case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL:
  case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2:return true;
  default:return false;
  }
}
static void video(const void* p,unsigned w,unsigned h,size_t stride){(void)p;(void)w;(void)h;(void)stride;frames++;}
static size_t audio(const int16_t* p,size_t n){(void)p;audio_frames+=n;return n;}
static void sample(int16_t l,int16_t r){(void)l;(void)r;audio_frames++;}
static void poll_input(void){}
static int16_t input(unsigned p,unsigned d,unsigned i,unsigned id){(void)p;(void)d;(void)i;(void)id;return 0;}
#define CHECK(c) do {if(!(c)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c);return 1;}}while(0)
#define LOAD(n) __typeof__(&n) n##_fn=dlsym(lib,#n);CHECK(n##_fn)
int main(int argc,char** argv) {
  CHECK(argc==2 || argc==3);int gb=argc==3;void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
  if(!lib){fprintf(stderr,"Loader failure: %s\n",dlerror());return 1;}
  LOAD(retro_set_environment);LOAD(retro_set_video_refresh);LOAD(retro_set_audio_sample_batch);
  LOAD(retro_set_audio_sample);LOAD(retro_set_input_poll);LOAD(retro_set_input_state);
  LOAD(retro_get_system_av_info);LOAD(retro_init);LOAD(retro_load_game);LOAD(retro_run);LOAD(retro_reset);LOAD(retro_unload_game);LOAD(retro_deinit);
  char directory[80],socket_path[108],marker[120];
  snprintf(directory,sizeof(directory),"/tmp/ac-libretro-cable-%ld",(long)getpid());CHECK(!mkdir(directory,0700));
  snprintf(socket_path,sizeof(socket_path),"%s/clock.sock",directory);snprintf(marker,sizeof(marker),"%s.ready",socket_path);
  CHECK(!setenv("AUDIOCAST_CLOCK_SOCKET",socket_path,1));CHECK(!setenv("AUDIOCAST_LINK_PROTOCOL",gb?"dmgo-gb":"gba-clock",1));
  CHECK(!setenv("AUDIOCAST_PPQN",gb?"16":"2",1));
  uint8_t rom[512]={0};uint32_t branch=0xeafffffe;memcpy(rom,&branch,4);
  /* Minimal recognition signature; no copyrighted logo image or game code. */
  /* Branch past the recognition header; write SOUNDBIAS=65536 Hz at runtime.
     An AV callback here used to reopen ALSA after the first initialization. */
  branch=0xea00002e;memcpy(rom,&branch,4);
  const uint32_t program[]={0xe59f0008,0xe59f1008,0xe1c010b0,0xeafffffe,0x04000088,0x4200};
  memcpy(rom+0xc0,program,sizeof(program));
  rom[4]=0x24;rom[5]=0xff;rom[6]=0xae;rom[7]=0x51;rom[0xb2]=0x96;
  retro_set_environment_fn(environment);retro_set_video_refresh_fn(video);retro_set_audio_sample_batch_fn(audio);
  retro_set_audio_sample_fn(sample);retro_set_input_poll_fn(poll_input);retro_set_input_state_fn(input);retro_init_fn();
  uint8_t gbrom[32768]={0};
  /* Recognition via GBX footer, not a copyrighted cartridge logo. */
  gbrom[0x100]=0xc3;gbrom[0x101]=0x50;gbrom[0x102]=1;
  gbrom[0x150]=0xc3;gbrom[0x151]=0x50;gbrom[0x152]=1;
  uint8_t* footer=gbrom+sizeof(gbrom)-16;
  footer[3]=0x40;footer[7]=1;memcpy(footer+12,"GBX!",4);
  struct retro_game_info game={gb?"generated-loop.gb":"generated-loop.gba",gb?(void*)gbrom:(void*)rom,gb?sizeof(gbrom):sizeof(rom),NULL};CHECK(retro_load_game_fn(&game));
  struct retro_system_av_info info;retro_get_system_av_info_fn(&info);
  CHECK(info.timing.sample_rate==48000 && av_reopens==0);
  CHECK(!access(socket_path,F_OK));CHECK(access(marker,F_OK)!=0);
  for(unsigned i=0;i<8;i++)retro_run_fn();
  CHECK(frames==8 && audio_frames>5000 && av_reopens==0);CHECK(!access(marker,F_OK));
  FILE* f=fopen(marker,"rb");CHECK(f && fgetc(f)=='1' && fgetc(f)==EOF);fclose(f);
  retro_reset_fn();retro_run_fn();CHECK(frames==9 && av_reopens==0);
  retro_unload_game_fn();CHECK(access(socket_path,F_OK)!=0);
  retro_deinit_fn();dlclose(lib);CHECK(!unlink(marker));CHECK(!rmdir(directory));
  printf("PASS: actual libretro load/run/audio/reset/unload, %u frames, first-frame handshake and socket cleanup\n",frames);
  return 0;
}
