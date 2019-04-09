#pragma once
#include "vkContext.h"
#include <string>
#include <vector>

namespace mg {
struct Shader {
  std::string name;
  struct {
    VkPipelineShaderStageCreateInfo shaderStage;
  } vertex, fragment;
};
void createShaders(const std::vector<std::string> &shaderNames);
Shader getShader(const std::string &name);
void deleteShaders();

} // namespace mg
