#include "empty_render_pass.h"
#include "mg/textureContainer.h"
#include "rendering/rendering.h"

#include "mg/mgSystem.h"

namespace mg {

static void createFrameBuffers(EmptyRenderPass *emptyRenderPass, uint32_t size) {
  VkFramebufferCreateInfo framebufferCreateInfo = {};
  framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferCreateInfo.renderPass = emptyRenderPass->vkRenderPass;
  framebufferCreateInfo.width = size;
  framebufferCreateInfo.height = size;
  framebufferCreateInfo.layers = 1;

  checkResult(
      vkCreateFramebuffer(mg::vkContext.device, &framebufferCreateInfo, nullptr, &emptyRenderPass->vkFrameBuffer));
}

static void createRenderPass(EmptyRenderPass *emptyRenderPass) {
  VkSubpassDescription subPassDescription = {.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS};
  VkSubpassDependency subpassDependency[2] = {
      {.srcSubpass = VK_SUBPASS_EXTERNAL,
       .dstSubpass = 0,
       .srcStageMask = VK_PIPELINE_STAGE_HOST_BIT,
       .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
       .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
       .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
       .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT},
      {.srcSubpass = 0,
       .dstSubpass = VK_SUBPASS_EXTERNAL,
       .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
       .dstStageMask = VK_PIPELINE_STAGE_HOST_BIT,
       .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
       .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
       .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT},
  };

  // Create the render pass
  VkRenderPassCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  createInfo.subpassCount = 1;
  createInfo.pSubpasses = &subPassDescription;
  createInfo.dependencyCount = 2;
  createInfo.pDependencies = subpassDependency;

  checkResult(vkCreateRenderPass(mg::vkContext.device, &createInfo, nullptr, &emptyRenderPass->vkRenderPass));
}

void initEmptyRenderPass(EmptyRenderPass *emptyRenderPass, uint32_t frameBufferSize) {
  createRenderPass(emptyRenderPass);
  createFrameBuffers(emptyRenderPass, frameBufferSize);
}

void destroyEmptyRenderPass(EmptyRenderPass *emptyRenderPass) {
  vkDestroyFramebuffer(mg::vkContext.device, emptyRenderPass->vkFrameBuffer, nullptr);
  vkDestroyRenderPass(mg::vkContext.device, emptyRenderPass->vkRenderPass, nullptr);
}

void beginEmptyRenderPass(const EmptyRenderPass &emptyRenderPass, uint32_t viewPortSize, VkCommandBuffer commandBuffer) {
  VkRenderPassBeginInfo renderPassBeginInfo = {};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.renderPass = emptyRenderPass.vkRenderPass;
  renderPassBeginInfo.framebuffer = emptyRenderPass.vkFrameBuffer;
  renderPassBeginInfo.renderArea.extent.width = viewPortSize;
  renderPassBeginInfo.renderArea.extent.height = viewPortSize;

  vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
  VkViewport viewport = {};
  viewport.x = 0;
  viewport.y = 0;
  viewport.width = float(viewPortSize);
  viewport.height = float(viewPortSize);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  VkRect2D scissor = {};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent.width = viewPortSize;
  scissor.extent.height = viewPortSize;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}
void endRenderEmptyRenderPass(VkCommandBuffer commandBuffer) {
  vkCmdEndRenderPass(commandBuffer);
}

} // namespace mg

