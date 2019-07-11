  #pragma once
#include "vulkan/deviceAllocator.h"
#include "mg/mgUtils.h"
#include "vulkan/vkContext.h"
#include <string>

namespace mg {

struct StorageId {
  uint32_t generation;
  uint32_t index;
};

struct StorageData {
  VkBuffer buffer;
  VkDescriptorSet descriptorSet;
  uint32_t size;
};

struct _StorageData {
  StorageData storage;
  mg::DeviceHeapAllocation heapAllocation;
};

class StorageContainer : mg::nonCopyable {
public:
  void createStorageContainer() {}
  StorageId createStorage(uint32_t sizeInBytes);
  StorageData getStorage(StorageId storageId) const;
  void* mapDeviceMemory(StorageId storageId);
  void unmapDeviceMemory(StorageId storageId);
  void removeStorage(StorageId storageId);

  void destroyStorageContainer();
  ~StorageContainer();

private:
  std::vector<_StorageData> _idToStorage;
  std::vector<uint32_t> _freeIndices;
  std::vector<uint32_t> _generations;
};

} // namespace mg
