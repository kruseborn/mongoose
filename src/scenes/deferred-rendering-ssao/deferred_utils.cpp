#include "deferred_utils.h"
#include "mg/mgSystem.h"
#include "mg/textureContainer.h"
#include <random>

static float lerp(float a, float b, float f) {
    return a + f * (b - a);
} 

// https://learnopengl.com/Advanced-Lighting/SSAO
SSAOKernel createNoise() {
  SSAOKernel res;
  std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
  std::default_random_engine generator;

  const auto kernalSize = mg::countof(res.kernel);
  for (uint32_t i = 0; i < kernalSize; ++i) {
    glm::vec3 sample(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator));
    sample = glm::normalize(sample);
    sample *= randomFloats(generator);
    float scale = float(i) / kernalSize;

    // scale samples s.t. they're more aligned to center of kernel
    scale = lerp(0.1f, 1.0f, scale * scale);
    sample *= scale;
    res.kernel[i] = {sample, 1.0f};
  }
  glm::vec4 ssaoNoise[16];
  for (uint32_t i = 0; i < mg::countof(ssaoNoise); i++) {
      glm::vec3 noise(
          randomFloats(generator) * 2.0 - 1.0, 
          randomFloats(generator) * 2.0 - 1.0, 
          0.0f); 
      ssaoNoise[i] = {noise, 1.0f};
  } 

  mg::CreateTextureInfo createTextureInfo = {};
  createTextureInfo.id = "noise";
  createTextureInfo.textureSamplers = {mg::TEXTURE_SAMPLER::LINEAR_REPEAT};
  createTextureInfo.type = mg::TEXTURE_TYPE::TEXTURE_2D;
  createTextureInfo.size = {4, 4, 1};
  createTextureInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  createTextureInfo.data = ssaoNoise;
  createTextureInfo.sizeInBytes = mg::sizeofArrayInBytes(ssaoNoise);
  mg::mgSystem.textureContainer.createTexture(createTextureInfo);

  res.noiseScale = glm::vec2(mg::vkContext.screen.width / 4, mg::vkContext.screen.height / 4);

  return res;
}