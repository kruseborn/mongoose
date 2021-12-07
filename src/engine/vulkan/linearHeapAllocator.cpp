#include "linearHeapAllocator.h"
#include <algorithm>

#include "mg/mgAssert.h"
#include "vkContext.h"
#include "vkUtils.h"


// 'If descriptorType is VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER or VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
// the range member of each element of pBufferInfo, or the effective range if range is VK_WHOLE_SIZE, must be less than
// or equal to VkPhysicalDeviceLimits::maxUniformBufferRange'
// https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#VUID-VkWriteDescriptorSet-descriptorType-00332
static constexpr uint32_t uniformBufferSizeInBytes = 1u << 16; // 64 kb
static constexpr uint32_t storageBufferSizeInBytes = 1u << 25; // 2 meg
static constexpr uint32_t vertexBufferSizeInBytes = 1u << 25;  // 32 meg
static constexpr uint32_t stagingBufferSizeInBytes = 1u << 25; // 128 meg
static constexpr uint32_t MAX_ALLOC = 2048;

static constexpr struct {
  VkMemoryPropertyFlags requiredProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  VkBufferUsageFlags vertexBufferUsageFlags =
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
} usageFlags;

namespace mg {

static mg::_Buffer _createLinearBuffer(uint32_t bufferSizeInBytes, VkBufferUsageFlags vkBufferUsageFlags,
                                       VkMemoryPropertyFlags requiredProperties,
                                       VkMemoryPropertyFlags preferredProperties) {
  mg::_Buffer buffer = {};
  VkBufferCreateInfo vkBufferCreateInfo = {};
  vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vkBufferCreateInfo.size = bufferSizeInBytes;
  vkBufferCreateInfo.usage = vkBufferUsageFlags;
  vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffer is exclusive to a single queue family at a time.

  for (uint32_t i = 0; i < NrOfBuffers; i++) {
    checkResult(vkCreateBuffer(mg::vkContext.device, &vkBufferCreateInfo, nullptr, &buffer.bufferViews[i].vkBuffer));
  }

  // validation gives a warning if vkGetBufferMemoryRequirements has not been called on all buffers
  VkMemoryRequirements vkMemoryRequirements = {};
  for (uint32_t i = 0; i < NrOfBuffers; i++)
    vkGetBufferMemoryRequirements(mg::vkContext.device, buffer.bufferViews[i].vkBuffer, &vkMemoryRequirements);

  // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT is not cached and does not need be flushed
  const auto memoryTypeIndex =
      findMemoryTypeIndex(mg::vkContext.physicalDeviceMemoryProperties, vkMemoryRequirements.memoryTypeBits,
                          requiredProperties, preferredProperties);
  const auto alignedSize = vkMemoryRequirements.size;
  buffer.alignedSize = alignedSize;

  VkMemoryAllocateInfo vkMemoryAllocateInfo = {};
  vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  vkMemoryAllocateInfo.allocationSize = alignedSize * NrOfBuffers;
  vkMemoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

  checkResult(vkAllocateMemory(mg::vkContext.device, &vkMemoryAllocateInfo, nullptr, &buffer.deviceMemory));

  for (uint32_t i = 0; i < NrOfBuffers; i++)
    checkResult(
        vkBindBufferMemory(mg::vkContext.device, buffer.bufferViews[i].vkBuffer, buffer.deviceMemory, alignedSize * i));

  void *data = nullptr;
  checkResult(vkMapMemory(mg::vkContext.device, buffer.deviceMemory, 0, VK_WHOLE_SIZE, 0, &data));

  for (uint32_t i = 0; i < NrOfBuffers; i++) {
    buffer.bufferViews[i].data = (char *)data + alignedSize * i;
    buffer.bufferViews[i].offset = 0;
  }
  return buffer;
}

static void destroyLinearBuffer(mg::_Buffer *dynamicBuffer) {
  vkUnmapMemory(mg::vkContext.device, dynamicBuffer->deviceMemory);
  for (uint32_t i = 0; i < NrOfBuffers; i++) {
    vkDestroyBuffer(mg::vkContext.device, dynamicBuffer->bufferViews[i].vkBuffer, nullptr);
    dynamicBuffer->bufferViews[i].vkBuffer = VK_NULL_HANDLE;
  }
  vkFreeMemory
  (mg::vkContext.device, dynamicBuffer->deviceMemory, nullptr);
  dynamicBuffer->deviceMemory = VK_NULL_HANDLE;
}

static _UniformBuffer createUniformBuffer(VkDeviceSize bufferSizeInBytes, VkMemoryPropertyFlags requiredProperties,
                                          VkMemoryPropertyFlags preferredProperties,
                                          VkDescriptorSet vkDescriptorSet[NrOfBuffers]) {
  _UniformBuffer dynamicUniformBuffer = {};
  dynamicUniformBuffer.buffer = _createLinearBuffer(uniformBufferSizeInBytes, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                    requiredProperties, preferredProperties);

  for (uint32_t i = 0; i < NrOfBuffers; i++) {
    VkDescriptorBufferInfo vkDescriptorBufferInfo = {};
    vkDescriptorBufferInfo.buffer = dynamicUniformBuffer.buffer.bufferViews[i].vkBuffer;
    vkDescriptorBufferInfo.range = MAX_ALLOC;

    VkWriteDescriptorSet vkWriteDescriptorSet = {};
    vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vkWriteDescriptorSet.dstBinding = 0;
    vkWriteDescriptorSet.descriptorCount = 1;
    vkWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    vkWriteDescriptorSet.pBufferInfo = &vkDescriptorBufferInfo;
    vkWriteDescriptorSet.dstSet = vkDescriptorSet[i];

    vkUpdateDescriptorSets(mg::vkContext.device, 1, &vkWriteDescriptorSet, 0, nullptr);
  }
  return dynamicUniformBuffer;
}

static _StorageBuffer createStorageBuffers(VkDeviceSize bufferSizeInBytes, VkMemoryPropertyFlags requiredProperties,
                                           VkMemoryPropertyFlags preferredProperties,
                                           VkDescriptorSet vkDescriptorSets[NrOfBuffers]) {
  _StorageBuffer dynamicStorageBuffer = {};
  dynamicStorageBuffer.buffer = _createLinearBuffer(
      storageBufferSizeInBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                                                    requiredProperties, preferredProperties);

  for (uint32_t i = 0; i < NrOfBuffers; i++) {
    VkDescriptorBufferInfo vkDescriptorBufferInfo = {};
    vkDescriptorBufferInfo.buffer = dynamicStorageBuffer.buffer.bufferViews[i].vkBuffer;
    vkDescriptorBufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet vkWriteDescriptorSet = {};
    vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vkWriteDescriptorSet.dstSet = vkDescriptorSets[i];
    vkWriteDescriptorSet.dstBinding = 1;
    vkWriteDescriptorSet.descriptorCount = 1;
    vkWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    vkWriteDescriptorSet.pBufferInfo = &vkDescriptorBufferInfo;

    vkUpdateDescriptorSets(mg::vkContext.device, 1, &vkWriteDescriptorSet, 0, nullptr);
  }
  return dynamicStorageBuffer;
}

static void *allocateDynamicBuffer(mg::_Buffer *dynamicBuffer, uint32_t currentBufferIndex, VkDeviceSize sizeInBytes,
                                   VkBuffer *buffer, VkDeviceSize *offset) {
  mgAssertDesc((dynamicBuffer->bufferViews[currentBufferIndex].offset + sizeInBytes) <= dynamicBuffer->alignedSize,
               "Out of dynamic vertex buffer space");
  *buffer = dynamicBuffer->bufferViews[currentBufferIndex].vkBuffer;
  *offset = dynamicBuffer->bufferViews[currentBufferIndex].offset;

  char *data =
      dynamicBuffer->bufferViews[currentBufferIndex].data + dynamicBuffer->bufferViews[currentBufferIndex].offset;
  dynamicBuffer->bufferViews[currentBufferIndex].offset += sizeInBytes;

  return (void *)data;
}

LinearHeapAllocator::~LinearHeapAllocator() { /*mgAssert(_hasBeenDelete == true);*/ }

void *LinearHeapAllocator::allocateBuffer(VkDeviceSize sizeInBytes, VkBuffer *buffer, VkDeviceSize *offset) {
  VkDeviceSize alignedSize = mg::alignUpPowerOfTwo(sizeInBytes, 256); 
  return allocateDynamicBuffer(&_vertexBuffer, _currentBufferIndex, alignedSize, buffer, offset);
}

void *LinearHeapAllocator::allocateUniform(VkDeviceSize sizeInBytes, VkBuffer *buffer, uint32_t *offset,
                                           VkDescriptorSet *vkDescriptorSet) {
  const auto alignment = mg::vkContext.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
  VkDeviceSize alignedSize = mg::alignUpPowerOfTwo(sizeInBytes, alignment);

  VkDeviceSize vkDeviceSizeOffset;
  auto *data =
      allocateDynamicBuffer(&_uniformBuffer.buffer, _currentBufferIndex, alignedSize, buffer, &vkDeviceSizeOffset);
  *offset = static_cast<uint32_t>(vkDeviceSizeOffset);
  *vkDescriptorSet = _vkDescriptorSets[_currentBufferIndex];

  return data;
}

void *LinearHeapAllocator::allocateStorage(VkDeviceSize sizeInBytes, VkBuffer *buffer, uint32_t *offset,
                                           VkDescriptorSet *vkDescriptorSet) {
  const auto alignment = mg::vkContext.physicalDeviceProperties.limits.minStorageBufferOffsetAlignment;
  VkDeviceSize alignedSize = mg::alignUpPowerOfTwo(sizeInBytes, alignment);

  VkDeviceSize vkDeviceSizeOffset;
  auto *data =
      allocateDynamicBuffer(&_storageBuffer.buffer, _currentBufferIndex, alignedSize, buffer, &vkDeviceSizeOffset);
  *offset = static_cast<uint32_t>(vkDeviceSizeOffset);
  *vkDescriptorSet = _vkDescriptorSets[_currentBufferIndex];

  return data;
}

void *LinearHeapAllocator::allocateLargeStaging(VkDeviceSize sizeInBytes, VkCommandBuffer *commandBuffer,
                                                VkBuffer *buffer, VkDeviceSize *offset) {
  _largeLinearStagingAllocation.push_back({});
  auto *allocation = &_largeLinearStagingAllocation.back();

  VkCommandBufferAllocateInfo vkCommandBufferAllocateInfo = {};
  vkCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  vkCommandBufferAllocateInfo.commandPool = mg::vkContext.commandPool;
  vkCommandBufferAllocateInfo.commandBufferCount = 1;
  vkCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  // staging buffer
  checkResult(vkAllocateCommandBuffers(mg::vkContext.device, &vkCommandBufferAllocateInfo, &allocation->commandBuffer));
  VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {};
  vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(allocation->commandBuffer, &vkCommandBufferBeginInfo);

  VkBufferCreateInfo vkBufferCreateInfo = {};
  vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vkBufferCreateInfo.size = sizeInBytes;
  vkBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffer is exclusive to a single queue family at a time.

  checkResult(vkCreateBuffer(mg::vkContext.device, &vkBufferCreateInfo, nullptr, &allocation->buffer));

  // validation gives a warning if vkGetBufferMemoryRequirements has not been called on all buffers
  VkMemoryRequirements vkMemoryRequirements = {};
  vkGetBufferMemoryRequirements(mg::vkContext.device, allocation->buffer, &vkMemoryRequirements);

  // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT is not cached and does not need be flushed
  const auto memoryTypeIndex = findMemoryTypeIndex(mg::vkContext.physicalDeviceMemoryProperties,
                                                   vkMemoryRequirements.memoryTypeBits, usageFlags.requiredProperties);
  const auto alignedSize = vkMemoryRequirements.size;

  VkMemoryAllocateInfo vkMemoryAllocateInfo = {};
  vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  vkMemoryAllocateInfo.allocationSize = alignedSize;
  vkMemoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

  checkResult(vkAllocateMemory(mg::vkContext.device, &vkMemoryAllocateInfo, nullptr, &allocation->deviceMemory));
  checkResult(vkBindBufferMemory(mg::vkContext.device, allocation->buffer, allocation->deviceMemory, 0));

  void *data = nullptr;
  checkResult(vkMapMemory(mg::vkContext.device, allocation->deviceMemory, 0, VK_WHOLE_SIZE, 0, &data));

  *offset = allocation->offset;
  *buffer = allocation->buffer;
  *commandBuffer = allocation->commandBuffer;
  allocation->size = alignedSize;

  return data;
}

void *LinearHeapAllocator::allocateStaging(VkDeviceSize sizeInBytes, VkDeviceSize alignmentOffset,
                                           VkCommandBuffer *commandBuffer, VkBuffer *buffer, VkDeviceSize *offset) {
  auto &stagingBuffer = _stagingBuffer;
  auto &dynamicBuffer = stagingBuffer.buffer;

  // "Total staging memory size is too small for the current allocation";
  if (sizeInBytes > dynamicBuffer.alignedSize) {
    LOG("allocating large staging size: " << sizeInBytes);
    return allocateLargeStaging(sizeInBytes, commandBuffer, buffer, offset);
  }

  // for VkBufferImageCopy, bufferOffset must be a multiple of 4
  const auto alignOffset =
      mg::alignUpPowerOfTwo(dynamicBuffer.bufferViews[_currentBufferIndex].offset, alignmentOffset);
  dynamicBuffer.bufferViews[_currentBufferIndex].offset = alignOffset;

  if ((dynamicBuffer.bufferViews[_currentBufferIndex].offset + sizeInBytes) > dynamicBuffer.alignedSize) {
    submitStagingMemoryToDeviceLocalMemory();
    dynamicBuffer.bufferViews[_currentBufferIndex].offset = 0;
  }

  if (stagingBuffer.submitted[_currentBufferIndex]) {
    stagingBuffer.submitted[_currentBufferIndex] = false;
    checkResult(
        vkWaitForFences(mg::vkContext.device, 1, &stagingBuffer.vkFences[_currentBufferIndex], VK_TRUE, UINT64_MAX));
    vkResetFences(mg::vkContext.device, 1, &stagingBuffer.vkFences[_currentBufferIndex]);

    VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {};
    vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(stagingBuffer.vkCommandBuffers[_currentBufferIndex], &vkCommandBufferBeginInfo);
  }

  *commandBuffer = stagingBuffer.vkCommandBuffers[_currentBufferIndex];
  auto dataBuffer = allocateDynamicBuffer(&dynamicBuffer, _currentBufferIndex, sizeInBytes, buffer, offset);
  return dataBuffer;
}

void LinearHeapAllocator::create() {
  VkDescriptorSetAllocateInfo vkDescriptorSetAllocateInfo = {};
  vkDescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  vkDescriptorSetAllocateInfo.descriptorPool = mg::vkContext.descriptorPool;
  vkDescriptorSetAllocateInfo.descriptorSetCount = NrOfBuffers;
  vkDescriptorSetAllocateInfo.pSetLayouts = &mg::vkContext.descriptorSetLayout.dynamic;
  vkDescriptorSetAllocateInfo.descriptorSetCount = 1;

  for (uint32_t i = 0; i < NrOfBuffers; i++) {
    vkAllocateDescriptorSets(mg::vkContext.device, &vkDescriptorSetAllocateInfo, &_vkDescriptorSets[i]);
  }

  // vertex and uniform buffers
  _vertexBuffer =
      _createLinearBuffer(vertexBufferSizeInBytes, usageFlags.vertexBufferUsageFlags, usageFlags.requiredProperties, 0);
  _uniformBuffer = createUniformBuffer(uniformBufferSizeInBytes, usageFlags.requiredProperties, 0, _vkDescriptorSets);
  _storageBuffer = createStorageBuffers(uniformBufferSizeInBytes, usageFlags.requiredProperties, 0, _vkDescriptorSets);

  VkCommandBufferAllocateInfo vkCommandBufferAllocateInfo = {};
  vkCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  vkCommandBufferAllocateInfo.commandPool = mg::vkContext.commandPool;
  vkCommandBufferAllocateInfo.commandBufferCount = 2;
  vkCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  checkResult(
      vkAllocateCommandBuffers(mg::vkContext.device, &vkCommandBufferAllocateInfo, _stagingBuffer.vkCommandBuffers));

  // staging buffers
  std::fill(std::begin(_stagingBuffer.submitted), std::end(_stagingBuffer.submitted), true);
  _stagingBuffer.buffer =
      _createLinearBuffer(stagingBufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, usageFlags.requiredProperties, 0);

  VkFenceCreateInfo vkFenceCreateInfo = {};
  vkFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  vkFenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  for (uint32_t i = 0; i < NrOfBuffers; i++)
    checkResult(vkCreateFence(mg::vkContext.device, &vkFenceCreateInfo, nullptr, &_stagingBuffer.vkFences[i]));
}

void LinearHeapAllocator::destroy() {
  destroyLinearBuffer(&_vertexBuffer);
  destroyLinearBuffer(&_uniformBuffer.buffer);
  destroyLinearBuffer(&_stagingBuffer.buffer);
  destroyLinearBuffer(&_storageBuffer.buffer);

  for (uint32_t i = 0; i < NrOfBuffers; i++) {
    vkDestroyFence(mg::vkContext.device, _stagingBuffer.vkFences[i], nullptr);
    _stagingBuffer.vkFences[i] = VK_NULL_HANDLE;
  }
  _hasBeenDelete = true;
}

void LinearHeapAllocator::submitStagingMemoryToDeviceLocalMemory() {
  if (_stagingBuffer.buffer.bufferViews[_currentBufferIndex].offset > 0 &&
      _stagingBuffer.submitted[_currentBufferIndex] == false) {
    VkSubmitInfo vkSubmitInfo = {};
    vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    vkSubmitInfo.commandBufferCount = 1;
    vkSubmitInfo.pCommandBuffers = &_stagingBuffer.vkCommandBuffers[_currentBufferIndex];

    checkResult(vkEndCommandBuffer(_stagingBuffer.vkCommandBuffers[_currentBufferIndex]));
    checkResult(vkQueueSubmit(mg::vkContext.queue, 1, &vkSubmitInfo, _stagingBuffer.vkFences[_currentBufferIndex]));

    _stagingBuffer.submitted[_currentBufferIndex] = true;
    _stagingBuffer.buffer.bufferViews[_currentBufferIndex].offset = 0;
  }
  for (const auto &allocation : _largeLinearStagingAllocation) {
    checkResult(vkEndCommandBuffer(allocation.commandBuffer));
    VkSubmitInfo vkSubmitInfo = {};
    vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    vkSubmitInfo.commandBufferCount = 1;
    vkSubmitInfo.pCommandBuffers = &allocation.commandBuffer;
    checkResult(vkQueueSubmit(mg::vkContext.queue, 1, &vkSubmitInfo, VK_NULL_HANDLE));
    checkResult(vkQueueWaitIdle(mg::vkContext.queue));
  }
  destoryLargeStagingAllocations();
}

void LinearHeapAllocator::destoryLargeStagingAllocations() {
  for (const auto &allocation : _largeLinearStagingAllocation) {
    vkFreeCommandBuffers(mg::vkContext.device, mg::vkContext.commandPool, 1, &allocation.commandBuffer);
    vkUnmapMemory(mg::vkContext.device, allocation.deviceMemory);
    vkFreeMemory(mg::vkContext.device, allocation.deviceMemory, nullptr);
    vkDestroyBuffer(mg::vkContext.device, allocation.buffer, nullptr);
  }
  _largeLinearStagingAllocation.clear();
}

void LinearHeapAllocator::swapLinearHeapBuffers() {
  {
    // for ui
    _previousVertexSize = uint32_t(_vertexBuffer.bufferViews[_currentBufferIndex].offset);
    _previousUniformSize = uint32_t(_uniformBuffer.buffer.bufferViews[_currentBufferIndex].offset);
    _maxStagingSize =
        std::max(_maxStagingSize, uint32_t(_stagingBuffer.buffer.bufferViews[_currentBufferIndex].offset));
    for (const auto &allocation : _largeLinearStagingAllocation) {
      _largeAllocationHistory.push_back(allocation.size);
    }
  }
  submitStagingMemoryToDeviceLocalMemory();

  _vertexBuffer.bufferViews[_currentBufferIndex].offset = 0;
  _uniformBuffer.buffer.bufferViews[_currentBufferIndex].offset = 0;
  _stagingBuffer.buffer.bufferViews[_currentBufferIndex].offset = 0;
  _storageBuffer.buffer.bufferViews[_currentBufferIndex].offset = 0;

  _currentBufferIndex = (_currentBufferIndex + 1) % NrOfBuffers;
}

std::vector<GuiAllocation> LinearHeapAllocator::getAllocationForGUI() {
  GuiAllocation vertex = {};
  GuiAllocation dynamic = {};
  GuiAllocation staging = {};
  GuiAllocation largStaging = {};

  {
    vertex.type = "Vertex";
    vertex.showFullSize = true;
    vertex.totalSize = vertexBufferSizeInBytes;
    vertex.memoryTypeIndex = _vertexBuffer.memoryTypeIndex;
    SubAllocationGui subAllocationGui = {};
    subAllocationGui.free = false;
    subAllocationGui.size = _previousVertexSize;
    if (subAllocationGui.size)
      vertex.elements.push_back(subAllocationGui);

    SubAllocationGui subAllocationGuiEmpty = {};
    subAllocationGuiEmpty.offset = subAllocationGui.size;
    subAllocationGuiEmpty.free = true;
    subAllocationGuiEmpty.size = vertexBufferSizeInBytes - _previousVertexSize;
    if (subAllocationGuiEmpty.size)
      vertex.elements.push_back(subAllocationGuiEmpty);
  }
  {
    dynamic.type = "Uniform";
    dynamic.showFullSize = true;
    dynamic.totalSize = uniformBufferSizeInBytes;
    dynamic.memoryTypeIndex = _uniformBuffer.buffer.memoryTypeIndex;
    SubAllocationGui subAllocationGui = {};
    subAllocationGui.free = false;
    subAllocationGui.size = _previousUniformSize;
    if (subAllocationGui.size)
      dynamic.elements.push_back(subAllocationGui);

    SubAllocationGui subAllocationGuiEmpty = {};
    subAllocationGuiEmpty.offset = subAllocationGui.size;
    subAllocationGuiEmpty.free = true;
    subAllocationGuiEmpty.size = uniformBufferSizeInBytes - _previousUniformSize;
    if (subAllocationGuiEmpty.size)
      dynamic.elements.push_back(subAllocationGuiEmpty);
  }
  {
    staging.type = "Max staging size";
    staging.showFullSize = true;
    staging.totalSize = stagingBufferSizeInBytes;
    staging.memoryTypeIndex = _stagingBuffer.buffer.memoryTypeIndex;
    SubAllocationGui subAllocationGui = {};
    subAllocationGui.free = false;
    subAllocationGui.size = _maxStagingSize;
    if (subAllocationGui.size)
      staging.elements.push_back(subAllocationGui);

    SubAllocationGui subAllocationGuiEmpty = {};
    subAllocationGuiEmpty.offset = subAllocationGui.size;
    subAllocationGuiEmpty.free = true;
    subAllocationGuiEmpty.size = stagingBufferSizeInBytes - _maxStagingSize;
    if (subAllocationGuiEmpty.size)
      staging.elements.push_back(subAllocationGuiEmpty);
  }

  largStaging.largeStagingAllocation = true;
  for (const auto &size : _largeAllocationHistory) {
    SubAllocationGui subAllocationGui = {};
    subAllocationGui.size = uint32_t(size);
    largStaging.elements.push_back(subAllocationGui);
  }
  std::vector<GuiAllocation> res;
  res.push_back(vertex);
  res.push_back(dynamic);
  res.push_back(staging);
  return res;
}

} // namespace mg
