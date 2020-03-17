#include "mg/window.h"
#include "boids_scene.h"

int main() {
  mg::initWindow(2048, 1600);
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
