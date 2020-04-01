#include "shaders.h"

#include "mg/mgAssert.h"
#include "mg/mgUtils.h"
#include "vkContext.h"
#include <cassert>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace mg {

static std::unordered_map<std::string, mg::Shader> _shaders;

struct ShaderType {
  VkShaderStageFlagBits stage;
  bool isProcedural;
};

static ShaderType getShaderType(const std::string &fileName) {
  if (fileName.find(".vert") != std::string::npos)
    return {VK_SHADER_STAGE_VERTEX_BIT, false};
  else if (fileName.find(".frag") != std::string::npos)
    return {VK_SHADER_STAGE_FRAGMENT_BIT, false};
  else if (fileName.find(".comp") != std::string::npos)
    return {VK_SHADER_STAGE_COMPUTE_BIT, false};
  else if (fileName.find(".rgen") != std::string::npos)
    return {VK_SHADER_STAGE_RAYGEN_BIT_NV, false};
  else if (fileName.find(".rchit") != std::string::npos)
    return {VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, fileName.find(".proc") ? true : false};
  else if (fileName.find(".rmiss") != std::string::npos)
    return {VK_SHADER_STAGE_MISS_BIT_NV, false};
  else if (fileName.find(".rint") != std::string::npos)
    return {VK_SHADER_STAGE_INTERSECTION_BIT_NV, true};
  else {
    mgAssertDesc(false, "Shader type is not supported");
    return {};
  }
}

static mg::Shaders getShaderFiles() {
  mg::Shaders shaders = {};

  namespace fs = std::filesystem;
  const auto shaderPath = mg::getShaderPath();
  for (auto &file : fs::directory_iterator(shaderPath)) {
    const auto path = fs::path(file);
    auto fileName = path.filename().generic_string();
    auto nameIt = fileName.find_first_of('.');
    auto name = fileName.substr(0, nameIt);
    auto &fileNames = shaders.nameToShaderFiles[name];
    const auto shaderType = getShaderType(fileName);
    fileNames.files.push_back({fileName, shaderType.stage, shaderType.isProcedural});
  }

  for (auto &files : shaders.nameToShaderFiles) {
    std::stable_sort(std::begin(files.second.files), std::end(files.second.files),
                     [](const auto &a, const auto &b) { return a.stageFlag < b.stageFlag; });
  }
  return shaders;
}

static VkShaderModule loadShader(const char *fileName, VkDevice device, VkShaderStageFlagBits stageFlag) {
  auto shaderCode = mg::readBinaryFromDisc(fileName);
  VkShaderModule shaderModule;
  VkShaderModuleCreateInfo moduleCreateInfo;
  VkResult err;

  moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  moduleCreateInfo.pNext = nullptr;

  moduleCreateInfo.codeSize = shaderCode.size();
  moduleCreateInfo.pCode = (uint32_t *)shaderCode.data();
  moduleCreateInfo.flags = 0;
  err = vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &shaderModule);
  mgAssert(!err);
  return shaderModule;
}
static VkPipelineShaderStageCreateInfo createShader(std::string fileName, VkShaderStageFlagBits stage) {
  VkPipelineShaderStageCreateInfo shaderStage = {};
  shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStage.stage = stage;

  shaderStage.module = loadShader((fileName).c_str(), mg::vkContext.device, stage);
  shaderStage.pName = "main";
  mgAssert(shaderStage.module != 0);
  return shaderStage;
}

void createShaders() {
  const auto shaderFiles = getShaderFiles();
  for (const auto &files : shaderFiles.nameToShaderFiles) {
    Shader shader = {};
    shader.name = files.first;
    for (uint32_t i = 0; i < files.second.files.size(); i++) {
      VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo =
          createShader(mg::getShaderPath() + files.second.files[i].name, files.second.files[i].stageFlag);
      shader.isProcedural[shader.count] = files.second.files[i].isProcedural;
      shader.fileNameToIndex[files.second.files[i].name] = shader.count;
      shader.stageCreateInfo[shader.count++] = pipelineShaderStageCreateInfo;
    }
    _shaders.insert(make_pair(files.first, shader));
  }
}

Shader getShader(const std::string &name) {
  auto res = _shaders.find(name);
  mgAssert(res != _shaders.end());
  return res->second;
}
void deleteShaders() {
  for (auto &_shader : _shaders) {
    auto shader = _shader.second;
    for (uint32_t i = 0; i < shader.count; i++) {
      vkDestroyShaderModule(mg::vkContext.device, shader.stageCreateInfo[i].module, nullptr);
    }
  }
  _shaders.clear();
}
} // namespace mg
