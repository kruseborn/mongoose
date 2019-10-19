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

constexpr uint32_t MAX_NR_OF_2D_TEXTURES = 128;
constexpr uint32_t MAX_NR_OF_3D_TEXTURES = 5;

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
    VkPipelineLayout pipelineLayoutStorage;
    VkPipelineLayout pipelineLayoutRayTracing;

  } pipelineLayouts;

  VkCommandPool commandPool;
  uint32_t queueFamilyIndex;
  struct {
    VkDescriptorSetLayout dynamic;
    VkDescriptorSetLayout textures;
    VkDescriptorSetLayout textures3D;
    VkDescriptorSetLayout storage;
    VkDescriptorSetLayout storageImage;
    VkDescriptorSetLayout accelerationStructure;

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

namespace nv {
extern PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructureNV;
extern PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructureNV;
extern PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemoryNV;
extern PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandleNV;
extern PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV;
extern PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructureNV;
extern PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV;
extern PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV;
extern PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV;

} // namespace nv

} // namespace mg