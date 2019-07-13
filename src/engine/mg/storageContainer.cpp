#include "storageContainer.h"
#include "mg/mgSystem.h"
#include "vulkan/deviceAllocator.h"
#include "vulkan/linearHeapAllocator.h"
#include "vulkan/vkUtils.h"

namespace mg {

static void stageData(mg::_StorageData *storageData, void *data, uint32_t sizeInBytes) {
  VkCommandBuffer copyCommandBuffer;
  VkBuffer stagingBuffer;
  VkDeviceSize stagingOffset;
  void *stagingMemory =
      mg::mgSystem.linearHeapAllocator.allocateStaging(sizeInBytes, 1, &copyCommandBuffer, &stagingBuffer, &stagingOffset);
  memcpy(stagingMemory, data, sizeInBytes);

  VkBufferCopy region = {};
  region.srcOffset = stagingOffset;
  region.dstOffset = 0;
  region.size = sizeInBytes;
  vkCmdCopyBuffer(copyCommandBuffer, stagingBuffer, storageData->storage.buffer, 1, &region);

  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

  vkCmdPipelineBarrier(copyCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                       &memoryBarrier, 0, nullptr, 0, nullptr);
}

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

StorageId StorageContainer::_createStorage(void *data, uint32_t sizeInBytes, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
  uint32_t currentIndex = 0;
  if (_freeIndices.size()) {
    currentIndex = _freeIndices.back();
    _freeIndices.pop_back();
  } else {
    currentIndex = _idToStorage.size();
    _idToStorage.push_back({});
    _generations.push_back(0);
  }

  mg::_StorageData _storageData = {};
  _storageData.storage.size = sizeInBytes;

  VkBufferCreateInfo vertexBufferInfo = {};
  vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vertexBufferInfo.size = sizeInBytes;
  vertexBufferInfo.usage = usage;
  vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffer is exclusive to a single queue family at a time.

  checkResult(vkCreateBuffer(mg::vkContext.device, &vertexBufferInfo, nullptr, &_storageData.storage.buffer));

  VkMemoryRequirements vkMemoryRequirements = {};
  vkGetBufferMemoryRequirements(mg::vkContext.device, _storageData.storage.buffer, &vkMemoryRequirements);

  const auto memoryIndex = findMemoryTypeIndex(mg::vkContext.physicalDeviceMemoryProperties, vkMemoryRequirements.memoryTypeBits, properties);
  _storageData.heapAllocation = mg::mgSystem.storageDeviceMemoryAllocator.allocateDeviceOnlyMemory(memoryIndex, sizeInBytes,
                                                                                                  vkMemoryRequirements.alignment);
  checkResult(vkBindBufferMemory(mg::vkContext.device, _storageData.storage.buffer, _storageData.heapAllocation.deviceMemory,
                                 _storageData.heapAllocation.offset));

  if (properties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
    mgAssert(data != nullptr);
    stageData(&_storageData, data, sizeInBytes);
  } else {
    mgAssert(data == nullptr);
  }

  VkDescriptorSetAllocateInfo vkDescriptorSetAllocateInfo = {};
  vkDescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  vkDescriptorSetAllocateInfo.descriptorPool = mg::vkContext.descriptorPool;
  vkDescriptorSetAllocateInfo.descriptorSetCount = 1;
  vkDescriptorSetAllocateInfo.pSetLayouts = &mg::vkContext.descriptorSetLayout.storage;
  vkAllocateDescriptorSets(mg::vkContext.device, &vkDescriptorSetAllocateInfo, &_storageData.storage.descriptorSet);

  // Specify the buffer to bind to the descriptor.
  VkDescriptorBufferInfo descriptorBufferInfo = {};
  descriptorBufferInfo.buffer = _storageData.storage.buffer;
  descriptorBufferInfo.range = sizeInBytes;

  VkWriteDescriptorSet writeDescriptorSet = {};
  writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet.dstSet = _storageData.storage.descriptorSet;
  writeDescriptorSet.dstBinding = 0;
  writeDescriptorSet.descriptorCount = 1;
  writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

  // perform the update of the descriptor set.
  vkUpdateDescriptorSets(vkContext.device, 1, &writeDescriptorSet, 0, nullptr);
  _idToStorage[currentIndex] = _storageData;

  StorageId storageId = {};
  storageId.generation = _generations[currentIndex];
  storageId.index = currentIndex;

  return storageId;
}

StorageId StorageContainer::createStorage(void *data, uint32_t sizeInBytes) {
  auto storageId = _createStorage(
      data,
      sizeInBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  return storageId;
}

StorageId StorageContainer::createEmptyStorage(uint32_t sizeInBytes) {
  auto storageId = _createStorage(nullptr, sizeInBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  return storageId;
}

void *StorageContainer::mapDeviceMemory(StorageId storageId) {
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
