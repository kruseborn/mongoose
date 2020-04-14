#include "marching_cube_scene.h"
#include "mg/window.h"
#include <iostream>
#include <vector>
int main() {
  mg::initWindow(1000, 1000);
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