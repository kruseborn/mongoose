#include "ray_utils.h"
#include "mg/meshUtils.h"
#include "vulkan/vkContext.h"
#include <glm/glm.hpp>
#include <iostream>
#include <mg/mgSystem.h>
#include <mg/textureContainer.h>
#include <vulkan/vkContext.h>

struct Buffer {
  VkBuffer buffer;
  VkDeviceSize offset;
};

struct ScratchBuffer {
  Buffer buffer;
  std::vector<uint32_t> offsets;
};

// https://developer.nvidia.com/rtx/raytracing/vkray
struct VkGeometryInstance {
  /// Transform matrix, containing only the top 3 rows
  glm::mat3x4 transform;
  /// Instance index
  uint32_t instanceId : 24;
  /// Visibility mask
  uint32_t mask : 8;
  /// Index of the hit group which will be invoked when a ray hits the instance
  uint32_t instanceOffset : 24;
  /// Instance flags, such as culling
  uint32_t flags : 8;
  /// Opaque handle of the bottom-level acceleration structure
  uint64_t accelerationStructureHandle;
};

static void createStorageImages(RayInfo *rayInfo) {
  {
    mg::CreateImageStorageInfo createImageStorageInfo = {};
    createImageStorageInfo.format = mg::vkContext.swapChain->format;
    createImageStorageInfo.id = "storage image";
    createImageStorageInfo.size = {mg::vkContext.screen.width, mg::vkContext.screen.height, 1};
    rayInfo->storageImageId = mg::mgSystem.storageContainer.createImageStorage(createImageStorageInfo);
  }
  {
    mg::CreateImageStorageInfo createImageStorageInfo = {};
    createImageStorageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    createImageStorageInfo.id = "accumulationImage";
    createImageStorageInfo.size = {mg::vkContext.screen.width, mg::vkContext.screen.height, 1};
    rayInfo->storageAccumulationImageID = mg::mgSystem.storageContainer.createImageStorage(createImageStorageInfo);
  }
}
static void destroyStorageImges(RayInfo *rayInfo) {
  mg::mgSystem.storageContainer.removeStorage(rayInfo->storageImageId);
  mg::mgSystem.storageContainer.removeStorage(rayInfo->storageAccumulationImageID);
}

