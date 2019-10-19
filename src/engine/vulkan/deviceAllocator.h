#pragma once
#include "vkContext.h"
#include "mg/mgUtils.h"
#include "vulkan/vkUtils.h"
#include <array>

namespace mg {

struct DeviceHeapAllocation {
  VkDeviceMemory deviceMemory;
  VkDeviceSize size;
  VkDeviceSize offset;
  uint32_t memoryTypeIndex;
  int32_t largeSizeAllocationIndex = -1;
  uint64_t largeSizeGenerationIndex;
};

struct _Node {
  _Node *next;
  VkDeviceSize offset;
  VkDeviceSize size;
};

struct AllocationInfo {
  VkDeviceSize totalSize;
  VkDeviceSize currentSize;
  int32_t totalNrOfAllocations;
  int32_t allocationNotFreed;
};

struct CreateDeviceHeapAllocatorInfo;

class  DeviceMemoryAllocator : mg::nonCopyable {
public:
  void create(const CreateDeviceHeapAllocatorInfo &createDeviceHeapAllocationInfo);
  void destroy();
  DeviceHeapAllocation allocateDeviceOnlyMemory(uint32_t memoryTypeIndex, VkDeviceSize sizeInBytes, VkDeviceSize alignment);
  void freeDeviceOnlyMemory(const DeviceHeapAllocation &allocation);

  std::vector<GuiAllocation> getAllocationForGUI();
  ~DeviceMemoryAllocator();

  enum { NR_OF_HEAPS = 4 };

private:
  DeviceHeapAllocation allocateLargeDeviceOnlyMemory(uint32_t memoryTypeIndex, VkDeviceSize sizeInBytes, VkDeviceSize alignment);
  void freeLargeDeviceOnlyMemory(const DeviceHeapAllocation &allocation);

  VkDeviceMemory _deviceMemories[VK_MAX_MEMORY_TYPES][NR_OF_HEAPS];
  _Node _memoryPools[VK_MAX_MEMORY_TYPES][NR_OF_HEAPS];
  AllocationInfo _allocationInfos[VK_MAX_MEMORY_TYPES][NR_OF_HEAPS];
  uint32_t _heapSizes[NR_OF_HEAPS];
  uint32_t _maxHeapSize;
  uint32_t _nrOfHeapTypes;
  bool _useDifferentHeapsForSmallAllocations;
  uint32_t _smallSizeAllocationThreshold;
  bool _hasBeenDelete = true;
  uint64_t _largSizeGenerationIndex = 0;

  // allocations larger than the biggest heap
  struct LargeAllocation {
    VkDeviceMemory deviceMemory;
    VkDeviceSize size;
    uint32_t memoryIndex;
    uint64_t generationIndex;
  };
  std::vector<LargeAllocation> _largeSizeAllocations;
};

struct CreateDeviceHeapAllocatorInfo {
  bool useDifferentHeapsForSmallAllocations;
  uint32_t smallSizeAllocationThreshold;
  std::array<uint32_t, DeviceMemoryAllocator::NR_OF_HEAPS> heapSizes;
};

} // namespace