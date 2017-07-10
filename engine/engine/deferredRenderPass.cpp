#include "deferredRenderPass.h"
#include "textures.h"
#include "vkUtils.h"
#include "vulkanContext.h"

static VkAttachmentDescription attachmentDescs[RENDERPASS::SIZE] = {};

void createDeferredTextures() {
	Texture *normal = Textures::newTexture("normal");
	Texture *albedo = Textures::newTexture("albedo");
	Texture *depth = Textures::newTexture("depth");
	Texture *ssao = Textures::newTexture("ssao");
	Texture *ssaoblur = Textures::newTexture("ssaoblur");

	createHostTexture(normal, vkContext.width, vkContext.height, VK_FORMAT_R16G16_SFLOAT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	createHostTexture(albedo, vkContext.width, vkContext.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	createHostTexture(depth, vkContext.width, vkContext.height, VK_FORMAT_R16_SFLOAT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	createHostTexture(ssao, vkContext.width, vkContext.height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	createHostTexture(ssaoblur, vkContext.width, vkContext.height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
}

void createAttachmentForRenderPass(VkFormat swapChainFormat, VkFormat depthFormat) {
	for (uint32_t i = 0; i < RENDERPASS::SIZE; i++) {
		attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
	//depth
	attachmentDescs[RENDERPASS::DEPTH].format = depthFormat;
	attachmentDescs[RENDERPASS::DEPTH].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescs[RENDERPASS::DEPTH].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachmentDescs[RENDERPASS::SWAPCHAIN].format = swapChainFormat;
	attachmentDescs[RENDERPASS::SWAPCHAIN].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescs[RENDERPASS::SWAPCHAIN].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachmentDescs[RENDERPASS::NORMAL].format = VK_FORMAT_R16G16_SFLOAT;
	attachmentDescs[RENDERPASS::ALBEDO].format = VK_FORMAT_R8G8B8A8_UNORM;
	attachmentDescs[RENDERPASS::DEPTHOUT].format = VK_FORMAT_R16_SFLOAT;
	attachmentDescs[RENDERPASS::SSAO].format = VK_FORMAT_R16G16B16A16_SFLOAT;
	attachmentDescs[RENDERPASS::SSAOBLUR].format = VK_FORMAT_R16G16B16A16_SFLOAT;
}

void createDeferredRenderPass(VkFormat swapChainFormat, VkFormat depthFormat) {
	createAttachmentForRenderPass(swapChainFormat, depthFormat);

	VkAttachmentReference depthReference = {};
	depthReference.attachment = RENDERPASS::DEPTH;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference1[3] = {
		{RENDERPASS::NORMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		{RENDERPASS::ALBEDO, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		{RENDERPASS::DEPTHOUT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
	};

	VkSubpassDescription subpass1 = {};
	subpass1.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass1.pColorAttachments = colorReference1;
	subpass1.colorAttachmentCount = 3;
	subpass1.pDepthStencilAttachment = &depthReference;

	//subpass 2
	VkAttachmentReference inputReference2[2] = {
		{RENDERPASS::NORMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		{RENDERPASS::DEPTHOUT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
	};
	VkAttachmentReference colorReference2[1] = {
		{ SSAO, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
	};

	uint32_t preservedReference2[1] = { ALBEDO };

	VkSubpassDescription subpass2 = {};
	subpass2.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass2.colorAttachmentCount = 1;
	subpass2.pColorAttachments = colorReference2;
	subpass2.inputAttachmentCount = 2;
	subpass2.pInputAttachments = inputReference2;
	subpass2.preserveAttachmentCount = 1;
	subpass2.pPreserveAttachments = preservedReference2;

	//subpass 3
	VkAttachmentReference inputReference3[1] = {
		{ SSAO, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
	};
	VkAttachmentReference colorReference3[1] = {
		{ SSAOBLUR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
	};

	uint32_t preservedReference3[3] = { NORMAL, ALBEDO };

	VkSubpassDescription subpass3 = {};
	subpass3.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass3.colorAttachmentCount = 1;
	subpass3.pColorAttachments = colorReference3;
	subpass3.inputAttachmentCount = 1;
	subpass3.pInputAttachments = inputReference3;
	subpass2.preserveAttachmentCount = 2;
	subpass2.pPreserveAttachments = preservedReference3;

	//subpass 4
	VkAttachmentReference inputReference4[3] = {
		{ NORMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		{ ALBEDO, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		{ SSAOBLUR, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
	};
	VkAttachmentReference colorReference4[1] = {
		{ SWAPCHAIN, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
	};

	VkSubpassDescription subpass4 = {};
	subpass4.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass4.colorAttachmentCount = 1;
	subpass4.pColorAttachments = colorReference4;
	subpass4.inputAttachmentCount = 3;
	subpass4.pInputAttachments = inputReference4;

	//Render pass
	VkSubpassDependency dependency[3] = {};
	dependency[0].srcSubpass = 0;
	dependency[0].dstSubpass = 1;
	dependency[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency[0].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependency[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	dependency[1].srcSubpass = 1;
	dependency[1].dstSubpass = 2;
	dependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependency[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;


	dependency[2].srcSubpass = 2;
	dependency[2].dstSubpass = 3;
	dependency[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependency[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	VkSubpassDescription subpasses[4] = { subpass1, subpass2, subpass3, subpass4 };
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pAttachments = attachmentDescs;
	renderPassInfo.attachmentCount = RENDERPASS::SIZE;
	renderPassInfo.subpassCount = 4;
	renderPassInfo.pSubpasses = subpasses;
	renderPassInfo.dependencyCount = 3;
	renderPassInfo.pDependencies = dependency;

	checkResult(vkCreateRenderPass(vkContext.device, &renderPassInfo, nullptr, &vkContext.deferredRenderPass));

}

void createDeferredFrameBuffer(VkImageView depthStencilView, VkImageView *swapChainImageViews, uint32_t numOfSwapChainImages, VkFramebuffer *frameBuffers) {
	VkImageView attachments[RENDERPASS::SIZE] = {};
	attachments[NORMAL] = Textures::findTexture("normal")->imageView;
	attachments[ALBEDO] = Textures::findTexture("albedo")->imageView;
	attachments[DEPTHOUT] = Textures::findTexture("depth")->imageView;

	attachments[SSAO] = Textures::findTexture("ssao")->imageView;
	attachments[SSAOBLUR] = Textures::findTexture("ssaoblur")->imageView;
	attachments[DEPTH] = depthStencilView;

	for (uint32_t i = 0; i < numOfSwapChainImages; i++) {
		attachments[SWAPCHAIN] = swapChainImageViews[i];
		VkFramebufferCreateInfo fbufCreateInfo = {};
		fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbufCreateInfo.pNext = nullptr;
		fbufCreateInfo.renderPass = vkContext.deferredRenderPass;
		fbufCreateInfo.pAttachments = attachments;
		fbufCreateInfo.attachmentCount = RENDERPASS::SIZE;
		fbufCreateInfo.width = vkContext.width;
		fbufCreateInfo.height = vkContext.height;
		fbufCreateInfo.layers = 1;

		checkResult(vkCreateFramebuffer(vkContext.device, &fbufCreateInfo, nullptr, &frameBuffers[i]));
	}
}