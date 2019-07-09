#pragma once
#include "vkContext.h"
#include "mg/mgUtils.h"
#include "vulkan/vkUtils.h"

namespace mg {

enum { NrOfBuffers = 2 };

struct _Buffer {
  VkDeviceMemory deviceMemory;
  VkDeviceSize alignedSize;
  uint32_t memoryTypeIndex;

  struct {
    VkBuffer vkBuffer;
    VkDeviceSize offset;
    char *data;
  } bufferViews[NrOfBuffers];
};

struct _UniformBuffer {
  _Buffer buffer;
  VkDescriptorSet vkDescriptorSet[NrOfBuffers];
};

struct _StorageBuffer {
  _Buffer buffer;
  VkDescriptorSet vkDescriptorSet[NrOfBuffers];
};

struct _StagingBuffer {
  _Buffer buffer;
  VkCommandBuffer vkCommandBuffers[NrOfBuffers];
  VkFence vkFences[NrOfBuffers];
  bool submitted[NrOfBuffers];
};

struct LinearHeapAllocator : mg::nonCopyable {
public:
  void create();
  void destroy();

  void* allocateBuffer(VkDeviceSize sizeInBytes, VkBuffer *buffer, VkDeviceSize *offset);
  void* allocateUniform(VkDeviceSize sizeInBytes, VkBuffer *buffer, uint32_t *offset, VkDescriptorSet *vkDescriptorSet);
  void *allocateStorage(VkDeviceSize sizeInBytes, VkBuffer *buffer, uint32_t *offset, VkDescriptorSet *vkDescriptorSet);
  void* allocateStaging(VkDeviceSize sizeInBytes, VkDeviceSize alignmentOffset, VkCommandBuffer *commandBuffer, VkBuffer *buffer, VkDeviceSize *offset);

  void submitStagingMemoryToDeviceLocalMemory();
  void swapLinearHeapBuffers();
  std::vector<GuiAllocation> getAllocationForGUI();
  ~LinearHeapAllocator();

private:
  void* allocateLargeStaging(VkDeviceSize sizeInBytes, VkCommandBuffer *commandBuffer, VkBuffer *buffer, VkDeviceSize *offset);
  void destoryLargeStagingAllocations();

  uint32_t _previousVertexSize, _previousUniformSize, _maxStagingSize;

  _Buffer _vertexBuffer;
  _UniformBuffer _uniformBuffer;
  _StagingBuffer _stagingBuffer;
  _StorageBuffer _storageBuffer;
  uint32_t _currentBufferIndex = 0;
  // allocations larger than the staging buffer
  struct LargeLinearStagingAllocation {
    VkDeviceMemory deviceMemory;
    VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize alignSize;
    VkCommandBuffer commandBuffer;
  };
  std::vector<LargeLinearStagingAllocation> _largeLinearStagingAllocation;
  std::vector<VkDeviceSize> _largeAllocationHistory;
  bool _hasBeenDelete;
};

} // namespace