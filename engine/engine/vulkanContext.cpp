#pragma once
#include "vulkanContext.h"
#include <vulkan\vulkan.h>
#include "vkUtils.h"
#include "glm\glm.hpp"
#include "vkDefs.h"
#include "rendering.h"
#include "textures.h"
#include "pipelines.h"
#include "meshes.h"

//globals
VulkanContext vkContext;

void destroyVulkanContext() {
	vkDeviceWaitIdle(vkContext.device);

	Textures::destroy();
	Pipelines::destroy();
	Meshes::destroy();

	freeDynamicBuffers();
	vkDestroyRenderPass(vkContext.device, vkContext.renderPass, nullptr);
	vkDestroyRenderPass(vkContext.device, vkContext.deferredRenderPass, nullptr);

	vkDestroyPipelineCache(vkContext.device, vkContext.pipelineCache, nullptr);
	vkDestroyPipelineLayout(vkContext.device, vkContext.pipelineLayout, nullptr);
	vkDestroyPipelineLayout(vkContext.device, vkContext.pipelineLayoutMultiTexture, nullptr);

	vkDestroyDescriptorSetLayout(vkContext.device, vkContext.descriptorSetLayout.ubo, nullptr);
	vkDestroyDescriptorSetLayout(vkContext.device, vkContext.descriptorSetLayout.texture, nullptr);


	vkDestroyCommandPool(vkContext.device, vkContext.commandPool, nullptr);

	vkDestroyDescriptorPool(vkContext.device, vkContext.descriptorPool, nullptr);

	// Note: implicitly destroys images (in fact, we're not allowed to do that explicitly)

	
	destroyVkWindow();


	vkDestroyDevice(vkContext.device, nullptr);
}

static void createDescriptorLayout() {
	VkDescriptorSetLayoutBinding uboSamplerLayoutBindings = {};
	uboSamplerLayoutBindings.binding = 0;
	uboSamplerLayoutBindings.descriptorCount = 1;
	uboSamplerLayoutBindings.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	uboSamplerLayoutBindings.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = 1;
	descriptorSetLayoutCreateInfo.pBindings = &uboSamplerLayoutBindings;

	checkResult(vkCreateDescriptorSetLayout(vkContext.device, &descriptorSetLayoutCreateInfo, nullptr, &vkContext.descriptorSetLayout.ubo));

	VkDescriptorSetLayoutBinding textureLayout = {};
	textureLayout.binding = 0;
	textureLayout.descriptorCount = 1;
	textureLayout.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureLayout.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

	descriptorSetLayoutCreateInfo.bindingCount = 1;
	descriptorSetLayoutCreateInfo.pBindings = &textureLayout;

	checkResult(vkCreateDescriptorSetLayout(vkContext.device, &descriptorSetLayoutCreateInfo, nullptr, &vkContext.descriptorSetLayout.texture));

	/*VkDescriptorSetLayoutBinding textureLayout1 = {};
	textureLayout1.binding = 1;
	textureLayout1.descriptorCount = 1;
	textureLayout1.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureLayout1.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

	VkDescriptorSetLayoutBinding textureLayout2 = {};
	textureLayout2.binding = 2;
	textureLayout2.descriptorCount = 1;
	textureLayout2.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureLayout2.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

	VkDescriptorSetLayoutBinding textures[3] = { textureLayout, textureLayout1, textureLayout2 };
	descriptorSetLayoutCreateInfo.bindingCount = 3;
	descriptorSetLayoutCreateInfo.pBindings = textures;

	checkResult(vkCreateDescriptorSetLayout(vkContext.device, &descriptorSetLayoutCreateInfo, nullptr, &vkContext.descriptorSetLayout.multiTexture));*/
}

static void createPipelineLayout() {
	const uint32_t nrOfDescriptorLayouts = 6;
	VkDescriptorSetLayout descriptorSetLayouts[nrOfDescriptorLayouts] = { 
		vkContext.descriptorSetLayout.ubo,
		vkContext.descriptorSetLayout.texture, 
		vkContext.descriptorSetLayout.texture,
		vkContext.descriptorSetLayout.texture,
		vkContext.descriptorSetLayout.texture,
		vkContext.descriptorSetLayout.texture,
	};

	VkPipelineLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.setLayoutCount = 2;
	layoutCreateInfo.pSetLayouts = descriptorSetLayouts;

	VkPushConstantRange pushConstantRange = {}; 
	pushConstantRange.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
	pushConstantRange.size = 256;	

	layoutCreateInfo.pPushConstantRanges = &pushConstantRange;
	layoutCreateInfo.pushConstantRangeCount = 1;

	checkResult(vkCreatePipelineLayout(vkContext.device, &layoutCreateInfo, nullptr, &vkContext.pipelineLayout));
	
	//multiTexture
	layoutCreateInfo.setLayoutCount = nrOfDescriptorLayouts;
	checkResult(vkCreatePipelineLayout(vkContext.device, &layoutCreateInfo, nullptr, &vkContext.pipelineLayoutMultiTexture));
}
static void createPipelineCache() {
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	VkResult err = vkCreatePipelineCache(vkContext.device, &pipelineCacheCreateInfo, nullptr, &vkContext.pipelineCache);
	assert(!err);
}

//void onWindowSizeChanged() {
//	cleanup();
//	createSwapChain();
//	createRenderPass();
//	createImageViews();
//	createFramebuffers();
//}

static void cleanup() {
	vkDeviceWaitIdle(vkContext.device);
	vkDestroyRenderPass(vkContext.device, vkContext.renderPass, nullptr);
}

static void createCommandPool() {
	// Create graphics command pool
	VkCommandPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCreateInfo.queueFamilyIndex = vkContext.graphicsQueueFamilyIndex;
	poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	checkResult(vkCreateCommandPool(vkContext.device, &poolCreateInfo, nullptr, &vkContext.commandPool));
}

static void createDescriptorPool() {
	const uint32_t poolSize = 2;
	VkDescriptorPoolSize descriptorPoolSize[poolSize] = {};

	descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptorPoolSize[0].descriptorCount = 2;

	descriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorPoolSize[1].descriptorCount = 20;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = poolSize;
	createInfo.pPoolSizes = descriptorPoolSize;
	createInfo.maxSets = 20;

	checkResult(vkCreateDescriptorPool(vkContext.device, &createInfo, nullptr, &vkContext.descriptorPool));
}

void initVulkan(uint32_t width, uint32_t height) {
	vkContext.width = width;
	vkContext.height = height;
	createVulkanContext();
	createCommandPool();
	createPipelineCache();
	createDescriptorPool();
	createDescriptorLayout();
	createPipelineLayout();
	createCommandBuffers();

	createDynamicBuffers();

	//deferred rendering pass
	Textures::init();

	initDeferredRendering();
}

