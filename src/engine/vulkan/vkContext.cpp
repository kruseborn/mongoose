#include "vkContext.h"

#include <cfloat>
#include <memory>

#include "linearHeapAllocator.h"
#include "mg/mgAssert.h"
#include "mg/textureContainer.h"
#include "singleRenderpass.h"
#include "vkUtils.h"
#include "vkWindow.h"

namespace mg {

VulkanContext vkContext = {};

static void destroySampler();
void destroyVulkan() {
  mg::vkContext.swapChain->destroy();
  destroySampler();

  for (uint32_t i = 0; i < mg::vkContext.commandBuffers.nrOfBuffers; i++) {
    vkDestroySemaphore(mg::vkContext.device, mg::vkContext.commandBuffers.imageAquiredSemaphore[i], nullptr);
    vkDestroySemaphore(mg::vkContext.device, mg::vkContext.commandBuffers.renderCompleteSemaphore[i], nullptr);
    vkDestroyFence(mg::vkContext.device, mg::vkContext.commandBuffers.fences[i], nullptr);
  }

  vkDestroyPipelineCache(mg::vkContext.device, mg::vkContext.pipelineCache, nullptr);
  vkDestroyPipelineLayout(mg::vkContext.device, mg::vkContext.pipelineLayouts.pipelineLayout, nullptr);
  vkDestroyPipelineLayout(mg::vkContext.device, mg::vkContext.pipelineLayouts.pipelineLayoutStorage, nullptr);
  vkDestroyPipelineLayout(mg::vkContext.device, mg::vkContext.pipelineLayouts.pipelineLayoutRayTracing, nullptr);

  vkDestroyDescriptorSetLayout(mg::vkContext.device, mg::vkContext.descriptorSetLayout.dynamic, nullptr);
  vkDestroyDescriptorSetLayout(mg::vkContext.device, mg::vkContext.descriptorSetLayout.textures, nullptr);
  vkDestroyDescriptorSetLayout(mg::vkContext.device, mg::vkContext.descriptorSetLayout.textures3D, nullptr);
  vkDestroyDescriptorSetLayout(mg::vkContext.device, mg::vkContext.descriptorSetLayout.storage, nullptr);
  vkDestroyDescriptorSetLayout(mg::vkContext.device, mg::vkContext.descriptorSetLayout.storageImage, nullptr);
  vkDestroyDescriptorSetLayout(mg::vkContext.device, mg::vkContext.descriptorSetLayout.accelerationStructure, nullptr);

  vkDestroyCommandPool(mg::vkContext.device, mg::vkContext.commandPool, nullptr);

  vkDestroyDescriptorPool(mg::vkContext.device, mg::vkContext.descriptorPool, nullptr);

  destroyVulkanWindow();
  vkDestroyDevice(mg::vkContext.device, nullptr);
  destoyInstance();
}

static void createDescriptorLayout() {
  // ubo && storage, dynamic
  {
    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2] = {};
    descriptorSetLayoutBindings[0].binding = 0;
    descriptorSetLayoutBindings[0].descriptorCount = 1;
    descriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_ALL;

    descriptorSetLayoutBindings[1].binding = 1;
    descriptorSetLayoutBindings[1].descriptorCount = 1;
    descriptorSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    descriptorSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_ALL;

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags = {};
    setLayoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = mg::countof(descriptorSetLayoutBindings);
    descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;

    checkResult(vkCreateDescriptorSetLayout(mg::vkContext.device, &descriptorSetLayoutCreateInfo, nullptr,
                                            &mg::vkContext.descriptorSetLayout.dynamic));
  }
  // textures 2D
  {
    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2] = {};
    descriptorSetLayoutBindings[0].binding = 0;
    descriptorSetLayoutBindings[0].descriptorCount = 2;
    descriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_ALL;

    descriptorSetLayoutBindings[1].binding = 1;
    descriptorSetLayoutBindings[1].descriptorCount = MAX_NR_OF_2D_TEXTURES;
    descriptorSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptorSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_ALL;

    VkDescriptorBindingFlagsEXT descriptorBindingFlags[] = {0, VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT};
    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags = {};
    setLayoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    setLayoutBindingFlags.bindingCount = mg::countof(descriptorBindingFlags);
    setLayoutBindingFlags.pBindingFlags = descriptorBindingFlags;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = mg::countof(descriptorSetLayoutBindings);
    descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;
    descriptorSetLayoutCreateInfo.pNext = &setLayoutBindingFlags;

