#include "vkWindow.h"

#include "mg/logger.h"
#include "mg/mgSystem.h"
#include "singleRenderpass.h"
#include "vkContext.h"
#include "vkUtils.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

using namespace std;

namespace mg {
#if (defined _DEBUG) || (defined DEBUG)
#define __DEBUG true
#else
#define __DEBUG false
#endif

#define ENABLE_DEBUGGING (true && std::getenv("VULKAN_SDK") != nullptr)

static char *DEBUG_LAYER = (char *)"VK_LAYER_LUNARG_standard_validation";

void createCommandBuffers() {
  for (int i = 0; i < mg::vkContext.commandBuffers.nrOfBuffers; ++i) {
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = mg::vkContext.commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    checkResult(vkAllocateCommandBuffers(mg::vkContext.device, &commandBufferAllocateInfo,
                                         &mg::vkContext.commandBuffers.buffers[i]));
  }
}
void createCommandBufferFences() {
  VkFenceCreateInfo fenceCreateInfo = {};
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  for (int i = 0; i < mg::vkContext.commandBuffers.nrOfBuffers; ++i) {
    checkResult(vkCreateFence(mg::vkContext.device, &fenceCreateInfo, NULL, &mg::vkContext.commandBuffers.fences[i]));
  }
}
void createCommandBufferSemaphores() {
  for (size_t i = 0; i < mg::vkContext.commandBuffers.nrOfBuffers; i++) {
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    checkResult(vkCreateSemaphore(mg::vkContext.device, &semaphoreCreateInfo, nullptr,
                                  &mg::vkContext.commandBuffers.imageAquiredSemaphore[i]));
    checkResult(vkCreateSemaphore(mg::vkContext.device, &semaphoreCreateInfo, nullptr,
                                  &mg::vkContext.commandBuffers.renderCompleteSemaphore[i]));
  }
}
void initSwapChain() {
  mg::vkContext.swapChain = std::make_unique<SwapChain>();
  mg::vkContext.swapChain->init();
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                             const VkAllocationCallbacks *pAllocator,
                                             VkDebugUtilsMessengerEXT *pCallback) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pCallback);
  } else {
    cout << "here we are" << endl;
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                    void *pUserData) {
  std::string prefix;
  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
    prefix += "Verbose: ";
  } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    prefix += "Info: ";
  } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    prefix += "Debug: ";
  } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    prefix += "Error: ";
  }

  LOG(prefix << pCallbackData->pMessage);
  std::fflush(stdout);
  std::fflush(stderr);
  return VK_FALSE;
}

static void createDebugCallback() {
  if (ENABLE_DEBUGGING) {
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;

    if (CreateDebugUtilsMessengerEXT(mg::vkContext.instance, &createInfo, nullptr, &mg::vkContext.callback) !=
        VK_SUCCESS) {
      mgAssertDesc(false, "failed to set up debug callback!");
    }
  }
}

static bool createInstance() {
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "VulkanClear";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "mongoose engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  std::vector<const char *> extensions;
  extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
  extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#if defined(VK_USE_PLATFORM_WIN32_KHR)
  extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_IOS_MVK)
  extensions.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
  extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
  extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
  if (ENABLE_DEBUGGING)
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  // Check for extensions
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  if (extensionCount == 0) {
    LOG("could not create vulkan instance: vkEnumerateInstanceExtensionProperties returns 0");
    return false;
  }
  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

  LOG("supported extensions : ");
  std::string extensionsStr = "";
  for (const auto &extension : availableExtensions) {
    extensionsStr += std::string(extension.extensionName) + " ";
  }
  LOG(extensionsStr);

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = (uint32_t)extensions.size();
  createInfo.ppEnabledExtensionNames = extensions.data();

  if (ENABLE_DEBUGGING) {
    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = &DEBUG_LAYER;
  }
  auto res = vkCreateInstance(&createInfo, nullptr, &mg::vkContext.instance);
  if (res == VK_SUCCESS)
    return true;
  LOG("could not create vulkan instance: vkCreateInstance");
  return false;
}

static void createWindowSurface(SDL_Window *window) {
  if(SDL_Vulkan_CreateSurface(window, mg::vkContext.instance, &mg::vkContext.windowSurface) == SDL_FALSE) {
    SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "Couldn't initialize SDL: %s",
            SDL_GetError());
  }
}

