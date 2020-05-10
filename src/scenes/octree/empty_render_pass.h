#pragma once

#include "vulkan/vkContext.h"

namespace mg {
struct EmptyRenderPass {
  VkRenderPass vkRenderPass;
  VkFramebuffer vkFrameBuffer;
};

void initEmptyRenderPass(EmptyRenderPass *emptyRenderPass, uint32_t frameBufferSize);
void destroyEmptyRenderPass(EmptyRenderPass *emptyRenderPass);
void beginEmptyRenderPass(const EmptyRenderPass &emptyRenderPass, float viewPortSize, VkCommandBuffer commandBuffer);
void endRenderEmptyRenderPass(VkCommandBuffer commandBuffer);

} // namespace mg