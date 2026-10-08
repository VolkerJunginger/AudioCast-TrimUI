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
  TempoLatch latch;
  latch.observe(start,3.25,120,1,0);s=latch.publish(start,1);
  const auto initialAnchor=latch.anchor.monotonic_us;
  // Five minutes of noisy Link phase and unchanging tempo must not reanchor.
  for(int n=1;n<=600;++n) {
    const auto now=start+n*500000LL;
    assert(latch.checkDue(now,1));
    latch.observe(now,n%2?999:-999,120+1e-8,1,0);
    s=latch.publish(now,1);
    assert(latch.anchor.monotonic_us==initialAnchor && latch.latches==1 && !latch.listening);
    assert(std::fabs(s.beat-(3.25+n))<1e-9);
    assert(ac_clock_valid(&s,now));
  }
  int64_t change=start+300500000;
  latch.observe(change,999,150,1,0);assert(latch.listening && latch.anchor.tempo==120);
  for(int n=1;n<=10;++n) {
    const auto now=change+n*100000;
    assert(latch.checkDue(now,1));
    latch.observe(now,-999,150,1,0);
    assert(latch.anchor.tempo==(n==10?150:120));
  }
  assert(latch.latches==2 && latch.windows==1 && !latch.listening);
  double held=ac_beat_at(&latch.anchor,change+1000000);
  assert(std::fabs(held-(3.25+301.5*2))<1e-9);
  // A moving tempo restarts the confirmation window; a reversal cancels it.
  latch.observe(change+1500000,0,140,1,0);
  latch.observe(change+2000000,0,130,1,0);
  latch.observe(change+2500000,0,130,1,0);assert(latch.anchor.tempo==150);
  latch.observe(change+2600000,0,150,1,0);assert(!latch.listening && latch.latches==2);
  latch.observe(change+2700000,0,140,1,0);
  latch.observe(change+3700000,999,90,0,0);assert(!latch.listening && !latch.connected);
  latch.observe(change+3800000,12.5,90,1,0);s=latch.publish(change+3800000,1);
  assert(s.beat==12.5 && s.tempo==90 && latch.latches==3);
  assert(!ac_clock_valid(&s,change+4300001));
  std::puts("PASS: tempo latch stays fixed for five minutes despite phase/noise; confirmation window, moving tempo, reversal, disconnect/rejoin and expiry");
  std::puts("PASS: two-second sampling, fresh low-rate heartbeats, exact local extrapolation, continuous phase on tempo change, peer-loss stop, rejoin alignment and heartbeat expiry");
}
