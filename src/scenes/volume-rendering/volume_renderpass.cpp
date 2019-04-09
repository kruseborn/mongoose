#include "volume_renderpass.h"

#include "mg/mgSystem.h"
#include "mg/textureContainer.h"
#include "vulkan/vkContext.h"

static void createTextures() {
  mg::CreateTextureInfo createTextureInfo = {};

  createTextureInfo.textureSamplers = {mg::TEXTURE_SAMPLER::LINEAR_CLAMP_TO_BORDER};
  createTextureInfo.type = mg::TEXTURE_TYPE::ATTACHMENT;
  createTextureInfo.size = {mg::vkContext.screen.width, mg::vkContext.screen.height, 1};

  createTextureInfo.id = "front";
  createTextureInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  mg::mgSystem.textureContainer.createTexture(createTextureInfo);

  createTextureInfo.id = "back";
  createTextureInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  mg::mgSystem.textureContainer.createTexture(createTextureInfo);

  createTextureInfo.id = "color";
  createTextureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  mg::mgSystem.textureContainer.createTexture(createTextureInfo);

  createTextureInfo.id = "depth";
  createTextureInfo.type = mg::TEXTURE_TYPE::DEPTH;
  createTextureInfo.format = mg::vkContext.formats.depth;
  mg::mgSystem.textureContainer.createTexture(createTextureInfo);
}

