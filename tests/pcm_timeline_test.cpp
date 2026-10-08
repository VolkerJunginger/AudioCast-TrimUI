#include "../src/pcm_timeline.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

int main() {
  // Exact-rate frame batches remain sample-contiguous with no recovery.
  PcmTimeline steady(true);int64_t origin=1000000;
  for (uint64_t n=0;n<200000;++n) {
    const auto begin=origin+static_cast<int64_t>(n*256*1000000ULL/48000);
    const auto arrival=origin+static_cast<int64_t>((n/6)*6*256*1000000ULL/48000);
    assert(steady.next(arrival)==begin);
  }
  assert(steady.recoveries==0 && steady.longGaps==0);
  // Repeated short stalls are not caught by the legacy >250 ms gap reset.
  // Both intermittent scheduling stalls and a slightly slower PCM source
  // eventually make that unbounded sample-count timeline stale.
  for (const bool stalls : {false,true}) {
    PcmTimeline bounded(true),legacy(false);int64_t lost=0,last=-1;
    for (uint64_t n=0;n<200000;++n) {
      if (stalls && n%180==0) lost+=90000;
      const auto now=origin+static_cast<int64_t>(n*256*(stalls?1000000ULL:1005000ULL)/48000)+lost;
      const auto begin=bounded.next(now);legacy.next(now);
      assert(now-begin<=PcmTimeline::kMaxLagUs && begin>last);last=begin;
    }
    assert(bounded.recoveries>0 && bounded.longGaps==0);
    assert(legacy.lagMax>2000000 && legacy.recoveries==0 && legacy.longGaps==0);
  }
  // Preserve the previous behavior for long pauses and disconnected sinks.
  PcmTimeline paused(true);assert(paused.next(origin)==origin);
  assert(paused.next(origin+500000)==origin+500000 && paused.longGaps==1);
  paused.invalidate();assert(paused.next(origin+510000)==origin+510000);
  std::puts("PASS: exact PCM continuity, bounded age under drift/repeated short stalls, legacy regression, long-pause/disconnect recovery");
}
