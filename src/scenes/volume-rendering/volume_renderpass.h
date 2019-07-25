#include "vulkan/vkContext.h"
#include "vulkan/swapChain.h"
#include "mg/textureContainer.h"

namespace VOLUME_ATTACHMENTS {
enum { BACK, FRONT, COLOR, SWAPCHAIN, DEPTH, SIZE };
};

namespace SUBPASSES {
enum { FRONT_BACK, VOLUME, DENOISE };
};

struct VolumeRenderPass {
  VkFramebuffer vkFrameBuffers[mg::MAX_SWAP_CHAIN_IMAGES];
  VkRenderPass vkRenderPass;
  mg::TextureId front, back, color, depth;
};

void initVolumeRenderPass(VolumeRenderPass *volumeRenderPass);
void resizeVolumeRenderPass(VolumeRenderPass *volumeRenderPass);
void destroyVolumeRenderPass(VolumeRenderPass *volumeRenderPass);

void beginVolumeRenderPass(const VolumeRenderPass &VolumeRenderPass);
void endVolumeRenderPass();
