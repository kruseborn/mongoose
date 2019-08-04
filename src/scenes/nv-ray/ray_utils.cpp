#include "ray_utils.h"
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

static void createBottomLevelAccelerationStructure(RayInfo *rayInfo, VkGeometryNV *geometry) {
  struct Vertex {
    glm::vec3 pos;
  };
  Vertex vertices[3] = {{{1.0f, 1.0f, 0.0f}}, {{-1.0f, 1.0f, 0.0f}}, {{0.0f, -1.0f, 0.0f}}};
  uint32_t indices[] = {0, 1, 2};

  mg::CreateMeshInfo meshInfo = {};
  meshInfo.id = "triangle";
  meshInfo.vertices = (unsigned char *)vertices;
  meshInfo.verticesSizeInBytes = sizeof(vertices);
  meshInfo.nrOfIndices = 3;
  meshInfo.indices = (unsigned char *)indices;
  meshInfo.indicesSizeInBytes = sizeof(indices);

  auto meshId = mg::mgSystem.meshContainer.createMesh(meshInfo);
  auto mesh = mg::getMesh(meshId);

  mg::waitForDeviceIdle();

  VkGeometryDataNV geometryDataNV = {};

  VkGeometryTrianglesNV triangles = {};
  triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
  triangles.vertexData = mesh.buffer;
  triangles.vertexCount = 3;
  triangles.vertexStride = sizeof(Vertex);
  triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;

  triangles.indexData = mesh.buffer;
  triangles.indexOffset = mesh.indicesOffset;
  triangles.indexType = VK_INDEX_TYPE_UINT32;
  triangles.indexCount = 3;

  VkGeometryAABBNV geometryAABBNV = {};
  geometryAABBNV.sType = VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;

  geometry->sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
  geometry->geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
  geometry->geometry.triangles = triangles;
  geometry->geometry.aabbs = geometryAABBNV;
  geometry->flags = VK_GEOMETRY_OPAQUE_BIT_NV;
  // VK_GEOMETRY_OPAQUE_BIT_NV indicates that this geometry does not invoke the
  // any-hit shaders even if present in a hit group.

  VkAccelerationStructureInfoNV accelerationStructureInfo = {};
  accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
  accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
  accelerationStructureInfo.geometryCount = 1;
  accelerationStructureInfo.pGeometries = geometry;

  VkAccelerationStructureCreateInfoNV accelerationStructureCreateInfo = {};
  accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
  accelerationStructureCreateInfo.info = accelerationStructureInfo;
  mg::checkResult(mg::nv::vkCreateAccelerationStructureNV(mg::vkContext.device, &accelerationStructureCreateInfo,
                                                          nullptr, &rayInfo->bottomLevelAS.accelerationStructure));

  createAndBindASDeviceMemory(&rayInfo->bottomLevelAS);
}

static void createTopLevelAccelerationStructure(RayInfo *rayInfo, Buffer *instanceBuffer) {
  VkAccelerationStructureInfoNV accelerationStructureInfo = {};
  accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
  accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
  accelerationStructureInfo.instanceCount = 1;
  accelerationStructureInfo.geometryCount = 0;

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

  VkGeometryInstance geometryInstance = {};
  glm::mat3x4 transform = {
      1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
  };
  geometryInstance.transform = transform;
  geometryInstance.instanceId = 0;
  geometryInstance.mask = 0xff;
  geometryInstance.instanceOffset = 0;
  geometryInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
  geometryInstance.accelerationStructureHandle = rayInfo->bottomLevelAS.handle;

  VkGeometryInstance *instanceData = (VkGeometryInstance *)mg::mgSystem.linearHeapAllocator.allocateBuffer(
      sizeof(geometryInstance), &instanceBuffer->buffer, &instanceBuffer->offset);

  memcpy(instanceData, &geometryInstance, sizeof(geometryInstance));
}

