#pragma once
#include "vulkan/vkContext.h"
#include "mg/mgSystem.h"

struct AccelerationStructure {
  mg::DeviceHeapAllocation deviceHeapAllocation;
  VkAccelerationStructureNV accelerationStructure;
  VkGeometryNV vkGeometryNV;
  uint64_t handle;
};

struct RayInfo {
  std::vector<AccelerationStructure> bottomLevelASs;
  AccelerationStructure topLevelAS;
  VkPhysicalDeviceRayTracingPropertiesNV rayTracingProperties;
  mg::StorageId storageImageId;
  mg::StorageId storageSpheresId;
  VkDescriptorSet topLevelASDescriptorSet;
  mg::MeshId triangleId;
};

struct World {
  std::vector<glm::vec4> spheres;
};

void createRayInfo(const World &world, RayInfo *rayInfo);
void destroyRayInfo(RayInfo *rayInfo);