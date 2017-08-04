//#pragma once
//#include "vulkan/vulkan.h"
//#include <vector>
//#include "imageObject.h"
//#include "framebufferAttachment.h"
//
//struct VulkanContext;
//
//struct Attachment {
//	VkFormat format;
//	VkImageUsageFlagBits imageUsageFlagBits;
//};
//
//struct FrameBuffer {
//	uint32_t _width, _height;
//	VkFramebuffer _frameBuffer[2] = {};
//	std::vector<FrameBufferAttachment> _attachments;
//	FrameBufferAttachment _depth;
//	VkRenderPass _renderPass;
//	VkSampler _colorSampler;
//	VulkanContext *_context;
//
//	FrameBuffer() : _context(&context), _depth(context) { }
//	void createAttachments(Attachment *attachments, uint32_t attachmentsSize);
//private:
//	void createAttachements(Attachment *attachments, uint32_t);
//	void createRenderPass();
//
//	void createFrameBuffer();
//	void createImageSampler();
//};
