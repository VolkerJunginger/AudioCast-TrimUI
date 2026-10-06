// Test the private core's loader/API before selecting it for a normal game.
// No game is launched, no device/core is patched, and no log file is written.
#include <dlfcn.h>
#include <cstdio>
#include <cstring>
struct CoreInfo { const char* name; const char* version; const char* extensions; bool fullpath; bool block_extract; };
int main(int argc,char** argv) {
  if(argc!=2) return 2;
  void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
  if(!lib) { std::fprintf(stderr,"Virtual cable core unavailable: %s\n",dlerror());return 3; }
  const char* functions[]={"retro_init","retro_deinit","retro_set_environment","retro_set_video_refresh",
    "retro_set_audio_sample","retro_set_audio_sample_batch","retro_set_input_poll","retro_set_input_state",
    "retro_get_system_info","retro_get_system_av_info","retro_set_controller_port_device","retro_reset",
    "retro_run","retro_serialize_size","retro_serialize","retro_unserialize","retro_cheat_reset",
    "retro_cheat_set","retro_load_game","retro_load_game_special","retro_unload_game","retro_get_region",
    "retro_get_memory_data","retro_get_memory_size","retro_api_version","AudioCastClockAttach","AudioCastClockReady"};
  for(const char* f:functions)if(!dlsym(lib,f)){std::fprintf(stderr,"Missing virtual cable API: %s\n",f);dlclose(lib);return 4;}
  auto api=reinterpret_cast<unsigned(*)()>(dlsym(lib,"retro_api_version"));
  if(api()!=1){std::fprintf(stderr,"Unsupported libretro API\n");dlclose(lib);return 5;}
  auto getInfo=reinterpret_cast<void(*)(CoreInfo*)>(dlsym(lib,"retro_get_system_info"));
  CoreInfo info{};getInfo(&info);
  if(!info.name || std::strcmp(info.name,"mGBA") || !info.extensions || !std::strstr(info.extensions,"gba")){
    std::fprintf(stderr,"Unsupported virtual cable core\n");dlclose(lib);return 6;
  }
  std::printf("Virtual cable core load/API check passed: %s %s\n",info.name,info.version?info.version:"");
  dlclose(lib);return 0;
}
