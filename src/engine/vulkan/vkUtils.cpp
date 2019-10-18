#include "vkUtils.h"
#include "mg/mgSystem.h"
#include "mg/mgUtils.h"
#include "vkContext.h"
#include <lodepng.h>

namespace mg {

// Vulkan Spec 10.2. Device Memory
// Find a memory in `memoryTypeBitsRequirement` that includes all of `requiredProperties
static int32_t _findMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties &memoryProperties,
                                    uint32_t memoryTypeBitsRequirement, VkMemoryPropertyFlags requiredProperties) {
  const uint32_t memoryCount = memoryProperties.memoryTypeCount;
  for (uint32_t memoryIndex = 0; memoryIndex < memoryCount; ++memoryIndex) {
    const uint32_t memoryTypeBits = (1 << memoryIndex);
    const bool isRequiredMemoryType = memoryTypeBitsRequirement & memoryTypeBits;

    const VkMemoryPropertyFlags properties = memoryProperties.memoryTypes[memoryIndex].propertyFlags;
    const bool hasRequiredProperties = (properties & requiredProperties) == requiredProperties;

    if (isRequiredMemoryType && hasRequiredProperties)
      return static_cast<int32_t>(memoryIndex);
  }
  return -1;
}

int32_t findMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties &memoryProperties,
                            uint32_t memoryTypeBitsRequirement, VkMemoryPropertyFlags requiredProperties,
                            VkMemoryPropertyFlags preferredProperties) {
  auto memoryTypeIndex = _findMemoryTypeIndex(memoryProperties, memoryTypeBitsRequirement,
                                              preferredProperties == 0 ? requiredProperties : preferredProperties);
  if (memoryTypeIndex == -1) {
    memoryTypeIndex = _findMemoryTypeIndex(memoryProperties, memoryTypeBitsRequirement, requiredProperties);
    mgAssert(memoryTypeIndex > -1);
  }
  return memoryTypeIndex;
}

void setViewPort(float x, float y, float width, float height, float minDepth, float maxDepth) {
  // the vulkan viewport has it origin in the top left corner, so we we flipp the viewport and move the origin to the
  // bottom left VK_KHR_maintenance1 or VK_AMD_negative_viewport_height need to be set for this to work, from version
  // >= 1.0.39
  VkViewport viewport = {};
  viewport.x = x;
  viewport.y = vkContext.screen.height - y;
  viewport.width = width;
  viewport.height = -height;
  viewport.minDepth = minDepth;
  viewport.maxDepth = maxDepth;
  vkCmdSetViewport(vkContext.commandBuffer, 0, 1, &viewport);
}

static void setFullscreenScissor() {
  VkRect2D scissor = {};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent.width = vkContext.screen.width;
  scissor.extent.height = vkContext.screen.height;
  vkCmdSetScissor(vkContext.commandBuffer, 0, 1, &scissor);
}

void setFullscreenViewport() {
  setViewPort(0, 0, float(vkContext.screen.width), float(vkContext.screen.height), 0.0f, 1.0f);
  setFullscreenScissor();
}

void waitForDeviceIdle() {
  mg::mgSystem.linearHeapAllocator.submitStagingMemoryToDeviceLocalMemory();
  checkResult(vkDeviceWaitIdle(vkContext.device));
}

void beginRendering() {
  vkContext.commandBuffer = vkContext.commandBuffers.buffers[vkContext.commandBuffers.currentIndex];

  if (vkContext.commandBuffers.submitted[vkContext.commandBuffers.currentIndex]) {
    checkResult(vkWaitForFences(vkContext.device, 1,
                                &vkContext.commandBuffers.fences[vkContext.commandBuffers.currentIndex], VK_TRUE,
                                UINT64_MAX));
  }
  checkResult(
      vkResetFences(vkContext.device, 1, &vkContext.commandBuffers.fences[vkContext.commandBuffers.currentIndex]));

  VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {};
  vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  checkResult(vkBeginCommandBuffer(vkContext.commandBuffer, &vkCommandBufferBeginInfo));

  setFullscreenViewport();
  acquireNextSwapChainImage();
}

void acquireNextSwapChainImage() {
  auto result = vkAcquireNextImageKHR(vkContext.device, vkContext.swapChain->swapChain, UINT64_MAX,
                            vkContext.commandBuffers.imageAquiredSemaphore[vkContext.commandBuffers.currentIndex],
                            VK_NULL_HANDLE, &vkContext.swapChain->currentSwapChainIndex);
  if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    mg::vkContext.swapChain->resize();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }
}

void endRendering() {
  mg::mgSystem.linearHeapAllocator.swapLinearHeapBuffers();

  const auto commandBufferIndex = vkContext.commandBuffers.currentIndex;
  checkResult(vkEndCommandBuffer(vkContext.commandBuffer));

  VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &vkContext.commandBuffer;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &vkContext.commandBuffers.imageAquiredSemaphore[commandBufferIndex];
  submitInfo.pWaitDstStageMask = &waitDstStageMask;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &vkContext.commandBuffers.renderCompleteSemaphore[commandBufferIndex];

  checkResult(vkQueueSubmit(vkContext.queue, 1, &submitInfo, vkContext.commandBuffers.fences[commandBufferIndex]));

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &vkContext.swapChain->swapChain;
  presentInfo.pImageIndices = &vkContext.swapChain->currentSwapChainIndex;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &vkContext.commandBuffers.renderCompleteSemaphore[commandBufferIndex];
  presentInfo.pResults = nullptr;

  auto result = vkQueuePresentKHR(vkContext.queue, &presentInfo);
    if(!(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || result == VK_SUCCESS)) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  vkContext.commandBuffers.submitted[commandBufferIndex] = true;
  vkContext.commandBuffers.currentIndex =
      (vkContext.commandBuffers.currentIndex + 1) % vkContext.commandBuffers.nrOfBuffers;
}

mg::TextureId uploadPngImage(const std::string &name) {
  std::vector<unsigned char> imageData;
  uint32_t width, height;
  const auto error = lodepng::decode(imageData, width, height, getTexturePath() + name);
  mgAssertDesc(error == 0, "decoder error " << error << ": " << lodepng_error_text(error));

  mg::CreateTextureInfo createTextureInfo = {};
  createTextureInfo.data = imageData.data();
  createTextureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  createTextureInfo.id = name;
  createTextureInfo.size = {width, height, 1};
  createTextureInfo.sizeInBytes = mg::sizeofContainerInBytes(imageData);
  createTextureInfo.type = mg::TEXTURE_TYPE::TEXTURE_2D;
  return mg::mgSystem.textureContainer.createTexture(createTextureInfo);
}
} // namespace mg