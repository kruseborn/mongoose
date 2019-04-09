#include "vulkan/vkContext.h"
#include "vulkan/swapChain.h"

namespace VOLUME_ATTACHMENTS {
enum { FRONT, BACK, COLOR, SWAPCHAIN, DEPTH, SIZE };
};

namespace SUBPASSES {
enum { FRONT_BACK, VOLUME, DENOISE };
};

struct VolumeRenderPass {
  VkFramebuffer vkFrameBuffers[mg::MAX_SWAP_CHAIN_IMAGES];
  VkRenderPass vkRenderPass;
};

void initVolumeRenderPass(VolumeRenderPass *volumeRenderPass);
void resizeVolumeRenderPass(VolumeRenderPass *volumeRenderPass);
void destroyVolumeRenderPass(VolumeRenderPass *volumeRenderPass);

void beginVolumeRenderPass(const VolumeRenderPass &VolumeRenderPass);
void endVolumeRenderPass();
