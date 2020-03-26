#include "fluid_scene.h"
#include "mg/window.h"

int main() {
  mg::initWindow(128, 128);
  initScene();
  while (mg::startFrame()) {
    auto frameData = mg::getFrameData();
    updateScene(frameData);
    renderScene(frameData);
    mg::endFrame();
  }
  destroyScene();
  mg::destroyWindow();
  return 0;
}