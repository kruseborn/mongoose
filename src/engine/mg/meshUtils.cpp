#include "meshUtils.h"
#include <glm/gtc/matrix_transform.hpp>

namespace mg {
//   v6----- v5
//   /|      /|
//  v2------v3|
//  | |     | |
//  | |v7---|-|v4
//  |/      |/
//  v0------v1
VolumeCube createVolumeCube(const glm::vec3 &corner, const glm::vec3 &size) {
  const auto scale = glm::scale(glm::mat4{1}, size);
  const auto transform = glm::translate(glm::mat4{1}, corner);
  const auto mat = transform * scale;

  const auto v0 = glm::vec3{mat * glm::vec4{0, 0, 0, 1}};
  const auto v1 = glm::vec3{mat * glm::vec4{1, 0, 0, 1}};
  const auto v2 = glm::vec3{mat * glm::vec4{0, 1, 0, 1}};
  const auto v3 = glm::vec3{mat * glm::vec4{1, 1, 0, 1}};

  const auto v4 = glm::vec3{mat * glm::vec4{1, 0, 1, 1}};
  const auto v5 = glm::vec3{mat * glm::vec4{1, 1, 1, 1}};
  const auto v6 = glm::vec3{mat * glm::vec4{0, 1, 1, 1}};
  const auto v7 = glm::vec3{mat * glm::vec4{0, 0, 1, 1}};

  VolumeCube volumeCube = {{v0, v1, v2, v3, v4, v5, v6, v7}, {0, 1, 3, 3, 2, 0,   // front
                                                              1, 4, 3, 4, 5, 3,   // right
                                                              7, 0, 2, 7, 2, 6,   // left
                                                              2, 3, 5, 2, 5, 6,   // top
                                                              4, 1, 0, 4, 0, 7,   // bottom
                                                              4, 7, 5, 7, 6, 5}}; // back
  return volumeCube;
}

} // namespace mg