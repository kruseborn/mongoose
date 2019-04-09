#pragma once

#include "swapChain.h"
#include "vkContext.h"
#include <array>

namespace mg {
namespace SINGLEPASS_ATTACHMENTS {
enum { SWAPCHAIN, DEPTH, SIZE };
}

struct SingleRenderPass {
  VkFramebuffer vkFrameBuffers[mg::MAX_SWAP_CHAIN_IMAGES];
  VkRenderPass vkRenderPass;
};

void initSingleRenderPass(SingleRenderPass *singleRenderPass);
void resizeSingleRenderPass(SingleRenderPass *singleRenderPass);
void destroySingleRenderPass(SingleRenderPass *singleRenderPass);

void beginSingleRenderPass(const SingleRenderPass &singleRenderPass);
void endSingleRenderPass();

} // namespace mg
