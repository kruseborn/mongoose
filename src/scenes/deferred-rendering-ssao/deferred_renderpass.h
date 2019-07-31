#include "vulkan/vkContext.h"
#include "vulkan/swapChain.h"
#include "mg/textureContainer.h"

namespace DEFERRED_ATTACHMENTS {
enum { NORMAL, ALBEDO, WORLD_POS, DEPTH, SSAO, SSAOBLUR, SWAPCHAIN, SIZE };
};

namespace SUBPASSES {
enum { MRT, SSAO, SSAO_BLUR, FINAL };
};

struct DeferredRenderPass {
  VkFramebuffer vkFrameBuffers[mg::MAX_SWAP_CHAIN_IMAGES];
  VkRenderPass vkRenderPass;
  mg::TextureId normal, albedo, worldViewPosition, ssao, ssaoBlur, depth;
};

void initDeferredRenderPass(DeferredRenderPass *deferredRenderPass);
void resizeDeferredRenderPass(DeferredRenderPass *deferredRenderPass);
void destroyDeferredRenderPass(DeferredRenderPass *deferredRenderPass);

void beginDeferredRenderPass(const DeferredRenderPass &deferredRenderPass);
void endDeferredRenderPass();
