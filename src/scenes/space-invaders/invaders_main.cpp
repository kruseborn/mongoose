#include "invaders_scene.h"
#include "mg/window.h"
#include <chrono>
#include <cmath>
#include <thread>

int main() {
  mg::initWindow(800, 600); 
  Invaders invaders = {}; 
  invadersInit(&invaders);

  constexpr float lastFrameMsMin = 16.0f;
  float startTime = mg::getTime();
  bool resize = true;
  while (mg::startFrame()) {
    if (invaders.aliens.nrAliens == 0 || invaders.player.health <= 0)
      invadersReset(&invaders);

    {
      const auto frameData = mg::getFrameData(resize);
      const auto dt = 0.01f;
      invadersSimulate(&invaders, frameData, dt);
      resize = invadersRender(invaders, frameData);

      invadersEndFrame();
    }
    const auto endTime = mg::getTime();
    const auto lastFrameMs = float((endTime - startTime) * 1000.0);
    startTime = mg::getTime();

    if (lastFrameMs < lastFrameMsMin) {
      std::this_thread::sleep_for(std::chrono::milliseconds(uint32_t(lastFrameMsMin - lastFrameMs)));
    }
    mg::endFrame();
  }
  invadersDestroy(&invaders);
  mg::destroyWindow();
  return 0;
}
