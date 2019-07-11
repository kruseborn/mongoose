#include "swapChain.h"

#include <algorithm>
#include <vector>

#include "mg/logger.h"
#include "vkUtils.h"

namespace mg {

static void checkSwapChainSupport() {
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(mg::vkContext.physicalDevice, nullptr, &extensionCount, nullptr);
  mgAssert(extensionCount > 0);

  std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(mg::vkContext.physicalDevice, nullptr, &extensionCount, deviceExtensions.data());

  for (const auto &extension : deviceExtensions) {
    if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
      return;
    }
  }
  mgAssertDesc(false, "physical device doesn't support swap chains");
  exit(1);
}

static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
  if (int32_t(surfaceCapabilities.currentExtent.width) == -1) {
    VkExtent2D extend = {};
    extend.width = std::min(std::max(mg::vkContext.screen.width, surfaceCapabilities.minImageExtent.width),
                            surfaceCapabilities.maxImageExtent.width);
    extend.height = std::min(std::max(mg::vkContext.screen.height, surfaceCapabilities.minImageExtent.height),
                             surfaceCapabilities.maxImageExtent.height);
    return extend;
  } else {
    return surfaceCapabilities.currentExtent;
  }
}

static VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
  // We can either choose any format
  if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
    return {VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR};
  }
  // Or go with the standard format - if available
  for (const auto &availableSurfaceFormat : availableFormats) {
    if (availableSurfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM) {
      return availableSurfaceFormat;
    }
  }
  // Or fall back to the first available one
  return availableFormats[0];
}

void SwapChain::createImageViews() {
  for (size_t i = 0; i < numOfImages; i++) {
    VkImageViewCreateInfo vkImageViewCreateInfo = {};
    vkImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vkImageViewCreateInfo.subresourceRange = {};
    vkImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    vkImageViewCreateInfo.subresourceRange.levelCount = 1;
    vkImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    vkImageViewCreateInfo.subresourceRange.layerCount = 1;

    vkImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vkImageViewCreateInfo.format = format;
    vkImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    vkImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    vkImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    vkImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    vkImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    vkImageViewCreateInfo.image = images[i];
    checkResult(vkCreateImageView(mg::vkContext.device, &vkImageViewCreateInfo, nullptr, &imageViews[i]));
  }
}

void SwapChain::createImages() {
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  checkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mg::vkContext.physicalDevice, mg::vkContext.windowSurface, &surfaceCapabilities));
  if (surfaceCapabilities.currentExtent.width != mg::vkContext.screen.width ||
      surfaceCapabilities.currentExtent.height != mg::vkContext.screen.height) {
    mgAssertDesc(false, "Surface doesn't match video width or height");
    exit(1);
  }
  uint32_t formatCount;
  checkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(mg::vkContext.physicalDevice, mg::vkContext.windowSurface, &formatCount, nullptr));
  mgAssert(formatCount != 0);

  std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
  checkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(mg::vkContext.physicalDevice, mg::vkContext.windowSurface, &formatCount,
                                                   surfaceFormats.data()));

  // Find supported present modes
  uint32_t presentModeCount;
  checkResult(
      vkGetPhysicalDeviceSurfacePresentModesKHR(mg::vkContext.physicalDevice, mg::vkContext.windowSurface, &presentModeCount, nullptr));
  mgAssert(presentModeCount != 0);

  std::vector<VkPresentModeKHR> presentModes(presentModeCount);
  checkResult(vkGetPhysicalDeviceSurfacePresentModesKHR(mg::vkContext.physicalDevice, mg::vkContext.windowSurface, &presentModeCount,
                                                        presentModes.data()));

  VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(surfaceFormats);
  // Select swap chain size
  const auto swapChainExtent = chooseSwapExtent(surfaceCapabilities);

  // Determine transformation to use (preferring no transform)
  VkSurfaceTransformFlagBitsKHR surfaceTransform;
  if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    surfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  } else {
    surfaceTransform = surfaceCapabilities.currentTransform;
  }
  // VK_PRESENT_MODE_FIFO_KHR is always supported
  VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
  for (const auto &mode : presentModes) {
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      presentMode = mode;
    }
  }
  switch (presentMode) {
  case VK_PRESENT_MODE_FIFO_KHR:
    LOG("Using FIFO present mode");
    break;
  case VK_PRESENT_MODE_MAILBOX_KHR:
    LOG("Using MAILBOX present mode");
    break;
  case VK_PRESENT_MODE_IMMEDIATE_KHR:
    LOG("Using IMMEDIATE present mode");
    break;
  case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
    LOG("Using FIFO_RELAXED present mode");
    break;
  default:
    mgAssert(false);
    break;
  }

  vkDestroySwapchainKHR(mg::vkContext.device, swapChain, nullptr);

  VkSwapchainCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = mg::vkContext.windowSurface;
  createInfo.minImageCount = 2;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = swapChainExtent;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  createInfo.preTransform = surfaceTransform;
  createInfo.imageArrayLayers = 1;
  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffer is exclusive to a single queue family at a time.
  createInfo.queueFamilyIndexCount = 0;
  createInfo.pQueueFamilyIndices = nullptr;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = nullptr;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  // Not all devices support ALPHA_OPAQUE
  if (!(surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR))
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

  checkResult(vkCreateSwapchainKHR(mg::vkContext.device, &createInfo, nullptr, &swapChain));
  format = surfaceFormat.format;

  // Store the images used by the swap chain
  // Note: these are the images that swap chain image indices refer to
  // Note: actual number of images may differ   om requested number, since it's a lower bound
  checkResult(vkGetSwapchainImagesKHR(mg::vkContext.device, swapChain, &numOfImages, nullptr));
  mgAssert(numOfImages > 0 && numOfImages < MAX_SWAP_CHAIN_IMAGES);
  checkResult(vkGetSwapchainImagesKHR(mg::vkContext.device, swapChain, &numOfImages, images));
  LOG("NumOfSwapChainImages: " << numOfImages);
}

void SwapChain::init() {
  checkSwapChainSupport();
  createImages();
  createImageViews();
}

void SwapChain::resize() {
  for (size_t i = 0; i < numOfImages; i++)
    vkDestroyImageView(mg::vkContext.device, imageViews[i], nullptr);

  createImages();
  createImageViews();
}

void SwapChain::destroy() {
  for (size_t i = 0; i < numOfImages; i++) {
    vkDestroyImageView(mg::vkContext.device, imageViews[i], nullptr);
  }
  vkDestroySwapchainKHR(mg::vkContext.device, swapChain, nullptr);
}

} // namespace mg