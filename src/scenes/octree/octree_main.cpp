#include "octree_scene.h"
#include "mg/window.h"
#include <Windows.h>

#include <iostream>

int main() {

  std::cout << (7 << 3) << std::endl;
  std::cout << (7 * 8) << std::endl;
  

  HWND hWnd = GetConsoleWindow();
  MoveWindow(hWnd, 2200, 200, 1200, 800, TRUE);

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
