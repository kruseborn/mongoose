#pragma once

#include "vulkanContext.h"
#include <cassert>
#include <cstdio>

struct Texture;

int memoryTypeFromProperties(uint32_t type_bits, VkFlags requirements_mask);
uint32_t getMemoryType(uint32_t typeBits, VkFlags properties);

//buffers
void createDeviceVisibleBuffer(void *data, const uint32_t size, VkBuffer &buffer, VkDeviceMemory &deviceMemory, VkBufferUsageFlags usage);
void createHostVisibleBuffer(void *data, const uint32_t size, VkBuffer buffer, VkDeviceMemory deviceMemory, VkBufferUsageFlags usage);

void createBuffer(VkBuffer &buffer, VkDeviceSize size, VkBufferUsageFlags usage);
void allocateDeviceMemoryFromBuffer(VkBuffer &buffer, VkDeviceMemory &memory, VkMemoryPropertyFlags type);
void copyDataToDeviceMemory(VkDeviceMemory &memory, void *data, VkDeviceSize size);
void destoyBuffer(VkBuffer buffer);
void destroyDeviceMemory(VkDeviceMemory memory);

//command buffer
void beginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usageFlags = 0);
void endCommandBuffer(VkCommandBuffer commandBuffer);
VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level);
void submitCommandBufferToQueue(VkCommandBuffer commandBuffer);
void waitForQueueToBeIdle();
void freeCommandBuffer(VkCommandBuffer commandBuffer);

//image utils
void updateDescriptorSetForTexture(VkSampler sampler, VkImageView imageView, VkDescriptorSet descriptorSet);
void createHostTexture(Texture *texture, uint32_t width, uint32_t height, VkFormat format, VkImageLayout imageLayout, VkImageUsageFlags usage);
void createDeviceTexture(Texture *texture, uint32_t textureDataSize, uint32_t width, uint32_t height, void *data, VkFormat format);
VkImageCreateInfo createImageCreateInfo();
VkImage createImage(VkImageCreateInfo imageCreateInfo);
VkImageViewCreateInfo createImageViewCreateInfo();
VkImageView createImageView(VkImageViewCreateInfo imageCreateInfo);
VkDeviceSize allocateDeviceMemoryForImage(VkImage &image, VkDeviceMemory &memory, VkMemoryPropertyFlagBits type);
VkImageMemoryBarrier createImageMemoryBarrier();
void setImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageAspectFlags aspect, VkImageLayout oldLayout, VkImageLayout newLayout);

inline char* errorString(VkResult errorCode) {
	switch (errorCode) {
#define STR(r) case VK_ ##r: return #r
		STR(NOT_READY);
		STR(TIMEOUT);
		STR(EVENT_SET);
		STR(EVENT_RESET);
		STR(INCOMPLETE);
		STR(ERROR_OUT_OF_HOST_MEMORY);
		STR(ERROR_OUT_OF_DEVICE_MEMORY);
		STR(ERROR_INITIALIZATION_FAILED);
		STR(ERROR_DEVICE_LOST);
		STR(ERROR_MEMORY_MAP_FAILED);
		STR(ERROR_LAYER_NOT_PRESENT);
		STR(ERROR_EXTENSION_NOT_PRESENT);
		STR(ERROR_FEATURE_NOT_PRESENT);
		STR(ERROR_INCOMPATIBLE_DRIVER);
		STR(ERROR_TOO_MANY_OBJECTS);
		STR(ERROR_FORMAT_NOT_SUPPORTED);
		STR(ERROR_SURFACE_LOST_KHR);
		STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
		STR(SUBOPTIMAL_KHR);
		STR(ERROR_OUT_OF_DATE_KHR);
		STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
		STR(ERROR_VALIDATION_FAILED_EXT);
		STR(ERROR_INVALID_SHADER_NV);
#undef STR
	default:
		return "UNKNOWN_ERROR";
	}
}
inline VkResult checkResult(VkResult result) {
	if (result != VK_SUCCESS) {
		char	 st[60];
		sprintf_s(st, sizeof(st), "Fatal : VkResult returned %s", errorString(result));
		printf("%s", st);
		assert(result == VK_SUCCESS);
	}
	return result;
}
