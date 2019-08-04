#pragma once
#include "vulkan/vkContext.h"
#include "mg/mgSystem.h"

struct AccelerationStructure {
  mg::DeviceHeapAllocation deviceHeapAllocation;
  VkAccelerationStructureNV accelerationStructure;
  uint64_t handle;
};

struct RayInfo {
  AccelerationStructure bottomLevelAS;
  AccelerationStructure topLevelAS;
  VkPhysicalDeviceRayTracingPropertiesNV rayTracingProperties;
  mg::StorageId storageImageId;
  VkDescriptorSet topLevelASDescriptorSet;
  mg::MeshId triangleId;
};

void createRayInfo(RayInfo *rayInfo);
void destroyRayInfo(RayInfo *rayInfo);