    checkResult(vkCreateDescriptorSetLayout(mg::vkContext.device, &descriptorSetLayoutCreateInfo, nullptr,
                                            &mg::vkContext.descriptorSetLayout.textures));
  }
  // textures 3D
  {
    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[1] = {};
    descriptorSetLayoutBindings[0].binding = 0;
    descriptorSetLayoutBindings[0].descriptorCount = 1;
    descriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_ALL;

    VkDescriptorBindingFlagsEXT descriptorBindingFlags[] = {VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT};
    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags = {};
    setLayoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    setLayoutBindingFlags.bindingCount = mg::countof(descriptorBindingFlags);
    setLayoutBindingFlags.pBindingFlags = descriptorBindingFlags;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = mg::countof(descriptorSetLayoutBindings);
    descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;

    checkResult(vkCreateDescriptorSetLayout(mg::vkContext.device, &descriptorSetLayoutCreateInfo, nullptr,
                                            &mg::vkContext.descriptorSetLayout.textures3D));
  }
  // storage
  {
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags = {};
    setLayoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = 1;
    descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding;

    checkResult(vkCreateDescriptorSetLayout(mg::vkContext.device, &descriptorSetLayoutCreateInfo, nullptr,
                                            &mg::vkContext.descriptorSetLayout.storage));
  }
    // storage image
  {
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags = {};
    setLayoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = 1;
    descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding;

    checkResult(vkCreateDescriptorSetLayout(mg::vkContext.device, &descriptorSetLayoutCreateInfo, nullptr,
                                            &mg::vkContext.descriptorSetLayout.storageImage));
  }
    // acceleration structure
  {
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
    descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags = {};
    setLayoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = 1;
    descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding;

    checkResult(vkCreateDescriptorSetLayout(mg::vkContext.device, &descriptorSetLayoutCreateInfo, nullptr,
                                            &mg::vkContext.descriptorSetLayout.accelerationStructure));
  }
}

static void createPipelineLayout() {
  {
    VkDescriptorSetLayout descriptorSetLayoutsStorages[3] = {
        mg::vkContext.descriptorSetLayout.dynamic,
        mg::vkContext.descriptorSetLayout.textures,
        mg::vkContext.descriptorSetLayout.textures3D,
    };

    VkPipelineLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = mg::countof(descriptorSetLayoutsStorages);
    layoutCreateInfo.pSetLayouts = descriptorSetLayoutsStorages;

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_ALL;
    pushConstantRange.size = 256;

    layoutCreateInfo.pPushConstantRanges = &pushConstantRange;
    layoutCreateInfo.pushConstantRangeCount = 1;
    checkResult(vkCreatePipelineLayout(mg::vkContext.device, &layoutCreateInfo, nullptr,
                                       &mg::vkContext.pipelineLayouts.pipelineLayout));
  }
  {
    VkDescriptorSetLayout descriptorSetLayoutsStorages[] = {
        mg::vkContext.descriptorSetLayout.dynamic,
        mg::vkContext.descriptorSetLayout.storage,
        mg::vkContext.descriptorSetLayout.storage,
        mg::vkContext.descriptorSetLayout.storage,
        mg::vkContext.descriptorSetLayout.storage
    };

    VkPipelineLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = mg::countof(descriptorSetLayoutsStorages);
    layoutCreateInfo.pSetLayouts = descriptorSetLayoutsStorages;

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_ALL;
    pushConstantRange.size = 256;

    layoutCreateInfo.pPushConstantRanges = &pushConstantRange;
    layoutCreateInfo.pushConstantRangeCount = 1;
    checkResult(vkCreatePipelineLayout(mg::vkContext.device, &layoutCreateInfo, nullptr,
                                       &mg::vkContext.pipelineLayouts.pipelineLayoutStorage));
  }
  // raytracing
  {
    VkDescriptorSetLayout descriptorSetLayoutsStorages[5] = {
        mg::vkContext.descriptorSetLayout.dynamic,
        mg::vkContext.descriptorSetLayout.storageImage,
        mg::vkContext.descriptorSetLayout.accelerationStructure,
        mg::vkContext.descriptorSetLayout.textures,
        mg::vkContext.descriptorSetLayout.storageImage,
    };

    VkPipelineLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = mg::countof(descriptorSetLayoutsStorages);
    layoutCreateInfo.pSetLayouts = descriptorSetLayoutsStorages;

    checkResult(vkCreatePipelineLayout(mg::vkContext.device, &layoutCreateInfo, nullptr,
                                       &mg::vkContext.pipelineLayouts.pipelineLayoutRayTracing));
  }
}

static void createPipelineCache() {
  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
  pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  VkResult err =
      vkCreatePipelineCache(mg::vkContext.device, &pipelineCacheCreateInfo, nullptr, &mg::vkContext.pipelineCache);
  mgAssert(!err);
}

static void createCommandPool() {
  VkCommandPoolCreateInfo poolCreateInfo = {};
  poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolCreateInfo.queueFamilyIndex = mg::vkContext.queueFamilyIndex;
  poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  checkResult(vkCreateCommandPool(mg::vkContext.device, &poolCreateInfo, nullptr, &mg::vkContext.commandPool));
}

