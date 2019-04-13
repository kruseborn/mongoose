#pragma once
#include <string>

#include "mg/meshContainer.h"
#include "mg/mgUtils.h"
#include "mg/textureContainer.h"
#include "vulkan/imguiOverlay.h"
#include "vulkan/linearHeapAllocator.h"
#include "vulkan/pipelineContainer.h"
#include "vulkan/singleRenderpass.h"
#include "mg/fonts.h"

namespace mg {

struct MgSystem : mg::nonCopyable {
  PipelineContainer pipelineContainer;
  Pipelines pipelines;

  TextureContainer textureContainer;
  MeshContainer meshContainer;

  LinearHeapAllocator linearHeapAllocator;
  DeviceMemoryAllocator meshDeviceMemoryAllocator;
  DeviceMemoryAllocator textureDeviceMemoryAllocator;

  Fonts fonts;
  Imgui imguiOverlay;
};

void createMgSystem(MgSystem *system);
void destroyMgSystem(MgSystem *system);

extern MgSystem mgSystem;

// helper function
inline Texture getTexture(const std::string &id, mg::TEXTURE_SAMPLER sampler = mg::TEXTURE_SAMPLER::NONE) {
  return mgSystem.textureContainer.getTexture(id, sampler);
}

inline void removeTexture(const std::string &id) { mgSystem.textureContainer.removeTexture(id); }
inline Mesh getMesh(mg::MeshId meshId) { return mgSystem.meshContainer.getMesh(meshId); }
inline void addImguiLog(const char *str) { mgSystem.imguiOverlay.addLog(str); }

} // namespace mg