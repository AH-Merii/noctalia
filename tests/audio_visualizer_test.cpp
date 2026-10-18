#include "tests/test_check.h"
#include "ui/visuals/audio_visualizer.h"

#include <cmath>

namespace {

  constexpr bool kAudioActive = true;
  constexpr bool kAudioIdle = false;

  float bar(const AudioVisualizer& visualizer) { return visualizer.values().at(0); }

  bool near(float actual, float expected) { return std::fabs(actual - expected) < 0.001F; }

  // While audio plays, frames between spectrum updates neither redraw nor move the bars.
  void skipsFramesWithoutNewValuesWhileAudioPlays() {
    AudioVisualizer visualizer;
    visualizer.setValues({1.0F});
    TEST_CHECK(visualizer.advance(16.0F, kAudioActive));
    const float afterUpdate = bar(visualizer);
    TEST_CHECK(afterUpdate > 0.0F && afterUpdate < 1.0F);

    TEST_CHECK(!visualizer.advance(16.0F, kAudioActive));
    TEST_CHECK(bar(visualizer) == afterUpdate);

    visualizer.setValues({1.0F});
    TEST_CHECK(!visualizer.advance(16.0F, kAudioActive));
  }

  // Skipped frames are not lost: the next tick eases over all the time since the last one. With the
  // default 60 ms smoothing, 60 ms of easing from 0 towards 1 reaches 1 - 1/e.
  void carriesSkippedTimeIntoTheNextTick() {
    AudioVisualizer visualizer;
    visualizer.setValues({0.0F});
    TEST_CHECK(visualizer.advance(0.0F, kAudioActive));

    TEST_CHECK(!visualizer.advance(30.0F, kAudioActive));
    TEST_CHECK(!visualizer.advance(30.0F, kAudioActive));
    visualizer.setValues({1.0F});
    TEST_CHECK(visualizer.advance(0.0F, kAudioActive));
    TEST_CHECK(near(bar(visualizer), 0.632F));
  }

  // Once audio stops, the bars fall back every frame rather than waiting for updates that never come.
  void easesEveryFrameOnceAudioStops() {
    AudioVisualizer visualizer;
    visualizer.setValues({1.0F});
    TEST_CHECK(visualizer.advance(16.0F, kAudioIdle));
    const float first = bar(visualizer);
    TEST_CHECK(visualizer.advance(16.0F, kAudioIdle));
    TEST_CHECK(bar(visualizer) > first);
  }

  // Silence can arrive before the spectrum reports idle. Values stop changing at zero then, so the
  // bars keep falling every frame until they are down, and only then stop ticking.
  void settlesToRestWhileAudioIsStillActive() {
    AudioVisualizer visualizer;
    visualizer.setValues({1.0F});
    TEST_CHECK(visualizer.advance(1000.0F, kAudioActive));
    TEST_CHECK(near(bar(visualizer), 1.0F));

    visualizer.setValues({0.0F});
    TEST_CHECK(visualizer.advance(0.0F, kAudioActive));
    int frames = 0;
    while (!visualizer.converged() && frames < 1000) {
      TEST_CHECK(visualizer.advance(16.0F, kAudioActive));
      ++frames;
    }
    TEST_CHECK(frames > 1);
    TEST_CHECK(bar(visualizer) == 0.0F);
    TEST_CHECK(!visualizer.advance(16.0F, kAudioActive));
  }

} // namespace

int main() {
  skipsFramesWithoutNewValuesWhileAudioPlays();
  carriesSkippedTimeIntoTheNextTick();
  easesEveryFrameOnceAudioStops();
  settlesToRestWhileAudioIsStillActive();
  return 0;
}
