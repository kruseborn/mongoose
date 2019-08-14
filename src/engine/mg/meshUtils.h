#pragma once
#include <glm/glm.hpp>

namespace mg {
struct VolumeCube {
  glm::vec3 vertices[8];
  uint32_t indices[36];
};

VolumeCube createVolumeCube(const glm::vec3 &corner, const glm::vec3 &size);


} // namespace mg