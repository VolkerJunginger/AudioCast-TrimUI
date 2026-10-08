// AudioCast settings. GPL-2.0-or-later. System SDL2 is loaded at runtime.
#include <dlfcn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <stdexcept>
namespace {
constexpr int W=1024,H=768;
struct Menu {
  int row=0; bool enabled=false,audio=true,available=false; int protocol=0,ppqn=2;
  std::string message;
  const char* protocols[5]={"off","fms-gba","dmgo-gb","gba-clock","stepper-gba"};
  const char* labels[5]={"OFF","FMS / GBA","DMGO / GAME BOY","GBA PULSE","STEPPER / GBA"};
  std::string call(const char* script,const char* action,const char* value=nullptr) {
    int pipefd[2];if(pipe(pipefd))throw std::runtime_error("Settings unavailable");
    pid_t child=fork();
    if(child==0) { close(pipefd[0]);dup2(pipefd[1],1);dup2(pipefd[1],2);close(pipefd[1]);
      execl("/bin/sh","sh",script,action,value,(char*)nullptr);_exit(127); }
    close(pipefd[1]);std::string out;char buf[256];ssize_t n;
    while((n=read(pipefd[0],buf,sizeof(buf)))>0)if(out.size()<1024)out.append(buf,size_t(n));
    close(pipefd[0]);int status=0;
    if(child<0||waitpid(child,&status,0)!=child||!WIFEXITED(status)||WEXITSTATUS(status))
      throw std::runtime_error("CLOSE THE GAME FIRST / SETTING NOT SAVED");
    while(!out.empty()&&(out.back()=='\n'||out.back()=='\r'))out.pop_back();
    return out;
  }
  void load() {
    enabled=access("enabled",F_OK)==0;audio=call("settings.sh","get-audio")!="off";
    auto clock=call("settings.sh","get-clock");protocol=0;
    for(int i=1;i<5;i++)if(clock==protocols[i])protocol=i;
    ppqn=std::atoi(call("settings.sh","get-ppqn").c_str());
    available=access("cores/mgba-link_libretro.so",R_OK)==0&&access("bin/audiocast-core-probe",X_OK)==0;
  }
  void change() {
    try {
      if(row==0)call("control.sh",enabled?"off":"on");
      if(row==1)call("settings.sh","set-audio",audio?"off":"on");
      if(row==2) { if(!available)throw std::runtime_error("INSTALL THE SYNC BUILD TO USE CLOCK SYNC");
        call("settings.sh","set-clock",protocols[(protocol+1)%5]); }
      if(row==3) {
        if(protocol!=4)throw std::runtime_error("PPQ IS FIXED FOR THIS CLOCK MODE");
        const int rates[]={4,6,12,24,48,96};int i=0;
        while(i<6&&rates[i]!=ppqn)i++;
        auto value=std::to_string(rates[(i+1)%6]);call("settings.sh","set-ppqn",value.c_str());
      }
      load();message="SAVED - APPLIES TO THE NEXT GAME";
    }catch(const std::exception& e){message=e.what();}
  }
};
// Original 5x7 bitmap glyphs; no font or theme files are needed.
const char* alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/:.()";
const unsigned char glyph[][5]={
{126,9,9,9,126},{127,73,73,73,54},{62,65,65,65,34},{127,65,65,34,28},{127,73,73,73,65},{127,9,9,9,1},{62,65,73,73,122},{127,8,8,8,127},{65,65,127,65,65},{32,64,65,63,1},{127,8,20,34,65},{127,64,64,64,64},{127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},{127,9,9,9,6},{62,65,81,33,94},{127,9,25,41,70},{38,73,73,73,50},{1,1,127,1,1},{63,64,64,64,63},{31,32,64,32,31},{127,32,24,32,127},{99,20,8,20,99},{3,4,120,4,3},{97,81,73,69,67},
{62,81,73,69,62},{0,66,127,64,0},{98,81,73,73,70},{34,65,73,73,54},{24,20,18,127,16},{39,69,69,69,57},{62,73,73,73,48},{1,113,9,5,3},{54,73,73,73,54},{6,73,73,73,62},{8,8,8,8,8},{64,32,16,8,4},{0,54,54,0,0},{0,96,96,0,0},{0,28,34,65,0},{0,65,34,28,0}};
struct Canvas {
  std::vector<unsigned> pixels=std::vector<unsigned>(W*H,0xff101820);
  void rect(int x,int y,int w,int h,unsigned color) {
    for(int j=y;j<y+h&&j<H;j++)for(int i=x;i<x+w&&i<W;i++)if(i>=0&&j>=0)pixels[j*W+i]=color;
  }
  void text(int x,int y,const std::string& s,int scale,unsigned color) {
    for(unsigned char c:s) { if(c>='a'&&c<='z')c-=32;auto p=strchr(alphabet,c);
      if(p)for(int col=0;col<5;col++)for(int bit=0;bit<7;bit++)if(glyph[p-alphabet][col]&(1<<bit))rect(x+col*scale,y+bit*scale,scale,scale,color);
      x+=6*scale;
    }
  }
  void draw(const Menu& m) {
    rect(0,0,W,H,0xff101820);text(48,45,"LINK4BRICK",7,0xff81e4b3);text(48,118,"SETTINGS",3,0xffa8bac3);
    const std::string rows[4]={std::string("ENABLED: ")+(m.enabled?"ON":"OFF"),std::string("LINK AUDIO: ")+(m.audio?"ON":"OFF"),std::string("CLOCK: ")+m.labels[m.protocol],m.protocol?std::string("PPQ: ")+std::to_string(m.ppqn)+(m.protocol==4?"":" (FIXED)"):"PPQ: --"};
    for(int i=0;i<4;i++) { rect(36,170+i*88,952,72,i==m.row?0xff244a40:0xff1a2830);
      if(i==m.row) { rect(36,170+i*88,8,72,0xff81e4b3); }
      text(60,191+i*88,rows[i],4,i==m.row?0xffeaf8f0:0xffa8bac3); }
    const char* hint=m.row==0?"ENABLE OR RESTORE YOUR NORMAL GAME LAUNCHERS":m.row==1?"OFF: BRICK SPEAKER AND CLOCK KEEP WORKING":m.row==3?(m.protocol==4?"MATCH STEPPER LINK IN (BPQ) TO THIS VALUE":"PPQ IS FIXED FOR THIS CLOCK MODE"):m.protocol==4?"STEPPER: LINK IN - START QUEUES THE ONE":m.protocol==1?"FMS: SYNC IN / GBA - START QUEUES THE ONE":m.protocol==2?"DMGO: SETUP / SYNC: LINK IN":m.protocol==3?"EXTERNAL GPIO CLOCK - 2 PPQN":"CHOOSE THE PROTOCOL USED BY YOUR PROGRAM";
    text(48,536,hint,3,0xffa8bac3);text(48,593,m.message,2,0xff81e4b3);
    text(48,667,"UP/DOWN: SELECT   A: CHANGE   B: BACK",3,0xffeaf8f0);
    text(48,716,"CHANGES APPLY WHEN YOU NEXT OPEN A GAME",2,0xffa8bac3);
  }
  void save(const char* path) {
    std::ofstream f(path,std::ios::binary);f<<"P6\n"<<W<<" "<<H<<"\n255\n";
    for(auto p:pixels){char rgb[3]={char(p>>16),char(p>>8),char(p)};f.write(rgb,3);}
    if(!f)throw std::runtime_error("Cannot write menu preview");
  }
};
struct SDL {
  void* lib=nullptr;
  int (*Init)(unsigned);void (*Quit)();const char* (*GetError)();
  void* (*CreateWindow)(const char*,int,int,int,int,unsigned);
  void* (*CreateRenderer)(void*,int,unsigned);int (*RenderSetLogicalSize)(void*,int,int);
  void* (*CreateTexture)(void*,unsigned,int,int,int);int (*UpdateTexture)(void*,const void*,const void*,int);
  int (*RenderCopy)(void*,void*,const void*,const void*);void (*RenderPresent)(void*);
  void (*DestroyTexture)(void*);void (*DestroyRenderer)(void*);void (*DestroyWindow)(void*);
  void (*PumpEvents)();const unsigned char* (*GetKeyboardState)(int*);
  int (*NumJoysticks)();int (*IsGameController)(int);void* (*GameControllerOpen)(int);
  unsigned char (*GameControllerGetButton)(void*,int);void (*GameControllerClose)(void*);
  void* (*RWFromFile)(const char*,const char*);
  int (*GameControllerAddMappingsFromRW)(void*,int);
  template<class T>void sym(T& f,const char* name){f=reinterpret_cast<T>(dlsym(lib,name));if(!f)throw std::runtime_error(name);}
  SDL(){for(auto path:{"libSDL2-2.0.so.0","libSDL2.so","/usr/trimui/lib/libSDL2-2.0.so.0","libSDL2.dylib"}){lib=dlopen(path,RTLD_NOW|RTLD_LOCAL);if(lib)break;}
    if(!lib)throw std::runtime_error("System SDL2 unavailable");
#define AC_SDL(name) sym(name,"SDL_" #name)
    AC_SDL(Init);AC_SDL(Quit);AC_SDL(GetError);AC_SDL(CreateWindow);AC_SDL(CreateRenderer);AC_SDL(RenderSetLogicalSize);AC_SDL(CreateTexture);AC_SDL(UpdateTexture);AC_SDL(RenderCopy);AC_SDL(RenderPresent);AC_SDL(DestroyTexture);AC_SDL(DestroyRenderer);AC_SDL(DestroyWindow);AC_SDL(PumpEvents);AC_SDL(GetKeyboardState);AC_SDL(NumJoysticks);AC_SDL(IsGameController);AC_SDL(GameControllerOpen);AC_SDL(GameControllerGetButton);AC_SDL(GameControllerClose);
#undef AC_SDL
    sym(RWFromFile,"SDL_RWFromFile");sym(GameControllerAddMappingsFromRW,"SDL_GameControllerAddMappingsFromRW");
  }
  ~SDL(){if(lib)dlclose(lib);}
};
}
int main(int argc,char** argv) {
  try { Menu m;m.load();Canvas canvas;
    if(argc>=3&&!strcmp(argv[1],"--render")){canvas.draw(m);canvas.save(argv[2]);return 0;}
    if(argc>=3&&!strcmp(argv[1],"--change")){m.row=std::atoi(argv[2]);if(m.row<0||m.row>3)return 2;m.change();std::puts(m.message.c_str());return m.message.find("SAVED")==0?0:1;}
    SDL s;if(s.Init(0x20|0x2000))throw std::runtime_error(s.GetError());
    auto window=s.CreateWindow("LINK4BRICK",0x2fff0000,0x2fff0000,W,H,0x1005);if(!window)throw std::runtime_error(s.GetError());
    auto renderer=s.CreateRenderer(window,-1,2|4);if(!renderer)renderer=s.CreateRenderer(window,-1,1);if(!renderer)throw std::runtime_error(s.GetError());
    s.RenderSetLogicalSize(renderer,W,H);auto texture=s.CreateTexture(renderer,372645892,1,W,H);if(!texture)throw std::runtime_error(s.GetError());
    if(auto mappings=s.RWFromFile("/usr/trimui/gamecontrollerdb.txt","rb"))s.GameControllerAddMappingsFromRW(mappings,1);
    void* controller=nullptr;for(int i=0;i<s.NumJoysticks();i++)if(s.IsGameController(i)){controller=s.GameControllerOpen(i);break;}
    unsigned previous=0;bool done=false;
    while(!done) { s.PumpEvents();int n=0;auto keys=s.GetKeyboardState(&n);
      auto key=[&](int k){return k<n&&keys[k];};auto button=[&](int b){return controller&&s.GameControllerGetButton(controller,b);};
      unsigned current=(key(82)||button(11)?1:0)|(key(81)||button(12)?2:0)|(key(4)||key(40)||button(1)?4:0)|(key(5)||key(41)||button(0)?8:0);
      unsigned pressed=current&~previous;previous=current;
      if(pressed&1){m.row=(m.row+3)%4;m.message.clear();}if(pressed&2){m.row=(m.row+1)%4;m.message.clear();}
      if(pressed&4) { m.change(); }
      if(pressed&8) { done=true; }
      canvas.draw(m);s.UpdateTexture(texture,nullptr,canvas.pixels.data(),W*4);s.RenderCopy(renderer,texture,nullptr,nullptr);s.RenderPresent(renderer);
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    if(controller) { s.GameControllerClose(controller); }
    s.DestroyTexture(texture);s.DestroyRenderer(renderer);s.DestroyWindow(window);s.Quit();return 0;
  }catch(const std::exception& e){std::fprintf(stderr,"LINK4BRICK settings: %s\n",e.what());return 1;}
}
