// Local integration-test participant, GPL-2.0-or-later.
#include <ableton/Link.hpp>
#include <chrono>
#include <thread>
int main() {
  ableton::Link link(120);
  link.enable(true);
  for (int i = 0; i < 200; ++i) {
    if (link.numPeers()) {
      for (double tempo : {90.0, 150.0}) {
        auto state = link.captureAppSessionState();
        state.setTempo(tempo, link.clock().micros());
        link.commitAppSessionState(state);
        std::this_thread::sleep_for(std::chrono::seconds(2));
      }
      return 0;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return 2;
}