static void findPhysicalDevice() {
  uint32_t deviceCount = 0;
  if (vkEnumeratePhysicalDevices(mg::vkContext.instance, &deviceCount, nullptr) != VK_SUCCESS) {
    mgAssertDesc(false, "failed to get number of physical devices");
  }
  mgAssertDesc(deviceCount > 0, "no physical devices that support vulkan!");

  std::vector<VkPhysicalDevice> vkPhysicalDevices(deviceCount);
  checkResult(vkEnumeratePhysicalDevices(mg::vkContext.instance, &deviceCount, vkPhysicalDevices.data()));
  struct PhysicalDeviceInfo {
    VkPhysicalDevice vkPhysicalDevice;
    VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
    VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures;
  };
  std::vector<PhysicalDeviceInfo> physicalDeviceInfos(vkPhysicalDevices.size());

  for (uint32_t i = 0; i < uint32_t(vkPhysicalDevices.size()); i++) {
    vkGetPhysicalDeviceProperties(vkPhysicalDevices[i], &physicalDeviceInfos[i].vkPhysicalDeviceProperties);
    vkGetPhysicalDeviceFeatures(vkPhysicalDevices[i], &physicalDeviceInfos[i].vkPhysicalDeviceFeatures);
    physicalDeviceInfos[i].vkPhysicalDevice = vkPhysicalDevices[i];
  }
  const auto physicalDevice = physicalDeviceInfos.front();

  std::ios_base::fmtflags f(std::cout.flags());

  LOG("DeviceName: " << std::hex << physicalDevice.vkPhysicalDeviceProperties.deviceName);
  LOG("VendorID: " << std::hex << physicalDevice.vkPhysicalDeviceProperties.vendorID);
  LOG("DeviceID: " << std::hex << physicalDevice.vkPhysicalDeviceProperties.deviceID);
  LOG("DeviceType: " << physicalDevice.vkPhysicalDeviceProperties.deviceType);

  std::cout.flags(f);

  mg::vkContext.physicalDevice = physicalDevice.vkPhysicalDevice;
  mg::vkContext.physicalDeviceProperties = physicalDevice.vkPhysicalDeviceProperties;
  mg::vkContext.physicalDeviceFeatures = physicalDevice.vkPhysicalDeviceFeatures;

  constexpr int32_t nvidiaVendorId = 0x10DE, intelVendorId = 0x8086, amdVendorId = 0x1002;

  switch (mg::vkContext.physicalDeviceProperties.vendorID) {
  case intelVendorId:
    LOG("Vendor: Intel");
    break;
  case nvidiaVendorId:
    LOG("Vendor: NVIDIA");
    break;
  case amdVendorId:
    LOG("Vendor: AMD");
    break;
  default:
    LOG("Vendor: Unknown: " << mg::vkContext.physicalDeviceProperties.vendorID);
  }

  uint32_t supportedVersion[] = {VK_VERSION_MAJOR(mg::vkContext.physicalDeviceProperties.apiVersion),
                                 VK_VERSION_MINOR(mg::vkContext.physicalDeviceProperties.apiVersion),
                                 VK_VERSION_PATCH(mg::vkContext.physicalDeviceProperties.apiVersion)};
  LOG("Physical device supports version " << supportedVersion[0] << "." << supportedVersion[1] << "."
                                          << supportedVersion[2]);

  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  checkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mg::vkContext.physicalDevice, mg::vkContext.windowSurface,
                                                        &surfaceCapabilities));
  if (mg::vkContext.screen.width != surfaceCapabilities.currentExtent.width ||
      mg::vkContext.screen.height != surfaceCapabilities.currentExtent.height) {
    mg::vkContext.screen.width = surfaceCapabilities.currentExtent.width;
    mg::vkContext.screen.height = surfaceCapabilities.currentExtent.height;
  }
}

