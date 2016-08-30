#pragma once
#include "vulkan\vulkan.h"

#define MAX_TEXTURE_NAME_SIZE 16
struct Texture {
	char name[MAX_TEXTURE_NAME_SIZE] = {};
	VkImage image = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
	VkFormat format = VK_FORMAT_UNDEFINED;
	uint32_t binding = 0;
	VkDescriptorSet descriptorSet = nullptr;
	VkSampler sampler;
};

namespace Textures {
	void init();
	void destroy();

	Texture *findTexture(const char *name);
	Texture* newTexture(const char *name);
	void freeTexture(const char *name);
}

void createFontTexture(Texture *fontTexture);
void createNoiseTexture(Texture *texture);
