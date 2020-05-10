#include "empty_render_pass.h"
#include "mg/textureContainer.h"
#include "rendering/rendering.h"

#include "mg/mgSystem.h"

namespace mg {

static uint32_t width = 8;
static uint32_t height = 8;

static void createFrameBuffers(EmptyRenderPass *emptyRenderPass, float size) {
  VkFramebufferCreateInfo framebufferCreateInfo = {};
  framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferCreateInfo.renderPass = emptyRenderPass->vkRenderPass;
  framebufferCreateInfo.width = width;
  framebufferCreateInfo.height = height;
  framebufferCreateInfo.layers = 1;

  checkResult(
      vkCreateFramebuffer(mg::vkContext.device, &framebufferCreateInfo, nullptr, &emptyRenderPass->vkFrameBuffer));
}

static void createRenderPass(EmptyRenderPass *emptyRenderPass) {
  VkSubpassDescription subPassDescription = {.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS};
  VkSubpassDependency subpassDependency = {.srcSubpass = 0,
                                           .dstSubpass = VK_SUBPASS_EXTERNAL,
                                           .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                           .dstStageMask = VK_PIPELINE_STAGE_HOST_BIT,
                                           .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                                           .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
                                           .dependencyFlags = VK_DEPENDENCY_DEVICE_GROUP_BIT};

  // Create the render pass
  VkRenderPassCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  createInfo.subpassCount = 1;
  createInfo.pSubpasses = &subPassDescription;
  createInfo.dependencyCount = 1;
  createInfo.pDependencies = &subpassDependency;

  checkResult(vkCreateRenderPass(mg::vkContext.device, &createInfo, nullptr, &emptyRenderPass->vkRenderPass));
}

void initEmptyRenderPass(EmptyRenderPass *emptyRenderPass, uint32_t frameBufferSize) {
  createRenderPass(emptyRenderPass);
  createFrameBuffers(emptyRenderPass, float(frameBufferSize));
}

void destroyEmptyRenderPass(EmptyRenderPass *emptyRenderPass) {
  vkDestroyFramebuffer(mg::vkContext.device, emptyRenderPass->vkFrameBuffer, nullptr);
  vkDestroyRenderPass(mg::vkContext.device, emptyRenderPass->vkRenderPass, nullptr);
}

void beginEmptyRenderPass(const EmptyRenderPass &emptyRenderPass, float viewPortSize, VkCommandBuffer commandBuffer) {
  VkRenderPassBeginInfo renderPassBeginInfo = {};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.renderPass = emptyRenderPass.vkRenderPass;
  renderPassBeginInfo.framebuffer = emptyRenderPass.vkFrameBuffer;
  renderPassBeginInfo.renderArea.extent.width = width;
  renderPassBeginInfo.renderArea.extent.height = height;

  vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
  VkViewport viewport = {};
  viewport.x = 0;
  viewport.y = 0;
  viewport.width = float(width);
  viewport.height = float(height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  VkRect2D scissor = {};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent.width = width;
  scissor.extent.height = height;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}
void endRenderEmptyRenderPass(VkCommandBuffer commandBuffer) {
  vkCmdEndRenderPass(commandBuffer);
}

} // namespace mg

//#include "empty_render_pass.h"
//#include "vulkan/vkUtils.h"
//
// namespace mg {
//
// void initEmptyRenderPass(EmptyRenderPass *emptyRenderPass, uint32_t frameBufferSize) {
//  VkSubpassDescription subPassDescription = {.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS};
//
//  // clang-format off
//  VkSubpassDependency subpassDependency = {
//      .srcSubpass = 0,
//      .dstSubpass = VK_SUBPASS_EXTERNAL,
//      .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
//      .dstStageMask = VK_PIPELINE_STAGE_HOST_BIT,
//      .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
//      .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
//      .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
//  };
//
//  VkRenderPassCreateInfo createInfo = {
//      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
//      .subpassCount = 1,
//      .pSubpasses = &subPassDescription,
//      .dependencyCount = 1,
//      .pDependencies = &subpassDependency
//  };
//  checkResult(vkCreateRenderPass(mg::vkContext.device, &createInfo, nullptr, &emptyRenderPass->vkRenderPass));
//
//  VkFramebufferCreateInfo framebufferCreateInfo = {
//    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
//    .renderPass = emptyRenderPass->vkRenderPass,
//    .width = frameBufferSize,
//    .height = frameBufferSize,
//    .layers = 1
//  };
//  // clang-format on
//
//  checkResult(
//      vkCreateFramebuffer(mg::vkContext.device, &framebufferCreateInfo, nullptr, &emptyRenderPass->vkFrameBuffer));
//}
//
// void destroyEmptyRenderPass(EmptyRenderPass *emptyRenderPass) {
//  vkDestroyRenderPass(mg::vkContext.device, emptyRenderPass->vkRenderPass, nullptr);
//  vkDestroyFramebuffer(mg::vkContext.device, emptyRenderPass->vkFrameBuffer, nullptr);
//}
//
// void beginEmptyRenderPass(const EmptyRenderPass &emptyRenderPass, float viewPortSize, VkCommandBuffer commandBuffer)
// {
//  VkRenderPassBeginInfo renderPassBeginInfo = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
//                                               .renderPass = emptyRenderPass.vkRenderPass,
//                                               .framebuffer = emptyRenderPass.vkFrameBuffer};
//
//  vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
//  setViewPort(commandBuffer, 0, 0, viewPortSize, viewPortSize, 0.0f, 1.0f);
//  VkRect2D scissor = {};
//  scissor.offset.x = 0;
//  scissor.offset.y = 0;
//  scissor.extent.width = uint32_t(viewPortSize + 0.5f);
//  scissor.extent.height = uint32_t(viewPortSize + 0.5f);
//  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
//}
//
// void endRenderEmptyRenderPass(VkCommandBuffer commandBuffer) {
//  vkCmdEndRenderPass(commandBuffer);
//}
//
//} // namespace mg