static void createLogicalDevice() {
  // use same queue for graphic, present and compute
  float queuePriority = 1.0f;

  VkDeviceQueueCreateInfo queueCreateInfo = {};

  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = mg::vkContext.queueFamilyIndex;
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeatures = {};
  physicalDeviceDescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
  physicalDeviceDescriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
  physicalDeviceDescriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
  physicalDeviceDescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;

  // Create logical device from physical device
  // Note: there are separate instance and device extensions!
  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
  deviceCreateInfo.queueCreateInfoCount = 1;
  deviceCreateInfo.pNext = &physicalDeviceDescriptorIndexingFeatures;

  VkPhysicalDeviceDescriptorIndexingFeaturesEXT descIndexFeatures = {};
  descIndexFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
  VkPhysicalDeviceFeatures2 supportedFeatures = {};
  supportedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  supportedFeatures.pNext = &descIndexFeatures;

  // Check for extensions
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(mg::vkContext.physicalDevice, nullptr, &extensionCount, nullptr);
  mgAssert(extensionCount > 0);
  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(mg::vkContext.physicalDevice, nullptr, &extensionCount,
                                       availableExtensions.data());

  LOG("Device supported extensions : ");
  std::string extensionNameStr = "";
  for (const auto &extension : availableExtensions) {
    extensionNameStr += std::string(extension.extensionName) + " ";
  }
  LOG(extensionNameStr);

  // Necessary for shader (for some reason)
  VkPhysicalDeviceFeatures enabledFeatures = {};
  enabledFeatures.shaderClipDistance = VK_TRUE;
  enabledFeatures.shaderCullDistance = VK_TRUE;

  const char *deviceExtensions[6] = {VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
                                     VK_KHR_MAINTENANCE3_EXTENSION_NAME,
                                     VK_KHR_MAINTENANCE1_EXTENSION_NAME,
                                     VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                     VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
                                     VK_NV_RAY_TRACING_EXTENSION_NAME};

  deviceCreateInfo.enabledExtensionCount = mg::countof(deviceExtensions);
  deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

  deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
  checkResult(vkCreateDevice(mg::vkContext.physicalDevice, &deviceCreateInfo, nullptr, &mg::vkContext.device));

  vkGetDeviceQueue(mg::vkContext.device, mg::vkContext.queueFamilyIndex, 0, &mg::vkContext.queue);
  vkGetPhysicalDeviceMemoryProperties(mg::vkContext.physicalDevice, &mg::vkContext.physicalDeviceMemoryProperties);
}

static void findQueueFamilies() {
  // Check queue families
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(mg::vkContext.physicalDevice, &queueFamilyCount, nullptr);
  mgAssert(queueFamilyCount != 0);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(mg::vkContext.physicalDevice, &queueFamilyCount, queueFamilies.data());

  uint32_t i = 0;
  for (; i < queueFamilyCount; i++) {
    VkBool32 presentSupport;

    vkGetPhysicalDeviceSurfaceSupportKHR(mg::vkContext.physicalDevice, i, mg::vkContext.windowSurface, &presentSupport);

    if (presentSupport && queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
        queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
      mg::vkContext.queueFamilyIndex = i;
      break;
    }
  }
  mgAssert(i != queueFamilyCount);
}

static VkFormat getSupportedDepthFormat() {
  VkFormat formatDepth = VK_FORMAT_UNDEFINED;
  // Since all depth formats may be optional, we need to find a suitable depth format to use
  // Start with the highest precision packed format
  VkFormat depthFormats[5] = {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT,
                              VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM};
  for (auto &format : depthFormats) {
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(mg::vkContext.physicalDevice, format, &formatProps);
    // Format must support depth stencil attachment for optimal tiling
    if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      formatDepth = format;
      return formatDepth;
    }
  }
  mgAssert(false && "could not find any depth format that was supported");
  return formatDepth;
}

static void setupFormats() { mg::vkContext.formats.depth = getSupportedDepthFormat(); }

bool createVulkanContext(SDL_Window *window) {
  if (!createInstance())
    return false;
  createDebugCallback();
  createWindowSurface(window);
  findPhysicalDevice();
  findQueueFamilies();
  createLogicalDevice();

  setupFormats();
  initSwapChain();
  return true;
}

void destroyVulkanWindow() {
  vkDestroySurfaceKHR(mg::vkContext.instance, mg::vkContext.windowSurface, nullptr);

  if (ENABLE_DEBUGGING) {
    PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugReportCallback =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mg::vkContext.instance,
                                                                   "vkDestroyDebugUtilsMessengerEXT");
    DestroyDebugReportCallback(mg::vkContext.instance, mg::vkContext.callback, nullptr);
  }
}

void destoyInstance() { vkDestroyInstance(mg::vkContext.instance, nullptr); }

void resizeWindow() {
  waitForDeviceIdle();

  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  checkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mg::vkContext.physicalDevice, mg::vkContext.windowSurface,
                                                        &surfaceCapabilities));

  LOG("surfaceCapabilities.currentExtent.width" << surfaceCapabilities.currentExtent.width << " " << surfaceCapabilities.currentExtent.height);
  mgAssert(mg::vkContext.screen.width == surfaceCapabilities.currentExtent.width &&
          mg::vkContext.screen.height == surfaceCapabilities.currentExtent.height);

  mg::vkContext.swapChain->resize();
  waitForDeviceIdle();

}

} // namespace mg