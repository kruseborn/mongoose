#pragma once
#include <glm/glm.hpp>

struct SSAOKernel {
  glm::vec4 kernel[64];
  glm::vec2 noiseScale;
};

SSAOKernel createNoise();