#include "cube_scene.h"
#include "mg/window.h"

int main() {
  mg::initWindow(1500, 1024);
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