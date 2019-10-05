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
  mg::StorageId storageAccumulationImageID;
  mg::StorageId storageSpheresId;
  VkDescriptorSet topLevelASDescriptorSet;
  mg::MeshId triangleId;
  bool resetAccumulationImage;
};

struct World {
  enum MATERIAL { LAMBERTH, METAL, DIELECTRIC };
  std::vector<glm::vec4> positions;
  std::vector<glm::vec4> albedos;
  std::vector<MATERIAL> materials;
  mg::TextureId blueNoise;
};

struct Sphere {
  World *world;
  const glm::vec4 &position;
  const glm::vec4 &albedo;
  World::MATERIAL material;

};

inline void addSphere(const Sphere &sphere) {
  sphere.world->positions.emplace_back(sphere.position);
  sphere.world->albedos.emplace_back(sphere.albedo);
  sphere.world->materials.emplace_back(sphere.material);
}

void createRayInfo(const World &world, RayInfo *rayInfo);
void destroyRayInfo(RayInfo *rayInfo);
