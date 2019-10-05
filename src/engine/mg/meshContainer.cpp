#include "meshContainer.h"
#include "mg/mgSystem.h"
#include "vulkan/deviceAllocator.h"
#include "vulkan/linearHeapAllocator.h"
#include "vulkan/vkUtils.h"

namespace mg {

static void stageData(mg::MeshData *meshData, void *data, uint32_t sizeInBytes) {
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
  vkCmdCopyBuffer(copyCommandBuffer, stagingBuffer, meshData->mesh.buffer, 1, &region);

  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT;

  vkCmdPipelineBarrier(copyCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0,
                       &memoryBarrier, 0, nullptr, 0, nullptr);
}

static void uploadMeshWithoutIndices(const mg::CreateMeshInfo &createMeshInfo, mg::MeshData *meshData) {
  VkBufferCreateInfo vertexBufferInfo = {};
  vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vertexBufferInfo.size = createMeshInfo.verticesSizeInBytes;
  vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  checkResult(vkCreateBuffer(mg::vkContext.device, &vertexBufferInfo, nullptr, &meshData->mesh.buffer));

  VkMemoryRequirements vkMemoryRequirements = {};
  vkGetBufferMemoryRequirements(mg::vkContext.device, meshData->mesh.buffer, &vkMemoryRequirements);

  const auto memoryIndex = findMemoryTypeIndex(mg::vkContext.physicalDeviceMemoryProperties, vkMemoryRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  meshData->heapAllocation = mg::mgSystem.meshDeviceMemoryAllocator.allocateDeviceOnlyMemory(
      memoryIndex, createMeshInfo.verticesSizeInBytes, vkMemoryRequirements.alignment);
  checkResult(vkBindBufferMemory(mg::vkContext.device, meshData->mesh.buffer, meshData->heapAllocation.deviceMemory,
                                 meshData->heapAllocation.offset));

  stageData(meshData, createMeshInfo.vertices, createMeshInfo.verticesSizeInBytes);
}

static void uploadMeshWithIndices(const mg::CreateMeshInfo &createMeshInfo, mg::MeshData *meshData) {
  const uint32_t totalSize = createMeshInfo.verticesSizeInBytes + createMeshInfo.indicesSizeInBytes;
  void *data = malloc(totalSize);
  memcpy(data, createMeshInfo.vertices, createMeshInfo.verticesSizeInBytes);
  memcpy((char *)data + createMeshInfo.verticesSizeInBytes, createMeshInfo.indices, createMeshInfo.indicesSizeInBytes);

  VkBufferCreateInfo vertexBufferInfo = {};
  vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vertexBufferInfo.usage =
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  vertexBufferInfo.size = totalSize;

  checkResult(vkCreateBuffer(mg::vkContext.device, &vertexBufferInfo, nullptr, &meshData->mesh.buffer));

  VkMemoryRequirements vkMemoryRequirements = {};
  vkGetBufferMemoryRequirements(mg::vkContext.device, meshData->mesh.buffer, &vkMemoryRequirements);

  const auto memoryIndex = findMemoryTypeIndex(mg::vkContext.physicalDeviceMemoryProperties, vkMemoryRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  meshData->heapAllocation = mg::mgSystem.meshDeviceMemoryAllocator.allocateDeviceOnlyMemory(
      memoryIndex, vkMemoryRequirements.size, vkMemoryRequirements.alignment);
  checkResult(vkBindBufferMemory(mg::vkContext.device, meshData->mesh.buffer, meshData->heapAllocation.deviceMemory,
                                 meshData->heapAllocation.offset));

  stageData(meshData, data, totalSize);
  free(data);
}

MeshContainer::~MeshContainer() { mgAssert(_idToMesh.size() == 0); }

void MeshContainer::destroyMeshContainer() {
  for (auto &meshData : _idToMesh) {
    if (meshData.mesh.indexCount == 0)
      continue;

    vkDestroyBuffer(mg::vkContext.device, meshData.mesh.buffer, nullptr);
    mg::mgSystem.meshDeviceMemoryAllocator.freeDeviceOnlyMemory(meshData.heapAllocation);
  }
  _idToMesh.clear();
  _freeIndices.clear();
  _generations.clear();
}

MeshId MeshContainer::createMesh(const CreateMeshInfo &createMeshInfo) {
  mgAssert(createMeshInfo.verticesSizeInBytes > 0);
  mgAssert(createMeshInfo.vertices != nullptr);

  uint32_t currentIndex = 0;
  if (_freeIndices.size()) {
    currentIndex = _freeIndices.back();
    _freeIndices.pop_back();
  } else {
    currentIndex = uint32_t(_idToMesh.size());
    _idToMesh.push_back({});
    _generations.push_back(0);
  }
  mg::MeshData meshData = {};
  meshData.mesh.indexCount = createMeshInfo.nrOfIndices;
  meshData.mesh.indicesOffset = createMeshInfo.verticesSizeInBytes;

  if (createMeshInfo.indices == nullptr)
    uploadMeshWithoutIndices(createMeshInfo, &meshData);
  else {
    uploadMeshWithIndices(createMeshInfo, &meshData);
  }
  _idToMesh[currentIndex] = meshData;

  MeshId meshId = {};
  meshId.generation = _generations[currentIndex];
  meshId.index = currentIndex;
  return meshId;
}

Mesh MeshContainer::getMesh(MeshId meshId) const {
  mgAssert(meshId.index < _idToMesh.size());
  mgAssert(meshId.generation == _generations[meshId.index]);

  return _idToMesh[meshId.index].mesh;
}

void MeshContainer::removeMesh(MeshId meshId) {
  mgAssert(meshId.index < _idToMesh.size());
  mgAssert(meshId.generation == _generations[meshId.index]);

  mg::mgSystem.meshDeviceMemoryAllocator.freeDeviceOnlyMemory(_idToMesh[meshId.index].heapAllocation);
  vkDestroyBuffer(mg::vkContext.device, _idToMesh[meshId.index].mesh.buffer, nullptr);
  _idToMesh[meshId.index] = {};
  _generations[meshId.index]++;
  _freeIndices.push_back(meshId.index);
}

} // namespace mg
