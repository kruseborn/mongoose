#pragma once
#include "mg/mgUtils.h"
#include "vulkan/deviceAllocator.h"
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
  VkImage image;
  VkImageView imageView;
};

struct _StorageData {
  StorageData storage;
  mg::DeviceHeapAllocation heapAllocation;
};

struct CreateImageStorageInfo {
  std::string id;
  VkFormat format;
  VkExtent3D size;
};

class StorageContainer : mg::nonCopyable {
public:
  void createStorageContainer() {}
  StorageId createEmptyStorage(uint32_t sizeInBytes);
  StorageId createStorage(void *data, uint32_t sizeInBytes);
  StorageId createImageStorage(const CreateImageStorageInfo &info);

  StorageData getStorage(StorageId storageId) const;

  void removeStorage(StorageId storageId);

  void destroyStorageContainer();
  ~StorageContainer();

private:
  StorageId _createStorage(void *data, uint32_t sizeInBytes, VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags properties);

  std::vector<_StorageData> _idToStorage;
  std::vector<uint32_t> _freeIndices;
  std::vector<uint32_t> _generations;
};

} // namespace mg
