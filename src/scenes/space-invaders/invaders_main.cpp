#include "invaders_scene.h"
#include <chrono>
#include <thread>
#include <cmath>
#include "mg/window.h"

void EngineMain() {
  mg::initWindow(1500, 1024);
  initScene();

  Invaders invaders = {};
  invadersInit(&invaders);

  constexpr float lastFrameMsMin = 16.0f;
  float startTime = float(engine.getStopwatchElapsedSeconds());

  while (mg::startFrame()) {
    if (invaders.aliens.nrAliens == 0 || invaders.player.health <= 0)
      invadersReset(&invaders);

    {
      const auto frameData = mg::getFrameData();
      const auto dt = 0.07f;
      invadersSimulate(&invaders, frameData, dt);
      invadersRender(invaders);

      mg::invadersEndFrame();
    }
    const auto endTime = engine.getStopwatchElapsedSeconds();
    const auto lastFrameMs = float((endTime - startTime) * 1000.0);
    startTime = float(engine.getStopwatchElapsedSeconds());

    if (lastFrameMs < lastFrameMsMin) {
      std::this_thread::sleep_for(std::chrono::milliseconds(uint32_t(lastFrameMsMin - lastFrameMs)));
    }
  }
  invadersDestroy(&invaders);
  mg::destroyWindow();

}
