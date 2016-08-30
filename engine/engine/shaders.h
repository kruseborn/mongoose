#pragma once
#include "vulkan\vulkan.h"
#include <vector>

struct Shader {
	std::string name;
	struct {
		VkPipelineShaderStageCreateInfo shaderStage;
	} vertex, fragment;
};

struct VulkanContext;
void createShaders(const std::vector<std::string> &shaderNames);
Shader getShader(const std::string &name);
void deleteShaders();
