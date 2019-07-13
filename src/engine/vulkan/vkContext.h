#pragma once
#if defined(WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_XCB_KHR
#endif

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

  struct {
    VkPipelineLayout pipelineLayout;
    VkPipelineLayout pipelineStorageLayout;
    VkPipelineLayout pipelineLayoutMultiTexture;
    VkPipelineLayout pipelineComputeLayout;
  } pipelineLayouts;

  VkCommandPool commandPool;
  uint32_t queueFamilyIndex;
  struct {
    VkDescriptorSetLayout ubo;
    VkDescriptorSetLayout storageDynamic;
    VkDescriptorSetLayout storage;
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