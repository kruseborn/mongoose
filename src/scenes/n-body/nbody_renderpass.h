#include "vulkan/vkContext.h"
#include "vulkan/swapChain.h"
#include "mg/textureContainer.h"

namespace NBODY_ATTACHMENTS {
enum { TONE_MAPPING, SWAPCHAIN, SIZE };
};

namespace SUBPASSES {
enum { PARTICLES, TONE_MAPPING };
};

struct NBodyRenderPass {
  VkFramebuffer vkFrameBuffers[mg::MAX_SWAP_CHAIN_IMAGES];
  VkRenderPass vkRenderPass;
  mg::TextureId toneMapping;
};

void initNBodyRenderPass(NBodyRenderPass *nBodyRenderPass);
void resizeNBodyRenderPass(NBodyRenderPass *nBodyRenderPass);
void destroyNBodyRenderPass(NBodyRenderPass *nBodyRenderPass);

void beginNBodyRenderPass(const NBodyRenderPass &nBodyRenderPass);
void endNBodyRenderPass();