static void createRenderPass(VolumeRenderPass *volumeRenderPass) {
  const auto frontTexture = mg::getTexture("front");
  const auto backTexture = mg::getTexture("back");
  const auto colorTexture = mg::getTexture("color");
  const auto depthTexture = mg::getTexture("depth");

  // attachments
  VkAttachmentDescription attachmentDescs[VOLUME_ATTACHMENTS::SIZE] = {};
  for (uint32_t i = 0; i < VOLUME_ATTACHMENTS::SIZE; i++) {
    attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }

  attachmentDescs[VOLUME_ATTACHMENTS::DEPTH].format = depthTexture.format;
  attachmentDescs[VOLUME_ATTACHMENTS::DEPTH].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachmentDescs[VOLUME_ATTACHMENTS::DEPTH].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  attachmentDescs[VOLUME_ATTACHMENTS::SWAPCHAIN].format = mg::vkContext.swapChain->format;
  attachmentDescs[VOLUME_ATTACHMENTS::SWAPCHAIN].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachmentDescs[VOLUME_ATTACHMENTS::SWAPCHAIN].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  attachmentDescs[VOLUME_ATTACHMENTS::FRONT].format = frontTexture.format;
  attachmentDescs[VOLUME_ATTACHMENTS::BACK].format = backTexture.format;
  attachmentDescs[VOLUME_ATTACHMENTS::COLOR].format = colorTexture.format;

  // subpass 0
  VkAttachmentReference frontBackColorAttachments[] = {
      {VOLUME_ATTACHMENTS::FRONT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
      {VOLUME_ATTACHMENTS::BACK, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
  };

  VkSubpassDescription subpassFrontBack = {};
  subpassFrontBack.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassFrontBack.pColorAttachments = frontBackColorAttachments;
  subpassFrontBack.colorAttachmentCount = mg::countof(frontBackColorAttachments);
  subpassFrontBack.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

  // subpass 1
  VkAttachmentReference volumeInputAttachments[] = {
      {VOLUME_ATTACHMENTS::FRONT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
      {VOLUME_ATTACHMENTS::BACK, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
  };
  VkAttachmentReference volumeColorAttachments[] = {{VOLUME_ATTACHMENTS::COLOR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

  VkSubpassDescription subpassVolume = {};
  subpassVolume.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassVolume.colorAttachmentCount = mg::countof(volumeColorAttachments);
  subpassVolume.pColorAttachments = volumeColorAttachments;
  subpassVolume.inputAttachmentCount = mg::countof(volumeInputAttachments);
  subpassVolume.pInputAttachments = volumeInputAttachments;
  subpassVolume.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

  // subpass 2
  VkAttachmentReference denoiseInputAttachments[] = {{VOLUME_ATTACHMENTS::COLOR, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
  VkAttachmentReference denoiseColorAttachments[] = {{VOLUME_ATTACHMENTS::SWAPCHAIN, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};
  VkAttachmentReference denoiseDepthAttachment = {VOLUME_ATTACHMENTS::DEPTH, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subpassDenoise = {};
  subpassDenoise.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDenoise.colorAttachmentCount = mg::countof(denoiseColorAttachments);
  subpassDenoise.pColorAttachments = denoiseColorAttachments;
  subpassDenoise.inputAttachmentCount = mg::countof(denoiseInputAttachments);
  subpassDenoise.pInputAttachments = denoiseInputAttachments;
  subpassDenoise.pDepthStencilAttachment = &denoiseDepthAttachment;
  subpassDenoise.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

  // Render pass
  VkSubpassDependency dependencies[4] = {};

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = SUBPASSES::FRONT_BACK;
  dependencies[1].dstSubpass = SUBPASSES::VOLUME;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[2].srcSubpass = SUBPASSES::VOLUME;
  dependencies[2].dstSubpass = SUBPASSES::DENOISE;
  dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[3].srcSubpass = SUBPASSES::DENOISE;
  dependencies[3].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[3].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[3].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkSubpassDescription subpasses[] = {subpassFrontBack, subpassVolume, subpassDenoise};
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.pAttachments = attachmentDescs;
  renderPassInfo.attachmentCount = mg::countof(attachmentDescs);
  renderPassInfo.subpassCount = mg::countof(subpasses);
  renderPassInfo.pSubpasses = subpasses;
  renderPassInfo.dependencyCount = mg::countof(dependencies);
  renderPassInfo.pDependencies = dependencies;

  mg::checkResult(vkCreateRenderPass(mg::vkContext.device, &renderPassInfo, nullptr, &volumeRenderPass->vkRenderPass));
}

static void createFrameBuffer(VolumeRenderPass *volumeRenderPass) {
  const auto frontTexture = mg::getTexture("front");
  const auto backTexture = mg::getTexture("back");
  const auto colorTexture = mg::getTexture("color");
  const auto depthTexture = mg::getTexture("depth");

  VkImageView imageViews[VOLUME_ATTACHMENTS::SIZE] = {};
  imageViews[VOLUME_ATTACHMENTS::FRONT] = frontTexture.imageView;
  imageViews[VOLUME_ATTACHMENTS::BACK] = backTexture.imageView;
  imageViews[VOLUME_ATTACHMENTS::COLOR] = colorTexture.imageView;
  imageViews[VOLUME_ATTACHMENTS::DEPTH] = depthTexture.imageView;

  for (uint32_t i = 0; i < mg::vkContext.swapChain->numOfImages; i++) {
    imageViews[VOLUME_ATTACHMENTS::SWAPCHAIN] = mg::vkContext.swapChain->imageViews[i];
    VkFramebufferCreateInfo vkFramebufferCreateInfo = {};
    vkFramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    vkFramebufferCreateInfo.pNext = nullptr;
    vkFramebufferCreateInfo.renderPass = volumeRenderPass->vkRenderPass;
    vkFramebufferCreateInfo.pAttachments = imageViews;
    vkFramebufferCreateInfo.attachmentCount = mg::countof(imageViews);
    vkFramebufferCreateInfo.width = mg::vkContext.screen.width;
    vkFramebufferCreateInfo.height = mg::vkContext.screen.height;
    vkFramebufferCreateInfo.layers = 1;

    mg::checkResult(
        vkCreateFramebuffer(mg::vkContext.device, &vkFramebufferCreateInfo, nullptr, &volumeRenderPass->vkFrameBuffers[i]));
  }
}

static void destroyTextures() {
  mg::removeTexture("front");
  mg::removeTexture("back");
  mg::removeTexture("color");
  mg::removeTexture("depth");
}

static void destroyFrameBuffers(VolumeRenderPass *volumeRenderPass) {
  for (size_t i = 0; i < mg::vkContext.swapChain->numOfImages; i++) {
    vkDestroyFramebuffer(mg::vkContext.device, volumeRenderPass->vkFrameBuffers[i], nullptr);
  }
}

void initVolumeRenderPass(VolumeRenderPass *volumeRenderPass) {
  createTextures();
  createRenderPass(volumeRenderPass);
  createFrameBuffer(volumeRenderPass);
}
void resizeVolumeRenderPass(VolumeRenderPass *volumeRenderPass) {
  destroyFrameBuffers(volumeRenderPass);
  destroyTextures();
  createTextures();
  createFrameBuffer(volumeRenderPass);
}
void destroyVolumeRenderPass(VolumeRenderPass *volumeRenderPass) {
  destroyFrameBuffers(volumeRenderPass);
  destroyTextures();
  vkDestroyRenderPass(mg::vkContext.device, volumeRenderPass->vkRenderPass, nullptr);
}

void beginVolumeRenderPass(const VolumeRenderPass &volumeRenderPass) {
  VkClearValue clearValues[VOLUME_ATTACHMENTS::SIZE];
  clearValues[VOLUME_ATTACHMENTS::FRONT].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
  clearValues[VOLUME_ATTACHMENTS::BACK].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
  clearValues[VOLUME_ATTACHMENTS::COLOR].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
  clearValues[VOLUME_ATTACHMENTS::DEPTH].depthStencil = {1.0f, 0};
  clearValues[VOLUME_ATTACHMENTS::SWAPCHAIN].color = {{0.0f, 0.0f, 0.0f, 0.0f}};

  VkRenderPassBeginInfo renderPassBeginInfo = {};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.renderPass = volumeRenderPass.vkRenderPass;
  renderPassBeginInfo.framebuffer = volumeRenderPass.vkFrameBuffers[mg::vkContext.swapChain->currentSwapChainIndex];
  renderPassBeginInfo.renderArea.extent.width = mg::vkContext.screen.width;
  renderPassBeginInfo.renderArea.extent.height = mg::vkContext.screen.height;
  renderPassBeginInfo.clearValueCount = mg::countof(clearValues);
  renderPassBeginInfo.pClearValues = clearValues;

  vkCmdBeginRenderPass(mg::vkContext.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void endVolumeRenderPass() { vkCmdEndRenderPass(mg::vkContext.commandBuffer); }
