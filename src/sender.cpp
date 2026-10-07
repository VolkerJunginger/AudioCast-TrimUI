// StockUI-only sender. PCM arrives on stdin from the supervised FIFO relay.
// Separate from the legacy sender: no 33025-Hz workaround or shared rate file.
#include <ableton/LinkAudio.hpp>
#include <unistd.h>
#include "../sync/clock.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <cstdlib>
#include <poll.h>
#include <signal.h>
#include <cerrno>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <chrono>
#include "pcm_timeline.h"
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
  int clockFd = -1;
  sockaddr_un clockAddress{};
  const char* clockPath = std::getenv("AUDIOCAST_CLOCK_SOCKET");
  if (clockPath && std::strlen(clockPath) < sizeof(clockAddress.sun_path)) {
    clockFd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (clockFd >= 0) {
      fcntl(clockFd, F_SETFL, O_NONBLOCK);
      fcntl(clockFd, F_SETFD, FD_CLOEXEC);
      clockAddress.sun_family = AF_UNIX;
      std::strcpy(clockAddress.sun_path, clockPath);
    }
  }
  std::printf("AudioCast v0.2b: 48000Hz stereo S16_LE; Link transport 48000Hz\n");
  int16_t samples[512];
  size_t filled = 0;
  uint64_t received=0, committed=0, unavailable=0, rejected=0;
  unsigned peak=0;
  auto lastReport = std::chrono::steady_clock::now();
  const char* recoverySetting=std::getenv("AUDIOCAST_AUDIO_RECOVERY");
  const char* diagnosticSetting=std::getenv("AUDIOCAST_AUDIO_DIAGNOSTICS");
  const bool recovery=recoverySetting && std::strcmp(recoverySetting,"1")==0;
  // Align recovery commits with the pinned Link encoder's complete PCM packet.
  // Its resizer otherwise extends cached sample time across a timestamp reset.
  constexpr size_t recoveryFrames=125;
  static_assert((576-ableton::link_audio::v1::kHeaderSize
    -ableton::link_audio::AudioBuffer::kNonAudioBytes)/(2*sizeof(int16_t))==recoveryFrames,
    "Recheck PCM packet alignment when updating Link");
  const size_t blockFrames=recovery ? recoveryFrames : 256;
  const size_t inputBytes=blockFrames*2*sizeof(int16_t);
  PcmTimeline timeline(recovery,static_cast<uint32_t>(blockFrames));
  const bool timingDiagnostics=diagnosticSetting && std::strcmp(diagnosticSetting,"1")==0;
  auto report = [&]() {
    std::printf("stats: fifo_buffers=%llu committed=%llu no_buffer=%llu commit_rejected=%llu peak=%u\n",
      (unsigned long long)received, (unsigned long long)committed,
      (unsigned long long)unavailable, (unsigned long long)rejected, peak);
    peak=0;
    if (timingDiagnostics) {
      std::printf("audio_timing: policy=%s block_frames=%zu lag_min_us=%lld lag_max_us=%lld input_gap_max_us=%lld recoveries=%llu long_gaps=%llu peers=%zu\n",
        timeline.bounded ? "bounded" : "legacy",blockFrames,
        (long long)timeline.lagMin,(long long)timeline.lagMax,(long long)timeline.gapMax,
        (unsigned long long)timeline.recoveries,(unsigned long long)timeline.longGaps,link.numPeers());
      timeline.reported();
    }
  };
  while (running) {
    if (clockFd >= 0) {
      auto state = link.captureAppSessionState();
      auto linkNow = link.clock().micros();
      ACClockSnapshot snapshot{AC_CLOCK_MAGIC, AC_CLOCK_VERSION,
        ac_monotonic_us(), state.beatAtTime(linkNow, 4.0), state.tempo(),
        static_cast<uint32_t>(link.numPeers()), state.isPlaying() ? 1U : 0U};
      // Optional datagrams never block or determine whether audio is sent.
      (void)sendto(clockFd, &snapshot, sizeof(snapshot), 0,
        reinterpret_cast<const sockaddr*>(&clockAddress), sizeof(clockAddress));
    }
    pollfd p{STDIN_FILENO, POLLIN, 0};
    int ready = poll(&p, 1, 100);
    auto now = std::chrono::steady_clock::now();
    if (now-lastReport >= std::chrono::seconds(2)) { report(); lastReport=now; }
    if (ready < 0) { if (errno == EINTR) continue; break; }
    if (!ready) continue;
    ssize_t n = read(STDIN_FILENO, reinterpret_cast<char*>(samples)+filled, inputBytes-filled);
    if (!n) break;
    if (n < 0) { if (errno == EINTR || errno == EAGAIN) continue; break; }
    filled += size_t(n);
    if (filled != inputBytes) continue;
    filled=0;
    ++received;
    for (size_t i=0;i<blockFrames*2;++i) {
      int16_t s=samples[i];
      unsigned magnitude = s < 0 ? unsigned(-int(s)) : unsigned(s);
      if (magnitude > peak) peak=magnitude;
    }
    auto begin=std::chrono::microseconds(timeline.next(link.clock().micros().count()));
    ableton::LinkAudioSink::BufferHandle buffer(sink);
    if (!buffer) { ++unavailable; timeline.invalidate(); continue; }
    std::memcpy(buffer.samples, samples, inputBytes);
    auto state=link.captureAudioSessionState();
    if (buffer.commit(state, state.beatAtTime(begin, 4.0), 4.0, blockFrames, 2, 48000))
      ++committed;
    else { ++rejected; timeline.invalidate(); }
  }
  report();
  std::printf("sender exit: partial_bytes=%zu (not a remote-delivery acknowledgement)\n", filled);
  if (clockFd >= 0) close(clockFd);
  link.enableLinkAudio(false);
  link.enable(false);
  return 0;
}
