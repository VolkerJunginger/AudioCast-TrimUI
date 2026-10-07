// Opt-in local oscillator: align phase initially, then sample Link tempo at 2 s.
#pragma once
#include "../sync/clock.h"

struct TempoHold {
  static constexpr int64_t kRefreshUs=2000000;
  static constexpr int64_t kHeartbeatUs=100000;
  ACClockSnapshot anchor{};
  bool initialized=false,connected=false;
  int64_t lastSample=0,lastPublish=0;
  uint64_t samples=0,snapshots=0;
  bool publishDue(int64_t now,uint32_t peers) const {
    return !initialized || (peers>0)!=connected || now-lastPublish>=kHeartbeatUs;
  }
  bool sampleDue(int64_t now,uint32_t peers) const {
    return !initialized || (peers>0)!=connected || now-lastSample>=kRefreshUs;
  }
  void sample(int64_t now,double linkBeat,double tempo,uint32_t peers,uint32_t playing) {
    // Keep the running phase continuous on tempo changes. Rejoining a session
    // aligns once to Link again; missing peers never keep the cable running.
    const double beat=initialized && connected && peers ? ac_beat_at(&anchor,now) : linkBeat;
    anchor={AC_CLOCK_MAGIC,AC_CLOCK_VERSION,now,beat,tempo,peers,playing};
    initialized=true;connected=peers>0;lastSample=now;++samples;
  }
  ACClockSnapshot publish(int64_t now,uint32_t peers) {
    auto result=anchor;
    result.beat=ac_beat_at(&anchor,now);result.monotonic_us=now;result.peers=peers;
    lastPublish=now;++snapshots;
    return result;
  }
};
