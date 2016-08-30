#include "framebufferAttachment.h"
#include "vulkanContext.h"
#include <cassert>
#include "vkUtils.h"

void createAttachment(
	VkFormat format,
	VkImageUsageFlagBits usage,
	FrameBufferAttachment &attachment,
	VkCommandBuffer layoutCmd)
{
	VkImageAspectFlags aspectMask = 0;
	VkImageLayout imageLayout;

	attachment.format = format;

	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	assert(aspectMask > 0);

	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.extent.width = vkContext.width;
	imageCreateInfo.extent.height = vkContext.height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

	VkMemoryAllocateInfo memAlloc = {};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;

	checkResult(vkCreateImage(vkContext.device, &imageCreateInfo, nullptr, &attachment.image));
	vkGetImageMemoryRequirements(vkContext.device, attachment.image, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	checkResult(vkAllocateMemory(vkContext.device, &memAlloc, nullptr, &attachment.mem));
	checkResult(vkBindImageMemory(vkContext.device, attachment.image, attachment.mem, 0));

	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
		// Set the initial layout to shader read instead of attachment 
		// This is done as the render loop does the actualy image layout transitions
		setImageLayout(
			layoutCmd,
			attachment.image,
			aspectMask,
			VK_IMAGE_LAYOUT_UNDEFINED,
			imageLayout);
	}
	else {
		setImageLayout(
			layoutCmd,
			attachment.image,
			aspectMask,
			VK_IMAGE_LAYOUT_UNDEFINED,
			imageLayout);
	}

	VkImageViewCreateInfo imageView = {};
	imageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageView.format = format;
	imageView.subresourceRange = {};
	imageView.subresourceRange.aspectMask = aspectMask;
	imageView.subresourceRange.baseMipLevel = 0;
	imageView.subresourceRange.levelCount = 1;
	imageView.subresourceRange.baseArrayLayer = 0;
	imageView.subresourceRange.layerCount = 1;
	imageView.image = attachment.image;
	checkResult(vkCreateImageView(vkContext.device, &imageView, nullptr, &attachment.view));
}

/*
Texture createFrameBufferAttachment(FrameBufferAttachmentInfo frameBufferAttachmentInfo) {
	Texture texture;
	VkImageAspectFlags aspectMask = 0;
	VkImageLayout imageLayout;

	texture._binding = frameBufferAttachmentInfo.binding;
	texture._type = frameBufferAttachmentInfo.type;
	texture._format = frameBufferAttachmentInfo.format;
	texture._descriptorImageInfo.sampler = frameBufferAttachmentInfo.sampler;

	if (frameBufferAttachmentInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	else if (frameBufferAttachmentInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}
	else assert(false);

	VkImageCreateInfo imageCreateInfo = createImageCreateInfo();
	imageCreateInfo.format = texture._format;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent = { frameBufferAttachmentInfo.width, frameBufferAttachmentInfo.height, 1 };
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = frameBufferAttachmentInfo.usage | VK_IMAGE_USAGE_SAMPLED_BIT;

	texture.createVkImage(imageCreateInfo);

	if (frameBufferAttachmentInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
		setImageLayout(
			frameBufferAttachmentInfo.commandBuffer,
			texture._image,
			aspectMask,
			VK_IMAGE_LAYOUT_UNDEFINED,
			imageLayout);
	}
 	else {
		setImageLayout(
			frameBufferAttachmentInfo.commandBuffer,
			texture._image,
			aspectMask,
			VK_IMAGE_LAYOUT_UNDEFINED,
			imageLayout);
	}
	VkImageViewCreateInfo imageViewCreateInfo = createImageViewCreateInfo();
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = frameBufferAttachmentInfo.format;
	imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
	imageViewCreateInfo.image = texture._image;
	texture.createViewImage(imageViewCreateInfo);
	return texture;
}
*/