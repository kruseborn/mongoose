#include "shaders.h"
#include <cassert>
#include <string>
#include <vulkan/vulkan.h>
#include <unordered_map>
#include "vulkanContext.h"
#pragma warning( disable : 4996)

using namespace std;
namespace {
	std::unordered_map<string, Shader> _shaders;
	
	char *readBinaryFile(const char *filename, size_t *psize) {
		long int size;
		size_t retval;
		void *shader_code;
		FILE *fp = fopen(filename, "rb");
		if (!fp) return NULL;
		fseek(fp, 0L, SEEK_END);
		size = ftell(fp);
		fseek(fp, 0L, SEEK_SET);
		shader_code = malloc(size);
		retval = fread(shader_code, size, 1, fp);
		assert(retval == 1);
		*psize = size;
		return (char*)shader_code;
	}
	VkShaderModule loadShader(const char *fileName, VkDevice device, VkShaderStageFlagBits stage) {
		size_t size = 0;
		const char *shaderCode = readBinaryFile(fileName, &size);
		assert(size > 0);
		VkShaderModule shaderModule;
		VkShaderModuleCreateInfo moduleCreateInfo;
		VkResult err;

		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.pNext = NULL;

		moduleCreateInfo.codeSize = size;
		moduleCreateInfo.pCode = (uint32_t*)shaderCode;
		moduleCreateInfo.flags = 0;
		err = vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule);
		assert(!err);
		free((void*)shaderCode);

		return shaderModule;
	}
	VkPipelineShaderStageCreateInfo createShader(std::string fileName, VkShaderStageFlagBits stage) {
		VkPipelineShaderStageCreateInfo shaderStage = {};
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.stage = stage;

		shaderStage.module = loadShader((fileName).c_str(), vkContext.device, stage);
		shaderStage.pName = "main";
		assert(shaderStage.module != NULL);
		return shaderStage;
	}

}
void createShaders(const std::vector<std::string> &shaderNames) {
	for (uint32_t i = 0; i < shaderNames.size(); i++) {
		string shaderName = shaderNames[i];
		string path = "shaders/builds/";
		VkPipelineShaderStageCreateInfo shaderStageVert = createShader(path + shaderName + ".vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		VkPipelineShaderStageCreateInfo shaderStageFrag = createShader(path + shaderName + ".frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		Shader shader = {};
		shader.vertex.shaderStage = shaderStageVert;
		shader.fragment.shaderStage = shaderStageFrag;

		_shaders.insert(make_pair(shaderName, shader));
	}
}
Shader getShader(const std::string &name) {
	auto res = _shaders.find(name);
	assert(res != _shaders.end());
	return res->second;
}
void deleteShaders() {
	for (auto& _shader : _shaders) {
		auto shader = _shader.second;
		vkDestroyShaderModule(vkContext.device, shader.vertex.shaderStage.module, nullptr);
		vkDestroyShaderModule(vkContext.device, shader.fragment.shaderStage.module, nullptr);
	}
	_shaders.clear();
}
