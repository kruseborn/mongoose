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
  VkGeometryNV geometry;
  VkBuffer scratchBuffer;
  VkBuffer instanceBuffer;
  VkPhysicalDeviceRayTracingPropertiesNV rayTracingProperties;
  mg::StorageId storageImageId;
  VkDescriptorSet topLevelASDescriptorSet;
};

void createScene(RayInfo *rayInfo);