#include "vulkan/vkContext.h"
#include "vulkan/swapChain.h"
#include "mg/textureContainer.h"

namespace VOLUME_ATTACHMENTS {

};

namespace SUBPASSES {

};

struct VolumeRenderPass {

};

void initVolumeRenderPass(VolumeRenderPass *volumeRenderPass);
void resizeVolumeRenderPass(VolumeRenderPass *volumeRenderPass);
void destroyVolumeRenderPass(VolumeRenderPass *volumeRenderPass);

void beginVolumeRenderPass(const VolumeRenderPass &VolumeRenderPass);
void endVolumeRenderPass();
