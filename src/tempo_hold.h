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

// Event-driven tempo latch. Link phase is used only on connection. An unchanged
// tempo never reanchors the local oscillator; a change must settle for one second.
struct TempoLatch {
  static constexpr int64_t kCheckUs=500000, kListenUs=100000, kWindowUs=1000000;
  static constexpr int64_t kHeartbeatUs=100000;
  static constexpr double kEpsilon=0.000001;
  ACClockSnapshot anchor{};
  bool initialized=false,connected=false,listening=false;
  int64_t lastCheck=0,lastPublish=0,candidateSince=0;
  double candidate=0;
  uint64_t checks=0,latches=0,windows=0,snapshots=0;
  bool checkDue(int64_t now,uint32_t peers) const {
    return !initialized || (peers>0)!=connected ||
      now-lastCheck >= (listening ? kListenUs : kCheckUs);
  }
  bool publishDue(int64_t now,uint32_t peers) const {
    return !initialized || (peers>0)!=connected || now-lastPublish>=kHeartbeatUs;
  }
  void observe(int64_t now,double linkBeat,double tempo,uint32_t peers,uint32_t playing) {
    lastCheck=now;++checks;
    if(!initialized || (peers>0)!=connected) {
      anchor={AC_CLOCK_MAGIC,AC_CLOCK_VERSION,now,linkBeat,tempo,peers,playing};
      initialized=true;connected=peers>0;listening=false;
      if(connected)++latches;
      return;
    }
    anchor.peers=peers;anchor.playing=playing;
    if(!connected)return;
    if(fabs(tempo-anchor.tempo)<=kEpsilon) {listening=false;return;}
    if(!listening || fabs(tempo-candidate)>kEpsilon) {
      if(!listening)++windows;
      listening=true;candidate=tempo;candidateSince=now;
    } else if(now-candidateSince>=kWindowUs) {
      const double beat=ac_beat_at(&anchor,now);
      anchor={AC_CLOCK_MAGIC,AC_CLOCK_VERSION,now,beat,candidate,peers,playing};
      listening=false;++latches;
    }
  }
  ACClockSnapshot publish(int64_t now,uint32_t peers) {
    auto result=anchor;
    result.beat=ac_beat_at(&anchor,now);result.monotonic_us=now;result.peers=peers;
    lastPublish=now;++snapshots;
    return result;
  }
};
