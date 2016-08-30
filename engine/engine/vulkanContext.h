#pragma once
#include <vulkan\vulkan.h>


struct VulkanContext {
	uint32_t width, height;
	VkCommandBuffer commandBuffer;
	VkDevice device;
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;

	VkQueue queue;
	VkDescriptorPool descriptorPool;
	VkRenderPass renderPass;
	VkRenderPass deferredRenderPass;

	VkPipelineLayout pipelineLayout;
	VkPipelineLayout pipelineLayoutMultiTexture;


	VkCommandPool commandPool;
	uint32_t graphicsQueueFamilyIndex;
	uint32_t presentQueueFamilyIndex;
	struct {
		VkDescriptorSetLayout ubo;
		VkDescriptorSetLayout texture;
		VkDescriptorSetLayout multiTexture;

	} descriptorSetLayout;
	VkPipelineCache pipelineCache;
	VkPhysicalDeviceProperties deviceProperties;
};
extern VulkanContext vkContext;

void initVulkan(uint32_t width, uint32_t height);
void destroyVkWindow();
void destroyVulkanContext();
void preRendering();
void postRendering();
void beginRendering();
void beginDeferredRendering();
void endRendering();
void endDefferedRendering();

//
////deferred rendering
void initDeferredRendering();
//window managment
VkFormat getSupportedDepthFormat();
int windowShouldClose();
void windowPollEvents();

void createVulkanContext();
void createCommandBuffers();

//dynamic buffers
void createDynamicBuffers();
void swapDynamicBuffers();
void freeDynamicBuffers();
uint8_t* allocateUniform(int size, VkBuffer * buffer, uint32_t * buffer_offset, VkDescriptorSet * descriptor_set);
uint8_t *allocateBuffer(int size, VkBuffer *buffer, VkDeviceSize *buffer_offset);

//mouse
bool isMouseButtonDown();
void getMousePosition(float *xpos, float *ypos);
bool isKeyDown(char c);
