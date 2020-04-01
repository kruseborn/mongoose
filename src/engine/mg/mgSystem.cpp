#include "mgSystem.h"
#include "vulkan/linearHeapAllocator.h"
#include "vulkan/deviceAllocator.h"
#include "vulkan/pipelineContainer.h"
#include "vulkan/vkUtils.h"
#include <glm/glm.hpp>

namespace mg {

MgSystem mgSystem = {};

static void createAllocators(MgSystem *system) {
  {
    constexpr uint32_t mgTobytes = 1024 * 1024;
    constexpr uint32_t heapSize = 16 * mgTobytes;
    CreateDeviceHeapAllocatorInfo vertexAllocationInfo = {};
    vertexAllocationInfo.heapSizes = { heapSize, heapSize, heapSize, heapSize };
    vertexAllocationInfo.useDifferentHeapsForSmallAllocations = false;
    system->meshDeviceMemoryAllocator.create(vertexAllocationInfo);
  }
  {
    constexpr uint32_t mgTobytes = 1024 * 1024;
    constexpr uint32_t heapSize = 1 * mgTobytes;
    CreateDeviceHeapAllocatorInfo textureAllocationInfo = {};
    textureAllocationInfo.heapSizes = { heapSize, heapSize, heapSize, heapSize };
    textureAllocationInfo.useDifferentHeapsForSmallAllocations = false;
    system->textureDeviceMemoryAllocator.create(textureAllocationInfo);
  }
  system->linearHeapAllocator.create();
}

static void destroyAllocators(MgSystem *system) {
  LOG("destroyAllocators");
  system->textureDeviceMemoryAllocator.destroy();
  system->meshDeviceMemoryAllocator.destroy();
  system->linearHeapAllocator.destroy();
}

static void createContainers(MgSystem *system) {
  system->textureContainer.createTextureContainer();
  system->pipelineContainer.createPipelineContainer();
  system->meshContainer.createMeshContainer();
  system->storageContainer.createStorageContainer();
}

static void destroyContainers(MgSystem *system) {
  system->textureContainer.destroyTextureContainer();
  system->pipelineContainer.destroyPipelineContainer();
  system->meshContainer.destroyMeshContainer();
  system->storageContainer.destroyStorageContainer();
}

void createMgSystem(MgSystem *system) {
  createAllocators(system);
  createContainers(system);

  mgSystem.imguiOverlay.CreateContext();
  system->fonts.init();
}

void destroyMgSystem(MgSystem *system) {
  waitForDeviceIdle();
  system->fonts.destroy();
  mgSystem.imguiOverlay.destroy();
  destroyContainers(system);
  destroyAllocators(system);
}

} // namespace
