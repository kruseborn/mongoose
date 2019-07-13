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
  StorageId createEmptyStorage(uint32_t sizeInBytes);
  StorageId createStorage(void *data, uint32_t sizeInBytes);
  StorageData getStorage(StorageId storageId) const;
  void* mapDeviceMemory(StorageId storageId);
  void unmapDeviceMemory(StorageId storageId);
  void removeStorage(StorageId storageId);

  void destroyStorageContainer();
  ~StorageContainer();

private:
  StorageId StorageContainer::_createStorage(void *data, uint32_t sizeInBytes, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);


  std::vector<_StorageData> _idToStorage;
  std::vector<uint32_t> _freeIndices;
  std::vector<uint32_t> _generations;
};

} // namespace mg
