#pragma once
#define VK_USE_PLATFORM_WIN32_KHR 1

#include <memory>
#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace mg {

struct SwapChain;

struct VulkanContext {
  struct {
    uint32_t width, height;
    uint32_t dpix, dpiy;
  } screen;

  VkDevice device;

  VkQueue queue;
  VkDescriptorPool descriptorPool;

  VkPipelineLayout pipelineLayout;
  VkPipelineLayout pipelineLayoutMultiTexture;

  VkCommandPool commandPool;
  uint32_t graphicsQueueFamilyIndex;
  uint32_t presentQueueFamilyIndex;
  struct {
    VkDescriptorSetLayout ubo;
    VkDescriptorSetLayout texture;
  } descriptorSetLayout;

  VkPipelineCache pipelineCache;

  VkPhysicalDeviceProperties physicalDeviceProperties;
  VkPhysicalDeviceFeatures physicalDeviceFeatures;
  VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;

  VkDebugUtilsMessengerEXT callback;

  struct {
    VkSampler pointBorderSampler, linearBorderSampler, linearEdgeSampler, linearRepeat;
  } sampler;
  struct {
    VkFormat depth;
  } formats;
  VkCommandBuffer commandBuffer;
  struct CommandBuffers {
    static constexpr uint32_t nrOfBuffers = 2;
    uint32_t currentIndex = 0;
    VkCommandBuffer buffers[nrOfBuffers] = {};
    VkFence fences[nrOfBuffers] = {};
    bool submitted[nrOfBuffers] = {};
    VkSemaphore imageAquiredSemaphore[nrOfBuffers] = {};
    VkSemaphore renderCompleteSemaphore[nrOfBuffers] = {};
  } commandBuffers;

  std::unique_ptr<SwapChain> swapChain;

  VkInstance instance = VK_NULL_HANDLE;
  VkSurfaceKHR windowSurface = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

  uint64_t frameTimeInMs = 0;
  uint64_t updateAndRenderTime = 0;
};

extern VulkanContext vkContext;

bool initVulkan(GLFWwindow *window);
void destroyVulkan();

} // namespace mg