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
static bool environment(unsigned cmd,void* data) {
  switch(cmd) {
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
  CHECK(argc==2);void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
  if(!lib){fprintf(stderr,"Loader failure: %s\n",dlerror());return 1;}
  LOAD(retro_set_environment);LOAD(retro_set_video_refresh);LOAD(retro_set_audio_sample_batch);
  LOAD(retro_set_audio_sample);LOAD(retro_set_input_poll);LOAD(retro_set_input_state);
  LOAD(retro_init);LOAD(retro_load_game);LOAD(retro_run);LOAD(retro_reset);LOAD(retro_unload_game);LOAD(retro_deinit);
  char directory[80],socket_path[108],marker[120];
  snprintf(directory,sizeof(directory),"/tmp/ac-libretro-cable-%ld",(long)getpid());CHECK(!mkdir(directory,0700));
  snprintf(socket_path,sizeof(socket_path),"%s/clock.sock",directory);snprintf(marker,sizeof(marker),"%s.ready",socket_path);
  CHECK(!setenv("AUDIOCAST_CLOCK_SOCKET",socket_path,1));CHECK(!setenv("AUDIOCAST_LINK_PROTOCOL","gba-clock",1));
  CHECK(!setenv("AUDIOCAST_PPQN","2",1));
  uint8_t rom[512]={0};uint32_t branch=0xeafffffe;memcpy(rom,&branch,4);
  /* Minimal recognition signature; no copyrighted logo image or game code. */
  rom[4]=0x24;rom[5]=0xff;rom[6]=0xae;rom[7]=0x51;rom[0xb2]=0x96;
  retro_set_environment_fn(environment);retro_set_video_refresh_fn(video);retro_set_audio_sample_batch_fn(audio);
  retro_set_audio_sample_fn(sample);retro_set_input_poll_fn(poll_input);retro_set_input_state_fn(input);retro_init_fn();
  struct retro_game_info game={"generated-loop.gba",rom,sizeof(rom),NULL};CHECK(retro_load_game_fn(&game));
  CHECK(!access(socket_path,F_OK));CHECK(access(marker,F_OK)!=0);
  for(unsigned i=0;i<8;i++)retro_run_fn();
  CHECK(frames==8 && audio_frames>0);CHECK(!access(marker,F_OK));
  FILE* f=fopen(marker,"rb");CHECK(f && fgetc(f)=='1' && fgetc(f)==EOF);fclose(f);
  retro_reset_fn();retro_run_fn();CHECK(frames==9);
  retro_unload_game_fn();CHECK(access(socket_path,F_OK)!=0);
  retro_deinit_fn();dlclose(lib);CHECK(!unlink(marker));CHECK(!rmdir(directory));
  printf("PASS: actual libretro load/run/audio/reset/unload, %u frames, first-frame handshake and socket cleanup\n",frames);
  return 0;
}