static void createAndBindASDeviceMemory(AccelerationStructure *levelAS) {

  VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo = {};
  memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
  memoryRequirementsInfo.accelerationStructure = levelAS->accelerationStructure;

  VkMemoryRequirements2 memoryRequirements2 = {};
  mg::nv::vkGetAccelerationStructureMemoryRequirementsNV(mg::vkContext.device, &memoryRequirementsInfo,
                                                         &memoryRequirements2);

  const auto memoryIndex = mg::findMemoryTypeIndex(mg::vkContext.physicalDeviceMemoryProperties,
                                                   memoryRequirements2.memoryRequirements.memoryTypeBits,
                                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  levelAS->deviceHeapAllocation = mg::mgSystem.meshDeviceMemoryAllocator.allocateDeviceOnlyMemory(
      memoryIndex, memoryRequirements2.memoryRequirements.size, memoryRequirements2.memoryRequirements.alignment);

  VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo = {};
  accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
  accelerationStructureMemoryInfo.accelerationStructure = levelAS->accelerationStructure;
  accelerationStructureMemoryInfo.memory = levelAS->deviceHeapAllocation.deviceMemory;
  accelerationStructureMemoryInfo.memoryOffset = levelAS->deviceHeapAllocation.offset;
  mg::checkResult(
      mg::nv::vkBindAccelerationStructureMemoryNV(mg::vkContext.device, 1, &accelerationStructureMemoryInfo));

  mg::checkResult(mg::nv::vkGetAccelerationStructureHandleNV(mg::vkContext.device, levelAS->accelerationStructure,
                                                             sizeof(uint64_t), &levelAS->handle));
}

static void createBottomLevelAccelerationStructureAabb(const World &world, RayInfo *rayInfo) {
  std::vector<glm::vec3> aabb;
  aabb.reserve(world.positions.size() * 2);

  for (uint64_t i = 0; i < world.positions.size(); i++) {
    aabb.push_back(glm::vec3{world.positions[i].x, world.positions[i].y, world.positions[i].z} - world.positions[i].w);
    aabb.push_back(glm::vec3{world.positions[i].x, world.positions[i].y, world.positions[i].z} + world.positions[i].w);
  }
  rayInfo->storageSpheresId =
      mg::mgSystem.storageContainer.createStorage(aabb.data(), mg::sizeofContainerInBytes(aabb));
  auto storage = mg::mgSystem.storageContainer.getStorage(rayInfo->storageSpheresId);

  mg::waitForDeviceIdle();

  rayInfo->bottomLevelASs.reserve(world.positions.size());
  for (uint64_t i = 0; i < world.positions.size(); i++) {
    VkGeometryNV geometry = {};
    geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
    geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_NV;
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;

    geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
    geometry.geometry.aabbs.aabbData = storage.buffer;
    geometry.geometry.aabbs.offset = i * sizeof(glm::vec3) * 2;
    geometry.geometry.aabbs.numAABBs = 1;
    geometry.geometry.aabbs.stride = sizeof(glm::vec3) * 2;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

    VkAccelerationStructureInfoNV accelerationStructureInfo = {};
    accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
    accelerationStructureInfo.geometryCount = 1;
    accelerationStructureInfo.pGeometries = &geometry;

    VkAccelerationStructureCreateInfoNV accelerationStructureCreateInfo = {};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    accelerationStructureCreateInfo.info = accelerationStructureInfo;
    VkAccelerationStructureNV vkAccelerationStructureNV;
    mg::checkResult(mg::nv::vkCreateAccelerationStructureNV(mg::vkContext.device, &accelerationStructureCreateInfo,
                                                            nullptr, &vkAccelerationStructureNV));

    rayInfo->bottomLevelASs.push_back({.accelerationStructure = vkAccelerationStructureNV, .vkGeometryNV = geometry});
    createAndBindASDeviceMemory(&rayInfo->bottomLevelASs.back());
  }
}

static void createTopLevelAccelerationStructure(RayInfo *rayInfo) {
  VkAccelerationStructureInfoNV accelerationStructureInfo = {};
  accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
  accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
  accelerationStructureInfo.instanceCount = uint32_t(rayInfo->bottomLevelASs.size());

  VkAccelerationStructureCreateInfoNV accelerationStructureCreateInfo = {};
  accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
  accelerationStructureCreateInfo.info = accelerationStructureInfo;
  mg::checkResult(mg::nv::vkCreateAccelerationStructureNV(mg::vkContext.device, &accelerationStructureCreateInfo,
                                                          nullptr, &rayInfo->topLevelAS.accelerationStructure));

  createAndBindASDeviceMemory(&rayInfo->topLevelAS);

  VkDescriptorSetAllocateInfo vkDescriptorSetAllocateInfo = {};
  vkDescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  vkDescriptorSetAllocateInfo.descriptorPool = mg::vkContext.descriptorPool;
  vkDescriptorSetAllocateInfo.descriptorSetCount = 1;
  vkDescriptorSetAllocateInfo.pSetLayouts = &mg::vkContext.descriptorSetLayout.accelerationStructure;
  vkAllocateDescriptorSets(mg::vkContext.device, &vkDescriptorSetAllocateInfo, &rayInfo->topLevelASDescriptorSet);

  VkWriteDescriptorSetAccelerationStructureNV descriptorAccelerationStructureInfo = {};
  descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
  descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
  descriptorAccelerationStructureInfo.pAccelerationStructures = &rayInfo->topLevelAS.accelerationStructure;

  VkWriteDescriptorSet accelerationStructureWrite = {};
  accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
  accelerationStructureWrite.dstSet = rayInfo->topLevelASDescriptorSet;
  accelerationStructureWrite.dstBinding = 0;
  accelerationStructureWrite.descriptorCount = 1;
  accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;

  vkUpdateDescriptorSets(mg::vkContext.device, 1, &accelerationStructureWrite, 0, VK_NULL_HANDLE);
}

static Buffer createInstances(const World &world, const RayInfo &info) {
  Buffer instanceBuffer = {};
  VkGeometryInstance *instanceData = (VkGeometryInstance *)mg::mgSystem.linearHeapAllocator.allocateBuffer(
      sizeof(VkGeometryInstance) * info.bottomLevelASs.size(), &instanceBuffer.buffer, &instanceBuffer.offset);

  mgAssert(world.materials.size() == info.bottomLevelASs.size());
  mgAssert(world.positions.size() == info.bottomLevelASs.size());

  for (uint64_t i = 0; i < info.bottomLevelASs.size(); i++) {
    VkGeometryInstance geometryInstance = {};
    // clang-format off
    glm::mat3x4 transform = {
        1.0f, 0.0f, 0.0f, 0.0f, 
        0.0f, 1.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 1.0f, 0.0f,
    };
    // clang-format on
    geometryInstance.transform = transform;
    geometryInstance.instanceId = i;
    geometryInstance.mask = 0xff;
    geometryInstance.instanceOffset = world.materials[i];
    geometryInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
    geometryInstance.accelerationStructureHandle = info.bottomLevelASs[i].handle;

    memcpy((char *)(instanceData) + i * sizeof(VkGeometryInstance), &geometryInstance, sizeof(VkGeometryInstance));
  }
  return instanceBuffer;
}

static ScratchBuffer createScrathMemory(RayInfo *rayInfo) {
  ScratchBuffer scratchBuffer = {};
  scratchBuffer.offsets.reserve(rayInfo->bottomLevelASs.size());

  uint64_t scratchBufferBottomLevels = 0;
  for (uint64_t i = 0; i < rayInfo->bottomLevelASs.size(); i++) {
    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
    memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;

    VkMemoryRequirements2 memReqBottomLevelAS = {};
    memReqBottomLevelAS.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
    memoryRequirementsInfo.accelerationStructure = rayInfo->bottomLevelASs[i].accelerationStructure;
    mg::nv::vkGetAccelerationStructureMemoryRequirementsNV(mg::vkContext.device, &memoryRequirementsInfo,
                                                           &memReqBottomLevelAS);
    scratchBuffer.offsets.push_back(uint32_t(scratchBufferBottomLevels));
    scratchBufferBottomLevels += memReqBottomLevelAS.memoryRequirements.size;
  }
  VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
  memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
  memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;

  VkMemoryRequirements2 memReqTopLevelAS = {};
  memReqTopLevelAS.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
  memoryRequirementsInfo.accelerationStructure = rayInfo->topLevelAS.accelerationStructure;
  mg::nv::vkGetAccelerationStructureMemoryRequirementsNV(mg::vkContext.device, &memoryRequirementsInfo,
                                                         &memReqTopLevelAS);

  const auto scratchBufferSize = std::max(scratchBufferBottomLevels, memReqTopLevelAS.memoryRequirements.size);
  mg::mgSystem.linearHeapAllocator.allocateBuffer(scratchBufferSize, &scratchBuffer.buffer.buffer,
                                                  &scratchBuffer.buffer.offset);

  return scratchBuffer;
}

inline VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool commandPool, VkCommandBufferLevel level,
                                                             uint32_t bufferCount) {
  VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
  commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  commandBufferAllocateInfo.commandPool = commandPool;
  commandBufferAllocateInfo.level = level;
  commandBufferAllocateInfo.commandBufferCount = bufferCount;
  return commandBufferAllocateInfo;
}

