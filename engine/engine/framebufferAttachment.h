#pragma once
#include "vulkan/vulkan.h"

struct FrameBufferAttachment {
	VkImage image;
	VkDeviceMemory mem;
	VkImageView view;
	VkFormat format;
};

void createAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment &attachment, VkCommandBuffer layoutCmd);
