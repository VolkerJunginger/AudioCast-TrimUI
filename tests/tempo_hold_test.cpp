#include "../src/tempo_hold.h"
#include <cassert>
#include <cmath>
#include <cstdio>
int main() {
  TempoHold local;const int64_t start=1000000;
  assert(local.publishDue(start,1) && local.sampleDue(start,1));
  local.sample(start,3.25,120,1,0);
  auto s=local.publish(start,1);assert(ac_clock_valid(&s,start));
  for(int n=1;n<2000;++n) {
    const auto now=start+n*1000;
    assert(!local.sampleDue(now,1));
    if(local.publishDue(now,1))s=local.publish(now,1);
    assert(ac_clock_valid(&s,now));
    assert(std::fabs(ac_beat_at(&s,now)-(3.25+n*.002))<1e-9);
  }
  assert(local.samples==1 && local.snapshots==20);
  assert(local.sampleDue(start+2000000,1));
  // A very different incoming Link beat must not jump the local phase.
  local.sample(start+2000000,999,150,1,1);
  s=local.publish(start+2000000,1);
  assert(std::fabs(s.beat-7.25)<1e-9 && s.tempo==150 && s.playing==1);
  assert(std::fabs(ac_beat_at(&s,start+2400000)-8.25)<1e-9);
  assert(!ac_clock_valid(&s,start+2500001)); // Heartbeat loss still expires.
  assert(local.sampleDue(start+2100000,0));
  local.sample(start+2100000,1000,150,0,1);
  assert(local.publish(start+2100000,0).peers==0);
  assert(local.sampleDue(start+2110000,1));
  local.sample(start+2110000,12.5,90,1,0);s=local.publish(start+2110000,1);
  assert(s.beat==12.5 && s.tempo==90 && s.peers==1);
  std::puts("PASS: two-second sampling, fresh low-rate heartbeats, exact local extrapolation, continuous phase on tempo change, peer-loss stop, rejoin alignment and heartbeat expiry");
}
