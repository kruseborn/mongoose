#include "mg/window.h"
#include "nbody_scene.h"

int ubo() {
  mg::initWindow(1500, 1024);
  initScene();
  while (mg::startFrame()) {
    const auto frameData = mg::getFrameData();

    updateScene(frameData);
    renderScene(frameData);

    mg::endFrame();
  }
  destroyScene();
  mg::destroyWindow();
  return 0;
}
