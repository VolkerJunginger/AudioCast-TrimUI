// StockUI-only sender. PCM arrives on stdin from the supervised FIFO relay.
// Separate from the legacy sender: no 33025-Hz workaround or shared rate file.
#include <ableton/LinkAudio.hpp>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <cerrno>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <chrono>
static volatile sig_atomic_t running = 1;
static void stop(int) { running = 0; }
int main() {
  signal(SIGINT, stop);
  signal(SIGTERM, stop);
  signal(SIGPIPE, SIG_IGN);
  setvbuf(stdout, nullptr, _IOLBF, 0);
  ableton::LinkAudio link(120.0, "TrimUI Brick Hammer");
  link.setNumPeersCallback([](std::size_t n) { std::printf("link peers changed: %zu\n", n); });
  link.enable(true);
  link.enableLinkAudio(true);
  ableton::LinkAudioSink sink(link, "Brick Out", 512);
  std::printf("AudioCast v0.2b: 48000Hz stereo S16_LE; Link transport 48000Hz\n");
  int16_t samples[512];
  size_t filled = 0;
  uint64_t received=0, committed=0, unavailable=0, rejected=0;
  unsigned peak=0;
  auto lastReport = std::chrono::steady_clock::now();
  auto lastInput = lastReport;
  std::chrono::microseconds origin{0};
  uint64_t timelineFrames=0;
  bool timeline=false;
  auto report = [&]() {
    std::printf("stats: fifo_buffers=%llu committed=%llu no_buffer=%llu commit_rejected=%llu peak=%u\n",
      (unsigned long long)received, (unsigned long long)committed,
      (unsigned long long)unavailable, (unsigned long long)rejected, peak);
    peak=0;
  };
  while (running) {
    pollfd p{STDIN_FILENO, POLLIN, 0};
    int ready = poll(&p, 1, 100);
    auto now = std::chrono::steady_clock::now();
    if (now-lastReport >= std::chrono::seconds(2)) { report(); lastReport=now; }
    if (ready < 0) { if (errno == EINTR) continue; break; }
    if (!ready) continue;
    ssize_t n = read(STDIN_FILENO, reinterpret_cast<char*>(samples)+filled, sizeof(samples)-filled);
    if (!n) break;
    if (n < 0) { if (errno == EINTR || errno == EAGAIN) continue; break; }
    filled += size_t(n);
    if (filled != sizeof(samples)) continue;
    filled=0;
    ++received;
    for (int16_t s : samples) {
      unsigned magnitude = s < 0 ? unsigned(-int(s)) : unsigned(s);
      if (magnitude > peak) peak=magnitude;
    }
    if (!timeline || now-lastInput > std::chrono::milliseconds(250)) {
      origin=link.clock().micros(); timelineFrames=0; timeline=true;
    }
    lastInput=now;
    auto begin=origin+std::chrono::microseconds(timelineFrames*1000000ULL/48000);
    timelineFrames+=256;
    ableton::LinkAudioSink::BufferHandle buffer(sink);
    if (!buffer) { ++unavailable; timeline=false; continue; }
    std::memcpy(buffer.samples, samples, sizeof(samples));
    auto state=link.captureAudioSessionState();
    if (buffer.commit(state, state.beatAtTime(begin, 4.0), 4.0, 256, 2, 48000))
      ++committed;
    else { ++rejected; timeline=false; }
  }
  report();
  std::printf("sender exit: partial_bytes=%zu (not a remote-delivery acknowledgement)\n", filled);
  link.enableLinkAudio(false);
  link.enable(false);
  return 0;
}
