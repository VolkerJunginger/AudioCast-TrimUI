// FIFO arrival is bursty and may pause for less than the long-gap timeout.
// Sample counting alone then leaves live audio stamped increasingly in the past.
#pragma once
#include <cstdint>

struct PcmTimeline {
  explicit PcmTimeline(bool bounded,uint32_t blockFrames=256)
    : bounded(bounded),blockFrames(blockFrames) {}
  bool bounded, active=false;
  uint32_t blockFrames;
  int64_t origin=0, lastInput=0, lagMin=0, lagMax=0, gapMax=0;
  uint64_t frames=0, recoveries=0, longGaps=0;
  static constexpr int64_t kMaxLagUs=64000;
  static constexpr int64_t kLongGapUs=250000;
  int64_t next(int64_t now) {
    const auto gap=active ? now-lastInput : 0;
    if (gap>gapMax) gapMax=gap;
    if (!active || gap>kLongGapUs) {
      if (active) ++longGaps;
      origin=now;frames=0;active=true;
    }
    auto begin=origin+static_cast<int64_t>(frames*1000000ULL/48000);
    const auto lag=now-begin;
    if (lag<lagMin) lagMin=lag;
    if (lag>lagMax) lagMax=lag;
    if (bounded && lag>kMaxLagUs) {
      // Recover the timestamp of this live buffer. Do not replay old time or
      // alter the PCM, Link peer, channel identity, tempo or virtual cable.
      origin=now;frames=0;begin=now;++recoveries;
    }
    frames+=blockFrames;lastInput=now;
    return begin;
  }
  void invalidate() { active=false; }
  void reported() { lagMin=lagMax=gapMax=0; }
};
