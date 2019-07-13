#pragma once
#include "vkContext.h"
#include <string>
#include <vector>

namespace mg {

struct Shaders {
  std::vector<std::string> graphics;
  std::vector<std::string> computes;
};

struct Shader {
  std::string name;
  bool isCompute;
  struct {
    VkPipelineShaderStageCreateInfo shaderStage;
  } vertex, fragment, compute;
};
void createShaders(const std::vector<std::string> &shaderNames);
void createComputeShaders(const std::vector<std::string> &shaderNames);
Shader getShader(const std::string &name);

void deleteShaders();

} // namespace mg