static void createDescriptorPool() {
  VkDescriptorPoolSize descriptorPoolSizes[3] = {};

  descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  descriptorPoolSizes[0].descriptorCount = 1;

  descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
  descriptorPoolSizes[1].descriptorCount = 2;

  descriptorPoolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  descriptorPoolSizes[2].descriptorCount = MAX_NR_OF_2D_TEXTURES;

  VkDescriptorPoolCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  createInfo.poolSizeCount = mg::countof(descriptorPoolSizes);
  createInfo.pPoolSizes = descriptorPoolSizes;
  createInfo.maxSets = MAX_NR_OF_2D_TEXTURES + 32;
  createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

  checkResult(vkCreateDescriptorPool(mg::vkContext.device, &createInfo, nullptr, &mg::vkContext.descriptorPool));
}

static void initSampler() {
  VkSamplerCreateInfo sampler_create_info = {};
  sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_create_info.magFilter = VK_FILTER_NEAREST;
  sampler_create_info.minFilter = VK_FILTER_NEAREST;
  sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  sampler_create_info.mipLodBias = 0.0f;
  sampler_create_info.maxAnisotropy = 1.0f;
  sampler_create_info.minLod = 0;
  sampler_create_info.maxLod = 0;
  sampler_create_info.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
  sampler_create_info.anisotropyEnable = VK_FALSE;
  sampler_create_info.compareOp = VK_COMPARE_OP_NEVER;

  checkResult(
      vkCreateSampler(mg::vkContext.device, &sampler_create_info, NULL, &mg::vkContext.sampler.pointBorderSampler));

  sampler_create_info.magFilter = VK_FILTER_LINEAR;
  sampler_create_info.minFilter = VK_FILTER_LINEAR;
  sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  checkResult(
      vkCreateSampler(mg::vkContext.device, &sampler_create_info, NULL, &mg::vkContext.sampler.linearBorderSampler));

  sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  checkResult(
      vkCreateSampler(mg::vkContext.device, &sampler_create_info, NULL, &mg::vkContext.sampler.linearEdgeSampler));

  sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  checkResult(vkCreateSampler(mg::vkContext.device, &sampler_create_info, NULL, &mg::vkContext.sampler.linearRepeat));
}

static void destroySampler() {
  vkDestroySampler(mg::vkContext.device, mg::vkContext.sampler.pointBorderSampler, nullptr);
  vkDestroySampler(mg::vkContext.device, mg::vkContext.sampler.linearBorderSampler, nullptr);
  vkDestroySampler(mg::vkContext.device, mg::vkContext.sampler.linearEdgeSampler, nullptr);
  vkDestroySampler(mg::vkContext.device, mg::vkContext.sampler.linearRepeat, nullptr);
}

namespace nv {
PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructureNV;
PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructureNV;
PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemoryNV;
PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandleNV;
PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV;
PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructureNV;
PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV;
PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV;
PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV;

static void initNvidiaFunctions() {
  // Get VK_NV_ray_tracing related function pointers
  vkCreateAccelerationStructureNV = reinterpret_cast<PFN_vkCreateAccelerationStructureNV>(
      vkGetDeviceProcAddr(mg::vkContext.device, "vkCreateAccelerationStructureNV"));
  vkDestroyAccelerationStructureNV = reinterpret_cast<PFN_vkDestroyAccelerationStructureNV>(
      vkGetDeviceProcAddr(mg::vkContext.device, "vkDestroyAccelerationStructureNV"));
  vkBindAccelerationStructureMemoryNV = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryNV>(
      vkGetDeviceProcAddr(mg::vkContext.device, "vkBindAccelerationStructureMemoryNV"));
  vkGetAccelerationStructureHandleNV = reinterpret_cast<PFN_vkGetAccelerationStructureHandleNV>(
      vkGetDeviceProcAddr(mg::vkContext.device, "vkGetAccelerationStructureHandleNV"));
  vkGetAccelerationStructureMemoryRequirementsNV = reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsNV>(
      vkGetDeviceProcAddr(mg::vkContext.device, "vkGetAccelerationStructureMemoryRequirementsNV"));
  vkCmdBuildAccelerationStructureNV = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(
      vkGetDeviceProcAddr(mg::vkContext.device, "vkCmdBuildAccelerationStructureNV"));
  vkCreateRayTracingPipelinesNV = reinterpret_cast<PFN_vkCreateRayTracingPipelinesNV>(
      vkGetDeviceProcAddr(mg::vkContext.device, "vkCreateRayTracingPipelinesNV"));
  vkGetRayTracingShaderGroupHandlesNV = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesNV>(
      vkGetDeviceProcAddr(mg::vkContext.device, "vkGetRayTracingShaderGroupHandlesNV"));
  vkCmdTraceRaysNV =
      reinterpret_cast<PFN_vkCmdTraceRaysNV>(vkGetDeviceProcAddr(mg::vkContext.device, "vkCmdTraceRaysNV"));

}
} // namespace nv

bool initVulkan(GLFWwindow *window) {
  if (!createVulkanContext(window))
    return false;

  createCommandPool();
  createCommandBuffers();
  createCommandBufferSemaphores();
  createCommandBufferFences();

  createPipelineCache();
  createDescriptorPool();
  createDescriptorLayout();
  createPipelineLayout();

  initSampler();
  nv::initNvidiaFunctions();
  return true;
}

} // namespace mg