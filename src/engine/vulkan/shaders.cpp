#include "shaders.h"

#include <cassert>
#include <string>
#include <unordered_map>

#include "vkContext.h"
#include "mg/mgUtils.h"
#include "mg/mgAssert.h"

namespace mg {

static std::unordered_map<std::string, mg::Shader> _shaders;

static VkShaderModule loadShader(const char *fileName, VkDevice device, VkShaderStageFlagBits stage) {
    auto shaderCode = mg::readBinaryFromDisc(fileName);
	VkShaderModule shaderModule;
	VkShaderModuleCreateInfo moduleCreateInfo;
	VkResult err;

	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.pNext = nullptr;

	moduleCreateInfo.codeSize = shaderCode.size();
	moduleCreateInfo.pCode = (uint32_t*)shaderCode.data();
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

void createShaders(const std::vector<std::string> &shaderNames) {
	for (uint32_t i = 0; i < shaderNames.size(); i++) {
		std::string shaderName = shaderNames[i];
		VkPipelineShaderStageCreateInfo shaderStageVert = createShader(mg::getShaderPath() + shaderName + ".vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		VkPipelineShaderStageCreateInfo shaderStageFrag = createShader(mg::getShaderPath() + shaderName + ".frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		Shader shader = {};
		shader.vertex.shaderStage = shaderStageVert;
		shader.fragment.shaderStage = shaderStageFrag;

		_shaders.insert(make_pair(shaderName, shader));
	}
}
Shader getShader(const std::string &name) {
	auto res = _shaders.find(name);
	mgAssert(res != _shaders.end());
	return res->second;
}
void deleteShaders() {
	for (auto& _shader : _shaders) {
		auto shader = _shader.second;
		vkDestroyShaderModule(mg::vkContext.device, shader.vertex.shaderStage.module, nullptr);
		vkDestroyShaderModule(mg::vkContext.device, shader.fragment.shaderStage.module, nullptr);
	}
	_shaders.clear();
}
}
