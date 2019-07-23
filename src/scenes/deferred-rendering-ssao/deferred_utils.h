#pragma once
#include <glm/glm.hpp>
#include "mg/textureContainer.h"

struct SSAOKernel {
  glm::vec4 kernel[64];
  glm::vec2 noiseScale;
};

struct Noise {
  SSAOKernel ssaoKernel;
  mg::TextureId noiseTexture;
};

Noise createNoise();