static void createScrathMemory(RayInfo *rayInfo, Buffer *scratchBuffer) {
  // Acceleration structure build requires some scratch space to store temporary information
  VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
  memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
  memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;

  VkMemoryRequirements2 memReqBottomLevelAS = {};
  memReqBottomLevelAS.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
  memoryRequirementsInfo.accelerationStructure = rayInfo->bottomLevelAS.accelerationStructure;
  mg::nv::vkGetAccelerationStructureMemoryRequirementsNV(mg::vkContext.device, &memoryRequirementsInfo,
                                                         &memReqBottomLevelAS);

  VkMemoryRequirements2 memReqTopLevelAS = {};
  memReqTopLevelAS.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
  memoryRequirementsInfo.accelerationStructure = rayInfo->topLevelAS.accelerationStructure;
  mg::nv::vkGetAccelerationStructureMemoryRequirementsNV(mg::vkContext.device, &memoryRequirementsInfo,
                                                         &memReqTopLevelAS);

  const VkDeviceSize scratchBufferSize =
      std::max(memReqBottomLevelAS.memoryRequirements.size, memReqTopLevelAS.memoryRequirements.size);

  mg::mgSystem.linearHeapAllocator.allocateBuffer(scratchBufferSize, &scratchBuffer->buffer, &scratchBuffer->offset);
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

static void buildAccelerationStructures(RayInfo *rayInfo, Buffer *scratchBuffer, Buffer *instanceBuffer,
                                        VkGeometryNV *geometry) {
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

  VkAccelerationStructureInfoNV accelerationStructureInfo = {};
  accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
  accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
  accelerationStructureInfo.geometryCount = 1;
  accelerationStructureInfo.pGeometries = geometry;

  mg::nv::vkCmdBuildAccelerationStructureNV(commandBuffer, &accelerationStructureInfo, VK_NULL_HANDLE, 0, VK_FALSE,
                                            rayInfo->bottomLevelAS.accelerationStructure, VK_NULL_HANDLE,
                                            scratchBuffer->buffer, scratchBuffer->offset);

  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask =
      VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
  memoryBarrier.dstAccessMask =
      VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
                       VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

  accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
  accelerationStructureInfo.pGeometries = 0;
  accelerationStructureInfo.geometryCount = 0;
  accelerationStructureInfo.instanceCount = 1;

  mg::nv::vkCmdBuildAccelerationStructureNV(commandBuffer, &accelerationStructureInfo, instanceBuffer->buffer,
                                            instanceBuffer->offset, VK_FALSE, rayInfo->topLevelAS.accelerationStructure,
                                            VK_NULL_HANDLE, scratchBuffer->buffer, scratchBuffer->offset);

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

static void getRayTracingProperties(RayInfo *rayInfo) {
  VkPhysicalDeviceRayTracingPropertiesNV physicalDeviceRayTracingPropertiesNV = {};
  physicalDeviceRayTracingPropertiesNV.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
  VkPhysicalDeviceProperties2 physicalDeviceProperties2 = {};
  physicalDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  physicalDeviceProperties2.pNext = &physicalDeviceRayTracingPropertiesNV;
  vkGetPhysicalDeviceProperties2(mg::vkContext.physicalDevice, &physicalDeviceProperties2);

  rayInfo->rayTracingProperties = physicalDeviceRayTracingPropertiesNV;
}

void createScene(RayInfo *rayInfo) {
  Buffer instanceBuffer = {};
  Buffer scratchBuffer{};
  VkGeometryNV geometry = {};

  mg::CreateImageStorageInfo createImageStorageInfo = {};
  createImageStorageInfo.format = mg::vkContext.swapChain->format;
  createImageStorageInfo.id = "storage image";
  createImageStorageInfo.size = {mg::vkContext.screen.width, mg::vkContext.screen.height, 1};
  rayInfo->storageImageId = mg::mgSystem.storageContainer.createImageStorage(createImageStorageInfo);

  getRayTracingProperties(rayInfo);
  createBottomLevelAccelerationStructure(rayInfo, &geometry);
  createTopLevelAccelerationStructure(rayInfo, &instanceBuffer);

  createScrathMemory(rayInfo, &scratchBuffer);

  buildAccelerationStructures(rayInfo, &scratchBuffer, &instanceBuffer, &geometry);
}