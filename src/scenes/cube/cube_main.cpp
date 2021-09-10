#include "cube_scene.h"
#include "mg/window.h"
#include <immintrin.h>
#include "vulkan/vkContext.h"
#include "vulkan/vkUtils.h"
#include <glm/gtc/matrix_transform.hpp>


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