static void buildAccelerationStructures(const RayInfo &rayInfo, const Buffer &instanceBuffer,
                                        const ScratchBuffer &scratchBuffer) {
  VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
  commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  commandBufferAllocateInfo.commandPool = mg::vkContext.commandPool;
  commandBufferAllocateInfo.commandBufferCount = 1;
  commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  VkCommandBuffer commandBuffer;
  mg::checkResult(vkAllocateCommandBuffers(mg::vkContext.device, &commandBufferAllocateInfo, &commandBuffer));

  VkCommandBufferBeginInfo commandBufferBeginInfo = {};
  commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  mg::checkResult(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

  for (uint64_t i = 0; i < rayInfo.bottomLevelASs.size(); i++) {
    VkAccelerationStructureInfoNV accelerationStructureInfo = {};
    accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
    accelerationStructureInfo.geometryCount = 1;
    accelerationStructureInfo.pGeometries = &rayInfo.bottomLevelASs[i].vkGeometryNV;

    mg::nv::vkCmdBuildAccelerationStructureNV(commandBuffer, &accelerationStructureInfo, VK_NULL_HANDLE, 0, VK_FALSE,
                                              rayInfo.bottomLevelASs[i].accelerationStructure, VK_NULL_HANDLE,
                                              scratchBuffer.buffer.buffer,
                                              scratchBuffer.buffer.offset + scratchBuffer.offsets[i]);
  }

  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask =
      VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
  memoryBarrier.dstAccessMask =
      VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
                       VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

  VkAccelerationStructureInfoNV accelerationStructureInfo = {};
  accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
  accelerationStructureInfo.instanceCount = uint32_t(rayInfo.bottomLevelASs.size());

  mg::nv::vkCmdBuildAccelerationStructureNV(commandBuffer, &accelerationStructureInfo, instanceBuffer.buffer,
                                            instanceBuffer.offset, VK_FALSE, rayInfo.topLevelAS.accelerationStructure,
                                            VK_NULL_HANDLE, scratchBuffer.buffer.buffer, scratchBuffer.buffer.offset);

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
                       VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

  mg::checkResult(vkEndCommandBuffer(commandBuffer));

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  VkFence fence;
  mg::checkResult(vkCreateFence(mg::vkContext.device, &fenceInfo, nullptr, &fence));

  mg::checkResult(vkQueueSubmit(mg::vkContext.queue, 1, &submitInfo, fence));
  mg::checkResult(vkWaitForFences(mg::vkContext.device, 1, &fence, VK_TRUE, UINT64_MAX));
  vkDestroyFence(mg::vkContext.device, fence, nullptr);
  vkFreeCommandBuffers(mg::vkContext.device, mg::vkContext.commandPool, 1, &commandBuffer);
}

static VkPhysicalDeviceRayTracingPropertiesNV getRayTracingProperties(RayInfo *rayInfo) {
  VkPhysicalDeviceRayTracingPropertiesNV physicalDeviceRayTracingPropertiesNV = {};
  physicalDeviceRayTracingPropertiesNV.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
  VkPhysicalDeviceProperties2 physicalDeviceProperties2 = {};
  physicalDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  physicalDeviceProperties2.pNext = &physicalDeviceRayTracingPropertiesNV;
  vkGetPhysicalDeviceProperties2(mg::vkContext.physicalDevice, &physicalDeviceProperties2);

  return physicalDeviceRayTracingPropertiesNV;
}

void createRayInfo(const World &world, RayInfo *rayInfo) {
  createStorageImages(rayInfo);
  rayInfo->rayTracingProperties = getRayTracingProperties(rayInfo);
  createBottomLevelAccelerationStructureAabb(world, rayInfo);
  createTopLevelAccelerationStructure(rayInfo);

  auto instanceBuffer = createInstances(world, *rayInfo);
  auto scratchBuffer = createScrathMemory(rayInfo);

  buildAccelerationStructures(*rayInfo, instanceBuffer, scratchBuffer);
}

void resetSizeStorageImages(RayInfo *rayInfo) {
  destroyStorageImges(rayInfo);
  createStorageImages(rayInfo);
}

void destroyRayInfo(RayInfo *rayInfo) {
  destroyStorageImges(rayInfo);
  mg::mgSystem.storageContainer.removeStorage(rayInfo->storageSpheresId);

  for (uint64_t i = 0; i < rayInfo->bottomLevelASs.size(); i++) {
    mg::mgSystem.meshDeviceMemoryAllocator.freeDeviceOnlyMemory(rayInfo->bottomLevelASs[i].deviceHeapAllocation);
    mg::nv::vkDestroyAccelerationStructureNV(mg::vkContext.device, rayInfo->bottomLevelASs[i].accelerationStructure,
                                             nullptr);
  }
  mg::mgSystem.meshDeviceMemoryAllocator.freeDeviceOnlyMemory(rayInfo->topLevelAS.deviceHeapAllocation);
  mg::nv::vkDestroyAccelerationStructureNV(mg::vkContext.device, rayInfo->topLevelAS.accelerationStructure, nullptr);
  vkFreeDescriptorSets(mg::vkContext.device, mg::vkContext.descriptorPool, 1, &rayInfo->topLevelASDescriptorSet);

  *rayInfo = {};
}
