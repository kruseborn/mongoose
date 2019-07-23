#include "singleRenderpass.h"
#include "mg/textureContainer.h"
#include "rendering/rendering.h"
#include "vkUtils.h"

#include "mg/mgSystem.h"

namespace mg {
static TextureId depthId;

static void createAttachments() {
  mg::CreateTextureInfo createTextureInfo = {};

  createTextureInfo.type = mg::TEXTURE_TYPE::DEPTH;
  createTextureInfo.size = {mg::vkContext.screen.width, mg::vkContext.screen.height, 1};
  createTextureInfo.format = mg::vkContext.formats.depth;

  depthId = mg::mgSystem.textureContainer.createTexture(createTextureInfo);
}

static void createFrameBuffers(SingleRenderPass *singleRenderPass) {
  VkImageView attachments[SINGLEPASS_ATTACHMENTS::SIZE];
  attachments[SINGLEPASS_ATTACHMENTS::DEPTH] = mg::getTexture(depthId).imageView;

  VkFramebufferCreateInfo framebufferCreateInfo = {};
  framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferCreateInfo.renderPass = singleRenderPass->vkRenderPass;
  framebufferCreateInfo.pAttachments = attachments;
  framebufferCreateInfo.attachmentCount = SINGLEPASS_ATTACHMENTS::SIZE;
  framebufferCreateInfo.width = mg::vkContext.screen.width;
  framebufferCreateInfo.height = mg::vkContext.screen.height;
  framebufferCreateInfo.layers = 1;

  for (uint32_t i = 0; i < mg::vkContext.swapChain->numOfImages; i++) {
    attachments[SINGLEPASS_ATTACHMENTS::SWAPCHAIN] = mg::vkContext.swapChain->imageViews[i];
    checkResult(vkCreateFramebuffer(mg::vkContext.device, &framebufferCreateInfo, nullptr, &singleRenderPass->vkFrameBuffers[i]));
  }
}

static void createRenderPass(SingleRenderPass *singleRenderPass) {
  VkAttachmentDescription attachments[SINGLEPASS_ATTACHMENTS::SIZE] = {};
  // color attachment
  attachments[SINGLEPASS_ATTACHMENTS::SWAPCHAIN].format = mg::vkContext.swapChain->format;
  attachments[SINGLEPASS_ATTACHMENTS::SWAPCHAIN].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[SINGLEPASS_ATTACHMENTS::SWAPCHAIN].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[SINGLEPASS_ATTACHMENTS::SWAPCHAIN].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[SINGLEPASS_ATTACHMENTS::SWAPCHAIN].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[SINGLEPASS_ATTACHMENTS::SWAPCHAIN].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[SINGLEPASS_ATTACHMENTS::SWAPCHAIN].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[SINGLEPASS_ATTACHMENTS::SWAPCHAIN].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  // depth attachment
  attachments[SINGLEPASS_ATTACHMENTS::DEPTH].format = mg::getTexture(depthId).format;
  attachments[SINGLEPASS_ATTACHMENTS::DEPTH].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[SINGLEPASS_ATTACHMENTS::DEPTH].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[SINGLEPASS_ATTACHMENTS::DEPTH].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[SINGLEPASS_ATTACHMENTS::DEPTH].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[SINGLEPASS_ATTACHMENTS::DEPTH].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[SINGLEPASS_ATTACHMENTS::DEPTH].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[SINGLEPASS_ATTACHMENTS::DEPTH].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorAttachmentReference = {};
  colorAttachmentReference.attachment = SINGLEPASS_ATTACHMENTS::SWAPCHAIN;
  colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthReference = {};
  depthReference.attachment = SINGLEPASS_ATTACHMENTS::DEPTH;
  depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subPassDescription = {};
  subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subPassDescription.colorAttachmentCount = 1;
  subPassDescription.pColorAttachments = &colorAttachmentReference;
  subPassDescription.pDepthStencilAttachment = &depthReference;

  const uint32_t nrOfDependencies = 2;
  VkSubpassDependency subpassDependencies[nrOfDependencies] = {};

  subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  subpassDependencies[0].dstSubpass = 0;
  subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  subpassDependencies[1].srcSubpass = 0;
  subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  // Create the render pass
  VkRenderPassCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  createInfo.attachmentCount = SINGLEPASS_ATTACHMENTS::SIZE;
  createInfo.pAttachments = attachments;
  createInfo.subpassCount = 1;
  createInfo.pSubpasses = &subPassDescription;
  createInfo.dependencyCount = nrOfDependencies;
  createInfo.pDependencies = subpassDependencies;

  checkResult(vkCreateRenderPass(mg::vkContext.device, &createInfo, nullptr, &singleRenderPass->vkRenderPass));
}

void initSingleRenderPass(SingleRenderPass *singleRenderPass) {
  createAttachments();
  createRenderPass(singleRenderPass);
  createFrameBuffers(singleRenderPass);
}

void resizeSingleRenderPass(SingleRenderPass *singleRenderPass) {
  mg::removeTexture(depthId);
  for (size_t i = 0; i < mg::vkContext.swapChain->numOfImages; i++) {
    vkDestroyFramebuffer(mg::vkContext.device, singleRenderPass->vkFrameBuffers[i], nullptr);
  }
  createAttachments();
  createFrameBuffers(singleRenderPass);
}

void destroySingleRenderPass(SingleRenderPass *singleRenderPass) {
  mg::removeTexture(depthId);
  for (size_t i = 0; i < mg::vkContext.swapChain->numOfImages; i++) {
    vkDestroyFramebuffer(mg::vkContext.device, singleRenderPass->vkFrameBuffers[i], nullptr);
  }
  vkDestroyRenderPass(mg::vkContext.device, singleRenderPass->vkRenderPass, nullptr);
}

void beginSingleRenderPass(const SingleRenderPass &singleRenderPass) {
  VkClearValue clearValues[SINGLEPASS_ATTACHMENTS::SIZE];
  clearValues[SINGLEPASS_ATTACHMENTS::SWAPCHAIN].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
  clearValues[SINGLEPASS_ATTACHMENTS::DEPTH].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo renderPassBeginInfo = {};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.renderPass = singleRenderPass.vkRenderPass;
  renderPassBeginInfo.framebuffer = singleRenderPass.vkFrameBuffers[mg::vkContext.swapChain->currentSwapChainIndex];
  renderPassBeginInfo.renderArea.extent.width = mg::vkContext.screen.width;
  renderPassBeginInfo.renderArea.extent.height = mg::vkContext.screen.height;
  renderPassBeginInfo.clearValueCount = SINGLEPASS_ATTACHMENTS::SIZE;
  renderPassBeginInfo.pClearValues = clearValues;

  vkCmdBeginRenderPass(mg::vkContext.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
  setFullscreenViewport();
}
void endSingleRenderPass() { vkCmdEndRenderPass(mg::vkContext.commandBuffer); }

} // namespace mg