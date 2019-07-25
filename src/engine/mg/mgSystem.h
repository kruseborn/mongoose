#pragma once
#include <string>

#include "mg/fonts.h"
#include "mg/meshContainer.h"
#include "mg/mgUtils.h"
#include "mg/storageContainer.h"
#include "mg/textureContainer.h"
#include "vulkan/imguiOverlay.h"
#include "vulkan/linearHeapAllocator.h"
#include "vulkan/pipelineContainer.h"
#include "vulkan/singleRenderpass.h"

namespace mg {

struct MgSystem : mg::nonCopyable {
  PipelineContainer pipelineContainer;
  Pipelines pipelines;

  TextureContainer textureContainer;
  MeshContainer meshContainer;
  StorageContainer storageContainer;

  LinearHeapAllocator linearHeapAllocator;
  DeviceMemoryAllocator meshDeviceMemoryAllocator;
  DeviceMemoryAllocator textureDeviceMemoryAllocator;
  DeviceMemoryAllocator storageDeviceMemoryAllocator;

  Fonts fonts;
  Imgui imguiOverlay;
};

void createMgSystem(MgSystem *system);
void destroyMgSystem(MgSystem *system);

extern MgSystem mgSystem;

// helper function
inline VkDescriptorSet getTextureDescriptorSet() { return mgSystem.textureContainer.getDescriptorSet(); }
inline VkDescriptorSet getTextureDescriptorSet3D() { return mgSystem.textureContainer.getDescriptorSet3D(); }

inline uint32_t getTexture2DDescriptorIndex(mg::TextureId id) { return mgSystem.textureContainer.getTexture2DDescriptorIndex(id); }
inline uint32_t getTexture3DDescriptorIndex(mg::TextureId id) { return mgSystem.textureContainer.getTexture3DDescriptorIndex(id); }
inline Texture getTexture(mg::TextureId id) { return mgSystem.textureContainer.getTexture(id); }
inline void removeTexture(mg::TextureId id) { mgSystem.textureContainer.removeTexture(id); }
inline Mesh getMesh(mg::MeshId meshId) { return mgSystem.meshContainer.getMesh(meshId); }
inline StorageData getStorage(mg::StorageId storageId) { return mgSystem.storageContainer.getStorage(storageId); }
inline void addImguiLog(const char *str) { mgSystem.imguiOverlay.addLog(str); }

} // namespace mg