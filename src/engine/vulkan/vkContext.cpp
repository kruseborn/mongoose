#include "vkContext.h"

#include <float.h>
#include <memory>

#include "linearHeapAllocator.h"
#include "mg/mgAssert.h"
#include "mg/textureContainer.h"
#include "singleRenderPass.h"
#include "vkUtils.h"
#include "vkWindow.h"

namespace mg {

VulkanContext vkContext = {};

static constexpr uint32_t maxNrOfTextures = 128;

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
  vkDestroyPipelineLayout(mg::vkContext.device, mg::vkContext.pipelineLayout, nullptr);
  vkDestroyPipelineLayout(mg::vkContext.device, mg::vkContext.pipelineLayoutMultiTexture, nullptr);

  vkDestroyDescriptorSetLayout(mg::vkContext.device, mg::vkContext.descriptorSetLayout.ubo, nullptr);
  vkDestroyDescriptorSetLayout(mg::vkContext.device, mg::vkContext.descriptorSetLayout.texture, nullptr);

  vkDestroyCommandPool(mg::vkContext.device, mg::vkContext.commandPool, nullptr);

  vkDestroyDescriptorPool(mg::vkContext.device, mg::vkContext.descriptorPool, nullptr);

  destroyVulkanWindow();
  vkDestroyDevice(mg::vkContext.device, nullptr);
  destoyInstance();
}

static void createDescriptorLayout() {
  VkDescriptorSetLayoutBinding uboSamplerLayoutBindings = {};
  uboSamplerLayoutBindings.binding = 0;
  uboSamplerLayoutBindings.descriptorCount = 1;
  uboSamplerLayoutBindings.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  uboSamplerLayoutBindings.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
  descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount = 1;
  descriptorSetLayoutCreateInfo.pBindings = &uboSamplerLayoutBindings;

  checkResult(
      vkCreateDescriptorSetLayout(mg::vkContext.device, &descriptorSetLayoutCreateInfo, nullptr, &mg::vkContext.descriptorSetLayout.ubo));

  VkDescriptorSetLayoutBinding textureLayout = {};
  textureLayout.binding = 0;
  textureLayout.descriptorCount = 1;
  textureLayout.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  textureLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  descriptorSetLayoutCreateInfo.bindingCount = 1;
  descriptorSetLayoutCreateInfo.pBindings = &textureLayout;

  checkResult(vkCreateDescriptorSetLayout(mg::vkContext.device, &descriptorSetLayoutCreateInfo, nullptr,
                                          &mg::vkContext.descriptorSetLayout.texture));
}

static void createPipelineLayout() {
  const uint32_t nrOfDescriptorLayouts = 8;
  VkDescriptorSetLayout descriptorSetLayouts[nrOfDescriptorLayouts] = {
      mg::vkContext.descriptorSetLayout.ubo,     mg::vkContext.descriptorSetLayout.texture, mg::vkContext.descriptorSetLayout.texture,
      mg::vkContext.descriptorSetLayout.texture, mg::vkContext.descriptorSetLayout.texture, mg::vkContext.descriptorSetLayout.texture,
      mg::vkContext.descriptorSetLayout.texture, mg::vkContext.descriptorSetLayout.texture,
  };

  VkPipelineLayoutCreateInfo layoutCreateInfo = {};
  layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutCreateInfo.setLayoutCount = 3;
  layoutCreateInfo.pSetLayouts = descriptorSetLayouts;

  VkPushConstantRange pushConstantRange = {};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.size = 256;

  layoutCreateInfo.pPushConstantRanges = &pushConstantRange;
  layoutCreateInfo.pushConstantRangeCount = 1;
  checkResult(vkCreatePipelineLayout(mg::vkContext.device, &layoutCreateInfo, nullptr, &mg::vkContext.pipelineLayout));

  layoutCreateInfo.setLayoutCount = nrOfDescriptorLayouts;
  checkResult(vkCreatePipelineLayout(mg::vkContext.device, &layoutCreateInfo, nullptr, &mg::vkContext.pipelineLayoutMultiTexture));
}

static void createPipelineCache() {
  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
  pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  VkResult err = vkCreatePipelineCache(mg::vkContext.device, &pipelineCacheCreateInfo, nullptr, &mg::vkContext.pipelineCache);
  mgAssert(!err);
}

static void createCommandPool() {
  VkCommandPoolCreateInfo poolCreateInfo = {};
  poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolCreateInfo.queueFamilyIndex = mg::vkContext.graphicsQueueFamilyIndex;
  poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  checkResult(vkCreateCommandPool(mg::vkContext.device, &poolCreateInfo, nullptr, &mg::vkContext.commandPool));
}

static void createDescriptorPool() {
  const uint32_t poolSize = 2;
  VkDescriptorPoolSize descriptorPoolSize[poolSize] = {};

  descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  descriptorPoolSize[0].descriptorCount = 2;

  descriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorPoolSize[1].descriptorCount = maxNrOfTextures;

  VkDescriptorPoolCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  createInfo.poolSizeCount = poolSize;
  createInfo.pPoolSizes = descriptorPoolSize;
  createInfo.maxSets = maxNrOfTextures + 32;
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

  checkResult(vkCreateSampler(mg::vkContext.device, &sampler_create_info, NULL, &mg::vkContext.sampler.pointBorderSampler));

  sampler_create_info.magFilter = VK_FILTER_LINEAR;
  sampler_create_info.minFilter = VK_FILTER_LINEAR;
  sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  checkResult(vkCreateSampler(mg::vkContext.device, &sampler_create_info, NULL, &mg::vkContext.sampler.linearBorderSampler));

  sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  checkResult(vkCreateSampler(mg::vkContext.device, &sampler_create_info, NULL, &mg::vkContext.sampler.linearEdgeSampler));

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
  return true;
}

} // namespace mg