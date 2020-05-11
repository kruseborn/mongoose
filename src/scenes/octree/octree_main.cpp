#include "octree_scene.h"
#include "mg/window.h"
#include <Windows.h>

#include <iostream>

int main() {
  mg::initWindow(1024, 1024);
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
