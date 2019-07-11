#include "storageContainer.h"
#include "mg/mgSystem.h"
#include "vulkan/deviceAllocator.h"
#include "vulkan/linearHeapAllocator.h"
#include "vulkan/vkUtils.h"

namespace mg {

StorageContainer::~StorageContainer() { mgAssert(_idToStorage.size() == 0); }

void StorageContainer::destroyStorageContainer() {
  for (auto &storageData : _idToStorage) {
    vkDestroyBuffer(mg::vkContext.device, storageData.storage.buffer, nullptr);
    mg::mgSystem.storageDeviceMemoryAllocator.freeDeviceOnlyMemory(storageData.heapAllocation);
  }
  _idToStorage.clear();
  _freeIndices.clear();
  _generations.clear();
}

StorageId StorageContainer::createStorage(uint32_t sizeInBytes) {
  uint32_t currentIndex = 0;
  if (_freeIndices.size()) {
    currentIndex = _freeIndices.back();
    _freeIndices.pop_back();
  } else {
    currentIndex = _idToStorage.size();
    _idToStorage.push_back({});
    _generations.push_back(0);
  }

  mg::_StorageData storageData = {};
  storageData.storage.size = sizeInBytes;

  VkBufferCreateInfo vertexBufferInfo = {};
  vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vertexBufferInfo.size = sizeInBytes;
  vertexBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffer is exclusive to a single queue family at a time.

  checkResult(vkCreateBuffer(mg::vkContext.device, &vertexBufferInfo, nullptr, &storageData.storage.buffer));

  VkMemoryRequirements vkMemoryRequirements = {};
  vkGetBufferMemoryRequirements(mg::vkContext.device, storageData.storage.buffer, &vkMemoryRequirements);

  const auto memoryIndex = findMemoryTypeIndex(mg::vkContext.physicalDeviceMemoryProperties, vkMemoryRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  storageData.heapAllocation = mg::mgSystem.storageDeviceMemoryAllocator.allocateDeviceOnlyMemory(memoryIndex, sizeInBytes,
                                                                                                  vkMemoryRequirements.alignment);
  checkResult(vkBindBufferMemory(mg::vkContext.device, storageData.storage.buffer, storageData.heapAllocation.deviceMemory,
                                 storageData.heapAllocation.offset));

  VkDescriptorSetAllocateInfo vkDescriptorSetAllocateInfo = {};
  vkDescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  vkDescriptorSetAllocateInfo.descriptorPool = mg::vkContext.descriptorPool;
  vkDescriptorSetAllocateInfo.descriptorSetCount = 1;
  vkDescriptorSetAllocateInfo.pSetLayouts = &mg::vkContext.descriptorSetLayout.storage;
  vkAllocateDescriptorSets(mg::vkContext.device, &vkDescriptorSetAllocateInfo, &storageData.storage.descriptorSet);

  // Specify the buffer to bind to the descriptor.
  VkDescriptorBufferInfo descriptorBufferInfo = {};
  descriptorBufferInfo.buffer = storageData.storage.buffer;
  descriptorBufferInfo.range = sizeInBytes;

  VkWriteDescriptorSet writeDescriptorSet = {};
  writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet.dstSet = storageData.storage.descriptorSet;         // write to this descriptor set.
  writeDescriptorSet.dstBinding = 0;                                     // write to the first, and only binding.
  writeDescriptorSet.descriptorCount = 1;                                // update a single descriptor.
  writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; // storage buffer.
  writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

  // perform the update of the descriptor set.
  vkUpdateDescriptorSets(vkContext.device, 1, &writeDescriptorSet, 0, nullptr);
  _idToStorage[currentIndex] = storageData;

  StorageId storageId = {};
  storageId.generation = _generations[currentIndex];
  storageId.index = currentIndex;

  return storageId;
}

void* StorageContainer::mapDeviceMemory(StorageId storageId) { 
  mgAssert(storageId.index < _idToStorage.size());
  mgAssert(storageId.generation == _generations[storageId.index]);

  const auto &_storage = _idToStorage[storageId.index];

  void *mappedMemory = nullptr;
  vkMapMemory(mg::vkContext.device, _storage.heapAllocation.deviceMemory, _storage.heapAllocation.offset, _storage.storage.size,
              0, &mappedMemory);
  return mappedMemory;

}
void StorageContainer::unmapDeviceMemory(StorageId storageId) {
  mgAssert(storageId.index < _idToStorage.size());
  mgAssert(storageId.generation == _generations[storageId.index]);

  const auto &_storage = _idToStorage[storageId.index];
  vkUnmapMemory(mg::vkContext.device, _storage.heapAllocation.deviceMemory);
}

StorageData StorageContainer::getStorage(StorageId storageId) const {
  mgAssert(storageId.index < _idToStorage.size());
  mgAssert(storageId.generation == _generations[storageId.index]);

  return _idToStorage[storageId.index].storage;
}

void StorageContainer::removeStorage(StorageId storageId) {
  mgAssert(storageId.index < _idToStorage.size());
  mgAssert(storageId.generation == _generations[storageId.index]);

  mg::mgSystem.storageDeviceMemoryAllocator.freeDeviceOnlyMemory(_idToStorage[storageId.index].heapAllocation);
  vkDestroyBuffer(mg::vkContext.device, _idToStorage[storageId.index].storage.buffer, nullptr);
  _idToStorage = {};
  _generations[storageId.index]++;
  _freeIndices.push_back(storageId.index);
}

} // namespace mg
