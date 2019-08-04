#include "ray_utils.h"
#include "vulkan/vkContext.h"
#include <glm/glm.hpp>
#include <iostream>
#include <mg/mgSystem.h>
#include <mg/textureContainer.h>
#include <vulkan/vkContext.h>

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

static void createBottomLevelAccelerationStructure(RayInfo *rayInfo) {

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
  meshInfo.indices = (unsigned char*)indices;
  meshInfo.indicesSizeInBytes = sizeof(indices);

  auto meshId = mg::mgSystem.meshContainer.createMesh(meshInfo);
  auto mesh = mg::getMesh(meshId);

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

  rayInfo->geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
  rayInfo->geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
  rayInfo->geometry.geometry.triangles = triangles;
  rayInfo->geometry.geometry.aabbs = geometryAABBNV;
  rayInfo->geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;
  // VK_GEOMETRY_OPAQUE_BIT_NV indicates that this geometry does not invoke the
  // any-hit shaders even if present in a hit group.

  VkAccelerationStructureInfoNV accelerationStructureInfo = {};
  accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
  accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
  accelerationStructureInfo.geometryCount = 1;
  accelerationStructureInfo.pGeometries = &rayInfo->geometry;

  VkAccelerationStructureCreateInfoNV accelerationStructureCreateInfo = {};
  accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
  accelerationStructureCreateInfo.info = accelerationStructureInfo;
  mg::checkResult(mg::nv::vkCreateAccelerationStructureNV(mg::vkContext.device, &accelerationStructureCreateInfo,
                                                          nullptr, &rayInfo->bottomLevelAS.accelerationStructure));

  createAndBindASDeviceMemory(&rayInfo->bottomLevelAS);
}

static void createTopLevelAccelerationStructure(RayInfo *rayInfo) {
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
}

static void createGeometryInstance(RayInfo *rayInfo) {
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

  VkBufferCreateInfo bufferCreateInfo = {};
  bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCreateInfo.size = sizeof(geometryInstance);
  bufferCreateInfo.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;

  mg::checkResult(vkCreateBuffer(mg::vkContext.device, &bufferCreateInfo, nullptr, &rayInfo->instanceBuffer));

  VkMemoryRequirements vkMemoryRequirements = {};
  vkGetBufferMemoryRequirements(mg::vkContext.device, rayInfo->instanceBuffer, &vkMemoryRequirements);

  const auto memoryIndex =
      mg::findMemoryTypeIndex(mg::vkContext.physicalDeviceMemoryProperties, vkMemoryRequirements.memoryTypeBits,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  auto instanceHeapAllocation = mg::mgSystem.meshDeviceMemoryAllocator.allocateDeviceOnlyMemory(
      memoryIndex, vkMemoryRequirements.size, vkMemoryRequirements.alignment);

  mg::checkResult(vkBindBufferMemory(mg::vkContext.device, rayInfo->instanceBuffer, instanceHeapAllocation.deviceMemory,
                                     instanceHeapAllocation.offset));
}

static void createScrathMemory(RayInfo *rayInfo) {
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

  VkBufferCreateInfo bufferCreateInfo = {};
  bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCreateInfo.size = scratchBufferSize;
  bufferCreateInfo.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;

  mg::checkResult(vkCreateBuffer(mg::vkContext.device, &bufferCreateInfo, nullptr, &rayInfo->scratchBuffer));

  VkMemoryRequirements vkMemoryRequirements = {};
  vkGetBufferMemoryRequirements(mg::vkContext.device, rayInfo->scratchBuffer, &vkMemoryRequirements);

  const auto memoryIndex =
      mg::findMemoryTypeIndex(mg::vkContext.physicalDeviceMemoryProperties, vkMemoryRequirements.memoryTypeBits,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  auto instanceHeapAllocation = mg::mgSystem.meshDeviceMemoryAllocator.allocateDeviceOnlyMemory(
      memoryIndex, vkMemoryRequirements.size, vkMemoryRequirements.alignment);

  mg::checkResult(vkBindBufferMemory(mg::vkContext.device, rayInfo->scratchBuffer, instanceHeapAllocation.deviceMemory,
                                     instanceHeapAllocation.offset));
}

static void buildAccelerationStructures(RayInfo *rayInfo) {
  VkCommandBufferAllocateInfo vkCommandBufferAllocateInfo = {};
  vkCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  vkCommandBufferAllocateInfo.commandPool = mg::vkContext.commandPool;
  vkCommandBufferAllocateInfo.commandBufferCount = 1;
  vkCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  VkCommandBuffer commandBuffer;
  mg::checkResult(vkAllocateCommandBuffers(mg::vkContext.device, &vkCommandBufferAllocateInfo, &commandBuffer));

  VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {};
  vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &vkCommandBufferBeginInfo);

  // bottom level
  VkAccelerationStructureInfoNV accelerationStructureInfo = {};
  accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
  accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
  accelerationStructureInfo.geometryCount = 1;
  accelerationStructureInfo.pGeometries = &rayInfo->geometry;

  mg::nv::vkCmdBuildAccelerationStructureNV(commandBuffer, &accelerationStructureInfo, VK_NULL_HANDLE, 0, VK_FALSE,
                                            rayInfo->bottomLevelAS.accelerationStructure, VK_NULL_HANDLE,
                                            rayInfo->scratchBuffer, 0);

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

  mg::nv::vkCmdBuildAccelerationStructureNV(commandBuffer, &accelerationStructureInfo, rayInfo->instanceBuffer, 0,
                                            VK_FALSE, rayInfo->topLevelAS.accelerationStructure, VK_NULL_HANDLE,
                                            rayInfo->scratchBuffer, 0);

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
                       VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

  mg::checkResult(vkEndCommandBuffer(commandBuffer));
  VkSubmitInfo vkSubmitInfo = {};
  vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  vkSubmitInfo.commandBufferCount = 1;
  vkSubmitInfo.pCommandBuffers = &commandBuffer;


  mg::checkResult(vkQueueSubmit(mg::vkContext.queue, 1, &vkSubmitInfo, VK_NULL_HANDLE));
  
  mg::waitForDeviceIdle();
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
  mg::CreateImageStorageInfo createImageStorageInfo = {};
  createImageStorageInfo.format = mg::vkContext.swapChain->format;
  createImageStorageInfo.id = "storage image";
  createImageStorageInfo.size = {mg::vkContext.screen.width, mg::vkContext.screen.height, 1};
  rayInfo->storageImageId = mg::mgSystem.storageContainer.createImageStorage(createImageStorageInfo);

  getRayTracingProperties(rayInfo);
  createBottomLevelAccelerationStructure(rayInfo);
  createTopLevelAccelerationStructure(rayInfo);

  createGeometryInstance(rayInfo);
  createScrathMemory(rayInfo);

  buildAccelerationStructures(rayInfo);
}