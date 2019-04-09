#include "deferred_renderpass.h"

#include "mg/mgSystem.h"
#include "mg/textureContainer.h"
#include "vulkan/vkContext.h"

static void createTextures() {
  mg::CreateTextureInfo createTextureInfo = {};

  createTextureInfo.textureSamplers = {mg::TEXTURE_SAMPLER::LINEAR_CLAMP_TO_BORDER};
  createTextureInfo.type = mg::TEXTURE_TYPE::ATTACHMENT;
  createTextureInfo.size = {mg::vkContext.screen.width, mg::vkContext.screen.height, 1};

  createTextureInfo.id = "normal";
  createTextureInfo.format = VK_FORMAT_R16G16_SFLOAT;
  mg::mgSystem.textureContainer.createTexture(createTextureInfo);

  createTextureInfo.id = "albedo";
  createTextureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  mg::mgSystem.textureContainer.createTexture(createTextureInfo);

  createTextureInfo.id = "word view position";
  createTextureInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  mg::mgSystem.textureContainer.createTexture(createTextureInfo);

  createTextureInfo.id = "ssao";
  createTextureInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  mg::mgSystem.textureContainer.createTexture(createTextureInfo);

  createTextureInfo.id = "ssaoblur";
  createTextureInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  mg::mgSystem.textureContainer.createTexture(createTextureInfo);

  createTextureInfo.id = "depth";
  createTextureInfo.type = mg::TEXTURE_TYPE::DEPTH;
  createTextureInfo.format = mg::vkContext.formats.depth;
  mg::mgSystem.textureContainer.createTexture(createTextureInfo);
}

