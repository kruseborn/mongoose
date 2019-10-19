#pragma once
#include "vkContext.h"

namespace mg {

constexpr uint32_t MAX_SWAP_CHAIN_IMAGES = 8;

struct SwapChain {
  VkFormat format = {};
  VkSwapchainKHR swapChain = VK_NULL_HANDLE;

  VkImage images[MAX_SWAP_CHAIN_IMAGES] = {};
  VkImageView imageViews[MAX_SWAP_CHAIN_IMAGES] = {};

  uint32_t numOfImages;
  uint32_t currentSwapChainIndex;
  void init();
  void resize();
  void destroy();

  void (*resizeCallack)(void) = nullptr;

private:
  void createImages();
  void createImageViews();
};

} // namespace mg