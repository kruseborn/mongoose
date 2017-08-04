#pragma once
#include "vulkan/vulkan.h"

enum RENDERPASS {
	NORMAL, ALBEDO, DEPTHOUT, DEPTH, SWAPCHAIN, SSAO, SSAOBLUR, SIZE
};

void createDeferredTextures();
void createDeferredRenderPass(VkFormat swapChainFormat, VkFormat depthFormat);
void createDeferredFrameBuffer(VkImageView depthStencilView, VkImageView *swapChainImageViews, uint32_t numOfSwapChainImages, VkFramebuffer *frameBuffers);
