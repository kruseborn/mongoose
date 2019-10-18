#include "mg/window.h"
#include "deferred_scene.h"

int main() {
  mg::initWindow(1500, 1024);
  initScene();
  while (mg::startFrame()) {
    const auto frameData = mg::getFrameData(true);

    updateScene(frameData);
    renderScene(frameData);

    mg::endFrame();
  }
  destroyScene();
  mg::destroyWindow();
  return 0;
}