static void createRenderPass(DeferredRenderPass *deferredRenderPass) {
  const auto normalTexture = mg::getTexture("normal");
  const auto albedoTexture = mg::getTexture("albedo");
  const auto wordViewPostionTexture = mg::getTexture("word view position");
  const auto depthTexture = mg::getTexture("depth");
  const auto ssaoTexture = mg::getTexture("ssao");
  const auto ssaoblurTexture = mg::getTexture("ssaoblur");

  // attachments
  VkAttachmentDescription attachmentDescs[DEFERRED_ATTACHMENTS::SIZE] = {};
  for (uint32_t i = 0; i < DEFERRED_ATTACHMENTS::SIZE; i++) {
    attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }

  attachmentDescs[DEFERRED_ATTACHMENTS::DEPTH].format = depthTexture.format;
  attachmentDescs[DEFERRED_ATTACHMENTS::DEPTH].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachmentDescs[DEFERRED_ATTACHMENTS::DEPTH].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  attachmentDescs[DEFERRED_ATTACHMENTS::SWAPCHAIN].format = mg::vkContext.swapChain->format;
  attachmentDescs[DEFERRED_ATTACHMENTS::SWAPCHAIN].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachmentDescs[DEFERRED_ATTACHMENTS::SWAPCHAIN].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  attachmentDescs[DEFERRED_ATTACHMENTS::NORMAL].format = normalTexture.format;
  attachmentDescs[DEFERRED_ATTACHMENTS::ALBEDO].format = albedoTexture.format;
  attachmentDescs[DEFERRED_ATTACHMENTS::WORLD_POS].format = wordViewPostionTexture.format;
  attachmentDescs[DEFERRED_ATTACHMENTS::SSAO].format = ssaoTexture.format;
  attachmentDescs[DEFERRED_ATTACHMENTS::SSAOBLUR].format = ssaoblurTexture.format;

  // subpass 0
  VkAttachmentReference mrtDepthAttachment = {};
  mrtDepthAttachment.attachment = DEFERRED_ATTACHMENTS::DEPTH;
  mrtDepthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference mrtColorAttachments[] = {
      {DEFERRED_ATTACHMENTS::NORMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
      {DEFERRED_ATTACHMENTS::ALBEDO, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
      {DEFERRED_ATTACHMENTS::WORLD_POS, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
  };

  VkSubpassDescription subpassMRT = {};
  subpassMRT.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassMRT.pColorAttachments = mrtColorAttachments;
  subpassMRT.colorAttachmentCount = mg::countof(mrtColorAttachments);
  subpassMRT.pDepthStencilAttachment = &mrtDepthAttachment;
  subpassMRT.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

  // subpass 1
  VkAttachmentReference ssaoInputAttachments[] = {
      {DEFERRED_ATTACHMENTS::NORMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
      {DEFERRED_ATTACHMENTS::WORLD_POS, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
  };
  VkAttachmentReference ssaoColorAttachments[1] = {{DEFERRED_ATTACHMENTS::SSAO, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};
  uint32_t ssaoPreservedAttachments[] = {DEFERRED_ATTACHMENTS::ALBEDO};

  VkSubpassDescription subpassSSAO = {};
  subpassSSAO.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassSSAO.colorAttachmentCount = mg::countof(ssaoColorAttachments);
  subpassSSAO.pColorAttachments = ssaoColorAttachments;
  subpassSSAO.inputAttachmentCount = mg::countof(ssaoInputAttachments);
  subpassSSAO.pInputAttachments = ssaoInputAttachments;
  subpassSSAO.preserveAttachmentCount = mg::countof(ssaoPreservedAttachments);
  subpassSSAO.pPreserveAttachments = ssaoPreservedAttachments;
  subpassSSAO.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

  // subpass 2
  VkAttachmentReference ssaoBlurInputAttachments[1] = {{DEFERRED_ATTACHMENTS::SSAO, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
  VkAttachmentReference ssaoBlurColorAttachments[1] = {{DEFERRED_ATTACHMENTS::SSAOBLUR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

  uint32_t ssaoBLurPreservedAttachments[] = {DEFERRED_ATTACHMENTS::NORMAL, DEFERRED_ATTACHMENTS::ALBEDO};

  VkSubpassDescription subpassSSAOBlur = {};
  subpassSSAOBlur.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassSSAOBlur.colorAttachmentCount = mg::countof(ssaoBlurColorAttachments);
  subpassSSAOBlur.pColorAttachments = ssaoBlurColorAttachments;
  subpassSSAOBlur.inputAttachmentCount = mg::countof(ssaoBlurInputAttachments);
  subpassSSAOBlur.pInputAttachments = ssaoBlurInputAttachments;
  subpassSSAOBlur.preserveAttachmentCount = mg::countof(ssaoBLurPreservedAttachments);
  subpassSSAOBlur.pPreserveAttachments = ssaoBLurPreservedAttachments;
  subpassSSAOBlur.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;


  // subpass 3
  VkAttachmentReference finalInputAttachments[] = {{DEFERRED_ATTACHMENTS::NORMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                                                   {DEFERRED_ATTACHMENTS::ALBEDO, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                                                   {DEFERRED_ATTACHMENTS::SSAOBLUR, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}, 
                                                       {DEFERRED_ATTACHMENTS::DEPTH, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};

  VkAttachmentReference finalColorAttachments[] = {{DEFERRED_ATTACHMENTS::SWAPCHAIN, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

  VkSubpassDescription subpassFinal = {};
  subpassFinal.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassFinal.colorAttachmentCount = mg::countof(finalColorAttachments);
  subpassFinal.pColorAttachments = finalColorAttachments;
  subpassFinal.inputAttachmentCount = mg::countof(finalInputAttachments);
  subpassFinal.pInputAttachments = finalInputAttachments;
  subpassFinal.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

  // Render pass
  VkSubpassDependency dependencies[6] = {};

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = SUBPASSES::MRT;
  dependencies[1].dstSubpass = SUBPASSES::SSAO;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[2].srcSubpass = SUBPASSES::MRT;
  dependencies[2].dstSubpass = SUBPASSES::SSAO;
  dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[3].srcSubpass = SUBPASSES::SSAO;
  dependencies[3].dstSubpass = SUBPASSES::SSAO_BLUR;
  dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[3].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[4].srcSubpass = SUBPASSES::SSAO_BLUR;
  dependencies[4].dstSubpass = SUBPASSES::FINAL;
  dependencies[4].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[4].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[4].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[4].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[4].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[5].srcSubpass = SUBPASSES::FINAL;
  dependencies[5].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[5].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[5].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[5].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[5].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[5].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkSubpassDescription subpasses[4] = {subpassMRT, subpassSSAO, subpassSSAOBlur, subpassFinal};
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.pAttachments = attachmentDescs;
  renderPassInfo.attachmentCount = mg::countof(attachmentDescs);
  renderPassInfo.subpassCount = mg::countof(subpasses);
  renderPassInfo.pSubpasses = subpasses;
  renderPassInfo.dependencyCount = mg::countof(dependencies);
  renderPassInfo.pDependencies = dependencies;

  mg::checkResult(vkCreateRenderPass(mg::vkContext.device, &renderPassInfo, nullptr, &deferredRenderPass->vkRenderPass));
}

static void createFrameBuffer(DeferredRenderPass *deferredRenderPass) {
  const auto normalTexture = mg::getTexture("normal");
  const auto albedoTexture = mg::getTexture("albedo");
  const auto wordViewPostionTexture = mg::getTexture("word view position");
  const auto depthTexture = mg::getTexture("depth");
  const auto ssaoTexture = mg::getTexture("ssao");
  const auto ssaoblur = mg::getTexture("ssaoblur");

  VkImageView imageViews[DEFERRED_ATTACHMENTS::SIZE] = {};
  imageViews[DEFERRED_ATTACHMENTS::NORMAL] = normalTexture.imageView;
  imageViews[DEFERRED_ATTACHMENTS::ALBEDO] = albedoTexture.imageView;
  imageViews[DEFERRED_ATTACHMENTS::WORLD_POS] = wordViewPostionTexture.imageView;
  imageViews[DEFERRED_ATTACHMENTS::DEPTH] = depthTexture.imageView;
  imageViews[DEFERRED_ATTACHMENTS::SSAO] = ssaoTexture.imageView;
  imageViews[DEFERRED_ATTACHMENTS::SSAOBLUR] = ssaoblur.imageView;

  for (uint32_t i = 0; i < mg::vkContext.swapChain->numOfImages; i++) {
    imageViews[DEFERRED_ATTACHMENTS::SWAPCHAIN] = mg::vkContext.swapChain->imageViews[i];
    VkFramebufferCreateInfo vkFramebufferCreateInfo = {};
    vkFramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    vkFramebufferCreateInfo.pNext = nullptr;
    vkFramebufferCreateInfo.renderPass = deferredRenderPass->vkRenderPass;
    vkFramebufferCreateInfo.pAttachments = imageViews;
    vkFramebufferCreateInfo.attachmentCount = mg::countof(imageViews);
    vkFramebufferCreateInfo.width = mg::vkContext.screen.width;
    vkFramebufferCreateInfo.height = mg::vkContext.screen.height;
    vkFramebufferCreateInfo.layers = 1;

    mg::checkResult(vkCreateFramebuffer(mg::vkContext.device, &vkFramebufferCreateInfo, nullptr, &deferredRenderPass->vkFrameBuffers[i]));
  }
}

static void destroyTextures() {
  mg::removeTexture("normal");
  mg::removeTexture("albedo");
  mg::removeTexture("depth");
  mg::removeTexture("word view position");
  mg::removeTexture("ssao");
  mg::removeTexture("ssaoblur");
}

static void destroyFrameBuffers(DeferredRenderPass *deferredRenderPass) {
  for (size_t i = 0; i < mg::vkContext.swapChain->numOfImages; i++) {
    vkDestroyFramebuffer(mg::vkContext.device, deferredRenderPass->vkFrameBuffers[i], nullptr);
  }
}

void initDeferredRenderPass(DeferredRenderPass *deferredRenderPass) {
  createTextures();
  createRenderPass(deferredRenderPass);
  createFrameBuffer(deferredRenderPass);
}
void resizeDeferredRenderPass(DeferredRenderPass *deferredRenderPass) {
  destroyFrameBuffers(deferredRenderPass);
  destroyTextures();
  createTextures();
  createFrameBuffer(deferredRenderPass);
}
void destroyDeferredRenderPass(DeferredRenderPass *deferredRenderPass) {
  destroyFrameBuffers(deferredRenderPass);
  destroyTextures();
  vkDestroyRenderPass(mg::vkContext.device, deferredRenderPass->vkRenderPass, nullptr);
}

void beginDeferredRenderPass(const DeferredRenderPass &deferredRenderPass) {
  VkClearValue clearValues[DEFERRED_ATTACHMENTS::SIZE];
  clearValues[DEFERRED_ATTACHMENTS::NORMAL].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
  clearValues[DEFERRED_ATTACHMENTS::ALBEDO].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
  clearValues[DEFERRED_ATTACHMENTS::WORLD_POS].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
  clearValues[DEFERRED_ATTACHMENTS::DEPTH].depthStencil = {1.0f, 0};
  clearValues[DEFERRED_ATTACHMENTS::SWAPCHAIN].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
  clearValues[DEFERRED_ATTACHMENTS::SSAO].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
  clearValues[DEFERRED_ATTACHMENTS::SSAOBLUR].color = {{0.0f, 0.0f, 0.0f, 0.0f}};

  VkRenderPassBeginInfo renderPassBeginInfo = {};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.renderPass = deferredRenderPass.vkRenderPass;
  renderPassBeginInfo.framebuffer = deferredRenderPass.vkFrameBuffers[mg::vkContext.swapChain->currentSwapChainIndex];
  renderPassBeginInfo.renderArea.extent.width = mg::vkContext.screen.width;
  renderPassBeginInfo.renderArea.extent.height = mg::vkContext.screen.height;
  renderPassBeginInfo.clearValueCount = mg::countof(clearValues);
  renderPassBeginInfo.pClearValues = clearValues;

  vkCmdBeginRenderPass(mg::vkContext.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void endDeferredRenderPass() { vkCmdEndRenderPass(mg::vkContext.commandBuffer); }
