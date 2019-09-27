#pragma once
#include "vkContext.h"
#include <string>
#include <unordered_map>
#include <vector>
namespace mg {

struct Shaders {
  struct File {
    std::string name;
    VkShaderStageFlagBits stageFlag;
    bool isProcedural;
  };
  struct ShaderFiles {
    std::vector<File> files;
  };
  std::unordered_map<std::string, ShaderFiles> nameToShaderFiles;
};

struct Shader {
  std::string name;
  uint32_t count;
  bool isProcedural[5];
  VkPipelineShaderStageCreateInfo stageCreateInfo[5];

};
void createShaders();
Shader getShader(const std::string &name);

void deleteShaders();

} // namespace mg
