// Load the firmware's ALSA, never a bundled replacement. Stable public ALSA ABI:
// opaque snd_pcm_t, snd_pcm_sframes_t=long, snd_pcm_uframes_t=unsigned long.
#include <dlfcn.h>
#include <unistd.h>
#include <signal.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <cstring>
struct _snd_pcm;
using PCM = _snd_pcm;
template<class T> T symbol(void* lib, const char* name) {
  void* p = dlsym(lib, name);
  if (!p) { std::fprintf(stderr, "Missing ALSA symbol: %s\n", name); std::exit(2); }
  return reinterpret_cast<T>(p);
}
int main(int argc, char** argv) {
  if (argc != 3 || (std::strcmp(argv[2], "0") && std::strcmp(argv[2], "1") && std::strcmp(argv[2], "6"))) {
    std::fprintf(stderr, "usage: alsa-probe PCM {0|1|6}; 0 checks setup without a tone\n"); return 2;
  }
  setvbuf(stdout, nullptr, _IONBF, 0);
  // Covers dlopen, FIFO open, writes and drain; even a blocked plugin is bounded.
  signal(SIGALRM, SIG_DFL);
  alarm(12);
  void* lib = dlopen("libasound.so.2", RTLD_NOW | RTLD_LOCAL);
  if (!lib) { std::fprintf(stderr, "Firmware ALSA unavailable: %s\n", dlerror()); return 2; }
  auto version = symbol<const char*(*)()>(lib, "snd_asoundlib_version");
  auto open = symbol<int(*)(PCM**,const char*,int,int)>(lib, "snd_pcm_open");
  auto params = symbol<int(*)(PCM*,int,int,unsigned,unsigned,int,unsigned)>(lib, "snd_pcm_set_params");
  auto format = symbol<int(*)(const char*)>(lib, "snd_pcm_format_value");
  auto write = symbol<long(*)(PCM*,const void*,unsigned long)>(lib, "snd_pcm_writei");
  auto drain = symbol<int(*)(PCM*)>(lib, "snd_pcm_drain");
  auto close = symbol<int(*)(PCM*)>(lib, "snd_pcm_close");
  auto error = symbol<const char*(*)(int)>(lib, "snd_strerror");
  std::printf("ALSA=%s PCM=%s format=S16_LE channels=2 rate=48000\n", version(), argv[1]);
  PCM* pcm = nullptr;
  int rc = open(&pcm, argv[1], 0 /* playback */, 0);
  if (rc < 0) { std::fprintf(stderr, "open: %s\n", error(rc)); return 3; }
  // RW_INTERLEAVED=3; soft resampling disabled so capability is tested exactly.
  rc = params(pcm, format("S16_LE"), 3, 2, 48000, 0, 100000);
  if (rc < 0) { std::fprintf(stderr, "params: %s\n", error(rc)); close(pcm); return 4; }
  const unsigned long total = 48000UL * std::strtoul(argv[2], nullptr, 10);
  unsigned long done = 0;
  int16_t samples[512];
  while (done < total) {
    unsigned long frames = total - done < 256 ? total - done : 256;
    for (unsigned long i=0; i<frames; ++i) {
      double t = double(done+i)/48000.0;
      // Quiet, smoothly faded 440 Hz left / 660 Hz right test signal.
      double fade = std::fmin(1.0, std::fmin(double(done+i)/480.0, double(total-done-i)/480.0));
      samples[2*i] = int16_t(2048.0*fade*std::sin(6.283185307179586*440.0*t));
      samples[2*i+1] = int16_t(2048.0*fade*std::sin(6.283185307179586*660.0*t));
    }
    unsigned long offset = 0;
    while (offset < frames) {
      long n = write(pcm, samples+2*offset, frames-offset);
      // Fail on XRUN instead of claiming an uninterrupted duplication test.
      if (n <= 0) { std::fprintf(stderr, "write: %s\n", error(int(n))); close(pcm); return 5; }
      offset += static_cast<unsigned long>(n);
    }
    done += frames;
  }
  rc = drain(pcm);
  int closeRc = close(pcm);
  if (rc < 0 || closeRc < 0) { std::fprintf(stderr, "drain/close failed\n"); return 6; }
  std::printf("PLAYBACK_OK frames=%lu bytes=%lu\n", done, done*4);
  alarm(0);
  dlclose(lib);
  return 0;
}
