//#include "frameBuffer.h"
//#include <cassert>
//#include "vulkanContext.h"
//#include "vkUtils.h"
//#include "imageUtils.h"
//#include "utils.h"
//
//namespace {
//	VkAttachmentDescription createAttachmentDescription() {
//		VkAttachmentDescription attachmentDescs = {};
//		attachmentDescs.samples = VK_SAMPLE_COUNT_1_BIT;
//		attachmentDescs.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//		attachmentDescs.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//		attachmentDescs.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//		attachmentDescs.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//		return attachmentDescs;
//	}
//}
//
//
//void FrameBuffer::createAttachments(Attachment *attachments, uint32_t attachmentsSize) {
//	_width = 1024;
//	_height = 1024;
//	createImageSampler();
//	createAttachements(attachments, attachmentsSize);
//	createRenderPass();
//	createFrameBuffer();
//}
//
//void FrameBuffer::createAttachements(Attachment *attachments, uint32_t attachmentsSize) {
//	VkCommandBuffer commandBuffer = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
//	beginCommandBuffer(commandBuffer);
//
//	VkFormat depthFormat = getSupportedDepthFormat();
//	FrameBufferAttachmentInfo frameBufferInfo = {};
//	frameBufferInfo.sampler = nullptr;
//	frameBufferInfo.width = _width;
//	frameBufferInfo.height = _height;
//	frameBufferInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
//	frameBufferInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//	frameBufferInfo.format = depthFormat;
//	frameBufferInfo.commandBuffer = commandBuffer;
//	FrameBufferAttachment depthAttachment(*_context);
//	depthAttachment.createAttachment(frameBufferInfo);
//	_depth = std::move(depthAttachment);
//
//	for (uint32_t i = 0; i < attachmentsSize; i++) {
//		frameBufferInfo.format = attachments[i].format;
//		frameBufferInfo.usage = attachments[i].imageUsageFlagBits;
//		frameBufferInfo.sampler = _colorSampler;
//		
//
//		FrameBufferAttachment attachment(*_context);
//		attachment.createAttachment(frameBufferInfo);
//		_attachments.emplace_back(std::move(attachment));
//	}
//	endCommandBuffer(commandBuffer);
//	submitCommandBufferToQueue(commandBuffer, _context->graphicsQueue);
//	waitForQueueToBeIdle(_context->graphicsQueue);
//}
//
//void FrameBuffer::createImageSampler() {
//	VkSamplerCreateInfo sampler = {};
//	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
//	sampler.magFilter = VK_FILTER_LINEAR;
//	sampler.minFilter = VK_FILTER_LINEAR;
//	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
//	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
//	sampler.addressModeV = sampler.addressModeU;
//	sampler.addressModeW = sampler.addressModeU;
//	sampler.mipLodBias = 0.0f;
//	sampler.maxAnisotropy = 0;
//	sampler.minLod = 0.0f;
//	sampler.maxLod = 1.0f;
//	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
//	checkResult(vkCreateSampler(_context->device, &sampler, nullptr, &_colorSampler));
//}
//
//void FrameBuffer::createFrameBuffer() {
//	uint32_t totalNrOfAttachment = uint32_t(_attachments.size()) + 2;
//	uint32_t depthIndex = totalNrOfAttachment - 1;
//	uint32_t swapFrameIndex = totalNrOfAttachment - 2;
//
//	
//	VkImageView *viewAttachments = (VkImageView*)stackAlloc(sizeof(VkImageView) * 5);
//	for (uint32_t f = 0; f < 2; f++) {
//		for (uint32_t i = 0; i < _attachments.size(); ++i) {
//			viewAttachments[i] = _attachments[i]._imageObject._imageView;
//		}
//		//viewAttachments[swapFrameIndex] = vkContext.swapChainImageViews[f];
//		viewAttachments[depthIndex] = _depth._imageObject._imageView;
//		VkFramebufferCreateInfo fbufCreateInfo = {};
//		fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
//		fbufCreateInfo.pNext = NULL;
//		fbufCreateInfo.renderPass = _renderPass;
//		fbufCreateInfo.pAttachments = viewAttachments;
//		fbufCreateInfo.attachmentCount = totalNrOfAttachment;
//		fbufCreateInfo.width = _width;
//		fbufCreateInfo.height = _height;
//		fbufCreateInfo.layers = 1;
//
//		checkResult(vkCreateFramebuffer(_context->device, &fbufCreateInfo, nullptr, &_frameBuffer[f]));
//	}
//}
//
//void FrameBuffer::createRenderPass() {
//	//subpass 1
//	VkAttachmentReference colorReferences = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
//
//	VkSubpassDescription subpass1 = {};
//	subpass1.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//	subpass1.pColorAttachments = &colorReferences;
//	subpass1.colorAttachmentCount = 1;
//
//	//subpass 2
//	VkAttachmentReference inputRefernce = { 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
//
//	VkSubpassDescription subpass2 = {};
//	subpass2.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//	subpass2.inputAttachmentCount = 1;
//	subpass2.pInputAttachments = &inputRefernce;
//	
//	//Render pass
//	VkAttachmentDescription attachmentDescs = {};
//	attachmentDescs.samples = VK_SAMPLE_COUNT_1_BIT;
//	attachmentDescs.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//	attachmentDescs.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//	attachmentDescs.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//	attachmentDescs.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//	attachmentDescs.format = VK_FORMAT_R16G16B16A16_SFLOAT;
//	attachmentDescs.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//	attachmentDescs.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//	VkSubpassDependency dependency = {};
//	
//	dependency.srcSubpass = 0;
//	dependency.dstSubpass = 1;	
//	dependency.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
//	dependency.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
//	dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//	dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//
//	VkSubpassDescription subpasses[2] = { subpass1, subpass2 };
//	VkRenderPassCreateInfo renderPassInfo = {};
//	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//	renderPassInfo.pAttachments = &attachmentDescs;
//	renderPassInfo.attachmentCount = 1;
//	renderPassInfo.subpassCount = 2;
//	renderPassInfo.pSubpasses = subpasses;
//	renderPassInfo.dependencyCount = 1;
//	renderPassInfo.pDependencies = &dependency;
//
//	checkResult(vkCreateRenderPass(_context->device, &renderPassInfo, nullptr, &_renderPass));
//
//}
