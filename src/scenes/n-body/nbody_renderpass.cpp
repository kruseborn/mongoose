#include "nbody_renderpass.h"

#include "mg/mgSystem.h"
#include "mg/textureContainer.h"
#include "vulkan/vkContext.h"

static void createTextures(NBodyRenderPass *nBodyRenderPass) {
  mg::CreateTextureInfo createTextureInfo = {};

  createTextureInfo.type = mg::TEXTURE_TYPE::ATTACHMENT;
  createTextureInfo.size = {mg::vkContext.screen.width, mg::vkContext.screen.height, 1};

  createTextureInfo.id = "toneMapping";
  createTextureInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  nBodyRenderPass->toneMapping = mg::mgSystem.textureContainer.createTexture(createTextureInfo);
}

static void createRenderPass(NBodyRenderPass *nBodyRenderPass) {
  const auto toneTexture = mg::getTexture(nBodyRenderPass->toneMapping);

  // attachments
  VkAttachmentDescription attachmentDescs[NBODY_ATTACHMENTS::SIZE] = {};
  for (uint32_t i = 0; i < NBODY_ATTACHMENTS::SIZE; i++) {
    attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }
  attachmentDescs[NBODY_ATTACHMENTS::TONE_MAPPING].format = toneTexture.format;

  attachmentDescs[NBODY_ATTACHMENTS::SWAPCHAIN].format = mg::vkContext.swapChain->format;
  attachmentDescs[NBODY_ATTACHMENTS::SWAPCHAIN].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachmentDescs[NBODY_ATTACHMENTS::SWAPCHAIN].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


  // subpass 0
  VkAttachmentReference particlesColorAttachments[] = {
      {NBODY_ATTACHMENTS::TONE_MAPPING, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
  };

  VkSubpassDescription subpassParticles = {};
  subpassParticles.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassParticles.pColorAttachments = particlesColorAttachments;
  subpassParticles.colorAttachmentCount = mg::countof(particlesColorAttachments);

  // subpass 1
  VkAttachmentReference toneMappingInputAttachments[] = {
      {NBODY_ATTACHMENTS::TONE_MAPPING, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
  };
  VkAttachmentReference toneMappingColorAttachments[] = {
      {NBODY_ATTACHMENTS::SWAPCHAIN, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
  };

  VkSubpassDescription subpassToneMapping = {};
  subpassToneMapping.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassToneMapping.inputAttachmentCount = mg::countof(toneMappingInputAttachments);
  subpassToneMapping.pInputAttachments = toneMappingInputAttachments;
  subpassToneMapping.colorAttachmentCount = mg::countof(toneMappingColorAttachments);
  subpassToneMapping.pColorAttachments = toneMappingColorAttachments;

  // Render pass
  VkSubpassDependency dependencies[3] = {};

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = SUBPASSES::PARTICLES;
  dependencies[1].dstSubpass = SUBPASSES::TONE_MAPPING;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[2].srcSubpass = SUBPASSES::TONE_MAPPING;
  dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkSubpassDescription subpasses[] = {subpassParticles, subpassToneMapping};
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.pAttachments = attachmentDescs;
  renderPassInfo.attachmentCount = mg::countof(attachmentDescs);
  renderPassInfo.pSubpasses = subpasses;
  renderPassInfo.subpassCount = mg::countof(subpasses);
  renderPassInfo.dependencyCount = mg::countof(dependencies);
  renderPassInfo.pDependencies = dependencies;

  mg::checkResult(vkCreateRenderPass(mg::vkContext.device, &renderPassInfo, nullptr, &nBodyRenderPass->vkRenderPass));
}

static void createFrameBuffer(NBodyRenderPass *nBodyRenderPass) {
  const auto toneMappingTexture = mg::getTexture(nBodyRenderPass->toneMapping);

  VkImageView imageViews[NBODY_ATTACHMENTS::SIZE] = {};
  imageViews[NBODY_ATTACHMENTS::TONE_MAPPING] = toneMappingTexture.imageView;

  for (uint32_t i = 0; i < mg::vkContext.swapChain->numOfImages; i++) {
    imageViews[NBODY_ATTACHMENTS::SWAPCHAIN] = mg::vkContext.swapChain->imageViews[i];
    VkFramebufferCreateInfo vkFramebufferCreateInfo = {};
    vkFramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    vkFramebufferCreateInfo.pNext = nullptr;
    vkFramebufferCreateInfo.renderPass = nBodyRenderPass->vkRenderPass;
    vkFramebufferCreateInfo.pAttachments = imageViews;
    vkFramebufferCreateInfo.attachmentCount = mg::countof(imageViews);
    vkFramebufferCreateInfo.width = mg::vkContext.screen.width;
    vkFramebufferCreateInfo.height = mg::vkContext.screen.height;
    vkFramebufferCreateInfo.layers = 1;

    mg::checkResult(
        vkCreateFramebuffer(mg::vkContext.device, &vkFramebufferCreateInfo, nullptr, &nBodyRenderPass->vkFrameBuffers[i]));
  }
}

static void destroyTextures(NBodyRenderPass *nBodyRenderPass) { mg::removeTexture(nBodyRenderPass->toneMapping); }

static void destroyFrameBuffers(NBodyRenderPass *nBodyRenderPass) {
  for (size_t i = 0; i < mg::vkContext.swapChain->numOfImages; i++) {
    vkDestroyFramebuffer(mg::vkContext.device, nBodyRenderPass->vkFrameBuffers[i], nullptr);
  }
}

void initNBodyRenderPass(NBodyRenderPass *nBodyRenderPass) {
  createTextures(nBodyRenderPass);
  createRenderPass(nBodyRenderPass);
  createFrameBuffer(nBodyRenderPass);
}
void resizeNBodyRenderPass(NBodyRenderPass *nBodyRenderPass) {
  destroyFrameBuffers(nBodyRenderPass);
  destroyTextures(nBodyRenderPass);
  createTextures(nBodyRenderPass);
  createFrameBuffer(nBodyRenderPass);
}
void destroyNBodyRenderPass(NBodyRenderPass *nBodyRenderPass) {
  destroyFrameBuffers(nBodyRenderPass);
  destroyTextures(nBodyRenderPass);
  vkDestroyRenderPass(mg::vkContext.device, nBodyRenderPass->vkRenderPass, nullptr);
}

void beginNBodyRenderPass(const NBodyRenderPass &nBodyRenderPass) {
  VkClearValue clearValues[NBODY_ATTACHMENTS::SIZE];
  clearValues[NBODY_ATTACHMENTS::TONE_MAPPING].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
  clearValues[NBODY_ATTACHMENTS::SWAPCHAIN].color = {{0.0f, 0.0f, 0.0f, 0.0f}};

  VkRenderPassBeginInfo renderPassBeginInfo = {};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.renderPass = nBodyRenderPass.vkRenderPass;
  renderPassBeginInfo.framebuffer = nBodyRenderPass.vkFrameBuffers[mg::vkContext.swapChain->currentSwapChainIndex];
  renderPassBeginInfo.renderArea.extent.width = mg::vkContext.screen.width;
  renderPassBeginInfo.renderArea.extent.height = mg::vkContext.screen.height;
  renderPassBeginInfo.clearValueCount = mg::countof(clearValues);
  renderPassBeginInfo.pClearValues = clearValues;

  vkCmdBeginRenderPass(mg::vkContext.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void endNBodyRenderPass() { vkCmdEndRenderPass(mg::vkContext.commandBuffer); }
