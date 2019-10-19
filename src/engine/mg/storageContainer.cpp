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
  void *stagingMemory = mg::mgSystem.linearHeapAllocator.allocateStaging(sizeInBytes, 1, &copyCommandBuffer,
                                                                         &stagingBuffer, &stagingOffset);
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
  mgAssert(_idToStorage.size() == _freeIndices.size());
  _idToStorage.clear();
  _freeIndices.clear();
  _generations.clear();
}

StorageId StorageContainer::_createStorage(void *data, uint32_t sizeInBytes, VkBufferUsageFlags usage,
                                           VkMemoryPropertyFlags properties) {
  uint32_t currentIndex = 0;
  if (_freeIndices.size()) {
    currentIndex = _freeIndices.back();
    _freeIndices.pop_back();
  } else {
    currentIndex = uint32_t(_idToStorage.size());
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

  const auto memoryIndex = findMemoryTypeIndex(mg::vkContext.physicalDeviceMemoryProperties,
                                               vkMemoryRequirements.memoryTypeBits, properties);
  _storageData.heapAllocation = mg::mgSystem.textureDeviceMemoryAllocator.allocateDeviceOnlyMemory(
      memoryIndex, sizeInBytes, vkMemoryRequirements.alignment);

  checkResult(vkBindBufferMemory(mg::vkContext.device, _storageData.storage.buffer,
                                 _storageData.heapAllocation.deviceMemory, _storageData.heapAllocation.offset));

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
  auto storageId = _createStorage(data, sizeInBytes,
                                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  return storageId;
}

StorageId StorageContainer::createEmptyStorage(uint32_t sizeInBytes) {
  auto storageId = _createStorage(nullptr, sizeInBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  return storageId;
}

StorageId StorageContainer::createImageStorage(const CreateImageStorageInfo &info) {
  uint32_t currentIndex = 0;
  if (_freeIndices.size()) {
    currentIndex = _freeIndices.back();
    _freeIndices.pop_back();
  } else {
    currentIndex = uint32_t(_idToStorage.size());
    _idToStorage.push_back({});
    _generations.push_back(0);
  }

  mg::_StorageData _storageData = {};

  VkImageCreateInfo imageCreateInfo = {};
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = info.format;
  imageCreateInfo.extent.width = info.size.width;
  imageCreateInfo.extent.height = info.size.height;
  imageCreateInfo.extent.depth = 1;
  imageCreateInfo.mipLevels = 1;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  mg::checkResult(vkCreateImage(vkContext.device, &imageCreateInfo, nullptr, &_storageData.storage.image));

  VkMemoryRequirements vkMemoryRequirements;
  vkGetImageMemoryRequirements(mg::vkContext.device, _storageData.storage.image, &vkMemoryRequirements);
  const auto memoryIndex =
      findMemoryTypeIndex(mg::vkContext.physicalDeviceMemoryProperties, vkMemoryRequirements.memoryTypeBits,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  _storageData.heapAllocation = mg::mgSystem.textureDeviceMemoryAllocator.allocateDeviceOnlyMemory(
      memoryIndex, vkMemoryRequirements.size, vkMemoryRequirements.alignment);
  checkResult(vkBindImageMemory(mg::vkContext.device, _storageData.storage.image,
                                _storageData.heapAllocation.deviceMemory, _storageData.heapAllocation.offset));

  // https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
  // Pipeline barrier before the copy to perform a layout transition
  VkImageMemoryBarrier preCopyMemoryBarrier = {};
  preCopyMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  preCopyMemoryBarrier.srcAccessMask = 0;
  preCopyMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  preCopyMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  preCopyMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
  preCopyMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  preCopyMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  preCopyMemoryBarrier.image = _storageData.storage.image;
  preCopyMemoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
  commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  commandBufferAllocateInfo.commandPool = mg::vkContext.commandPool;
  commandBufferAllocateInfo.commandBufferCount = 1;
  commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  VkCommandBuffer commandBuffer;
  mg::checkResult(vkAllocateCommandBuffers(mg::vkContext.device, &commandBufferAllocateInfo, &commandBuffer));

  VkCommandBufferBeginInfo commandBufferBeginInfo = {};
  commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  mg::checkResult(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                       nullptr, 0, nullptr, 1, &preCopyMemoryBarrier);
  mg::checkResult(vkEndCommandBuffer(commandBuffer));

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  VkFence fence;
  mg::checkResult(vkCreateFence(mg::vkContext.device, &fenceInfo, nullptr, &fence));

  mg::checkResult(vkQueueSubmit(mg::vkContext.queue, 1, &submitInfo, fence));
  mg::checkResult(vkWaitForFences(mg::vkContext.device, 1, &fence, VK_TRUE, UINT64_MAX));
  vkDestroyFence(mg::vkContext.device, fence, nullptr);
  vkFreeCommandBuffers(mg::vkContext.device, mg::vkContext.commandPool, 1, &commandBuffer);

  VkImageViewCreateInfo vkImageViewCreateInfo = {};
  vkImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  vkImageViewCreateInfo.image = _storageData.storage.image;
  vkImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  vkImageViewCreateInfo.format = info.format;
  vkImageViewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  checkResult(
      vkCreateImageView(mg::vkContext.device, &vkImageViewCreateInfo, nullptr, &_storageData.storage.imageView));

  VkDescriptorSetAllocateInfo vkDescriptorSetAllocateInfo = {};
  vkDescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  vkDescriptorSetAllocateInfo.descriptorPool = mg::vkContext.descriptorPool;
  vkDescriptorSetAllocateInfo.descriptorSetCount = 1;
  vkDescriptorSetAllocateInfo.pSetLayouts = &mg::vkContext.descriptorSetLayout.storageImage;
  vkAllocateDescriptorSets(mg::vkContext.device, &vkDescriptorSetAllocateInfo, &_storageData.storage.descriptorSet);

  // Specify the buffer to bind to the descriptor.
  VkDescriptorImageInfo descriptorImageInfo = {};
  descriptorImageInfo.imageView = _storageData.storage.imageView;
  descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  VkWriteDescriptorSet writeDescriptorSet = {};
  writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet.dstSet = _storageData.storage.descriptorSet;
  writeDescriptorSet.dstBinding = 0;
  writeDescriptorSet.descriptorCount = 1;
  writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  writeDescriptorSet.pImageInfo = &descriptorImageInfo;

  // perform the update of the descriptor set.
  vkUpdateDescriptorSets(vkContext.device, 1, &writeDescriptorSet, 0, nullptr);

  _idToStorage[currentIndex] = _storageData;

  StorageId storageId = {};
  storageId.generation = _generations[currentIndex];
  storageId.index = currentIndex;

  return storageId;
}

StorageData StorageContainer::getStorage(StorageId storageId) const {
  mgAssert(storageId.index < _idToStorage.size());
  mgAssert(storageId.generation == _generations[storageId.index]);

  return _idToStorage[storageId.index].storage;
}

void StorageContainer::removeStorage(StorageId storageId) {
  mgAssert(storageId.index < _idToStorage.size());
  mgAssert(storageId.generation == _generations[storageId.index]);

  mg::mgSystem.textureDeviceMemoryAllocator.freeDeviceOnlyMemory(_idToStorage[storageId.index].heapAllocation);
  vkDestroyBuffer(mg::vkContext.device, _idToStorage[storageId.index].storage.buffer, nullptr);
  vkDestroyImageView(mg::vkContext.device, _idToStorage[storageId.index].storage.imageView, nullptr);
  vkDestroyImage(mg::vkContext.device, _idToStorage[storageId.index].storage.image, nullptr);
  _idToStorage[storageId.index] = {};
  _generations[storageId.index]++;
  _freeIndices.push_back(storageId.index);
}

} // namespace mg
