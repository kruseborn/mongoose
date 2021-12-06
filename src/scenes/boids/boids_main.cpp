#include "boids_scene.h"
#include "mg/window.h"
#include <immintrin.h>
#include "vulkan/vkContext.h"
#include "vulkan/vkUtils.h"
#include <glm/gtc/matrix_transform.hpp>

int main() {

  glm::vec3 u{3, -4, 1};
  glm::vec3 v{0, 2, -3};
  glm::vec3 w{3, 2, 2};

  glm::vec3 ww = glm::cross(v, w);
  float s = glm::dot(ww, ww);
  float ss = ww.x + ww.y + ww.z;


  mg::initWindow(1024, 960);
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
