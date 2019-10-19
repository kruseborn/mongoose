#include "deviceAllocator.h"

#include "mg/mgUtils.h"
#include "vkContext.h"
#include "vkUtils.h"

using namespace std;

namespace mg {

DeviceMemoryAllocator::~DeviceMemoryAllocator() { mgAssert(_hasBeenDelete == true); }

void DeviceMemoryAllocator::create(const CreateDeviceHeapAllocatorInfo &createDeviceHeapAllocationInfo) {
  _smallSizeAllocationThreshold = createDeviceHeapAllocationInfo.smallSizeAllocationThreshold;
  _nrOfHeapTypes = mg::vkContext.physicalDeviceMemoryProperties.memoryTypeCount;
  mgAssert(createDeviceHeapAllocationInfo.heapSizes.size() <= NR_OF_HEAPS);
  for (uint32_t i = 0; i < uint32_t(createDeviceHeapAllocationInfo.heapSizes.size()); i++) {
    _heapSizes[i] = createDeviceHeapAllocationInfo.heapSizes[i];
    _maxHeapSize = std::max(_maxHeapSize, _heapSizes[i]);
  }
  _useDifferentHeapsForSmallAllocations = createDeviceHeapAllocationInfo.useDifferentHeapsForSmallAllocations;
}

void DeviceMemoryAllocator::destroy() {
  for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < _nrOfHeapTypes; memoryTypeIndex++) {
    for (uint32_t heapIndex = 0; heapIndex < NR_OF_HEAPS; heapIndex++) {
      mgAssert(_allocationInfos[memoryTypeIndex][heapIndex].allocationNotFreed == 0);
      mgAssert(_allocationInfos[memoryTypeIndex][heapIndex].currentSize == 0);
      _Node baseNode = _memoryPools[memoryTypeIndex][heapIndex];
      if (baseNode.next != nullptr) {
        mgAssert(baseNode.next->size == _allocationInfos[memoryTypeIndex][heapIndex].totalSize);
        mgAssert(baseNode.next->next == nullptr);
        delete baseNode.next;
        baseNode.next = nullptr;
        mgAssert(_deviceMemories[memoryTypeIndex][heapIndex] != nullptr);
        vkFreeMemory(mg::vkContext.device, _deviceMemories[memoryTypeIndex][heapIndex], nullptr);
        _deviceMemories[memoryTypeIndex][heapIndex] = nullptr;
      }
    }
  }
  _hasBeenDelete = true;
}

DeviceHeapAllocation DeviceMemoryAllocator::allocateLargeDeviceOnlyMemory(uint32_t memoryTypeIndex,
                                                                          VkDeviceSize sizeInBytes,
                                                                          VkDeviceSize alignment) {
  const auto size = mg::alignUpPowerOfTwo(sizeInBytes, alignment);
  int32_t index = -1;
  for (int32_t i = 0; i < int32_t(_largeSizeAllocations.size()); i++) {
    if (_largeSizeAllocations[i].deviceMemory == 0) {
      index = i;
      break;
    }
  }
  if (index == -1) {
    _largeSizeAllocations.push_back({});
    _largeSizeAllocations.back().generationIndex = _largSizeGenerationIndex++;
    index = int32_t(_largeSizeAllocations.size() - 1);
  }

  VkMemoryAllocateInfo vkMemoryAllocateInfo = {};
  vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  vkMemoryAllocateInfo.allocationSize = size;
  vkMemoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;
  checkResult(vkAllocateMemory(mg::vkContext.device, &vkMemoryAllocateInfo, nullptr,
                               &_largeSizeAllocations[index].deviceMemory));

  DeviceHeapAllocation deviceHeapAllocation = {};
  deviceHeapAllocation.size = size;
  deviceHeapAllocation.deviceMemory = _largeSizeAllocations[index].deviceMemory;
  deviceHeapAllocation.largeSizeGenerationIndex = _largeSizeAllocations[index].generationIndex;

  deviceHeapAllocation.largeSizeAllocationIndex = index;
  deviceHeapAllocation.offset = 0;

  _largeSizeAllocations[index].size = size;
  _largeSizeAllocations[index].memoryIndex = memoryTypeIndex;

  return deviceHeapAllocation;
}

void DeviceMemoryAllocator::freeLargeDeviceOnlyMemory(const DeviceHeapAllocation &allocation) {
  mgAssert(allocation.largeSizeAllocationIndex >= 0);
  vkFreeMemory(mg::vkContext.device, _largeSizeAllocations[allocation.largeSizeAllocationIndex].deviceMemory, nullptr);
  _largeSizeAllocations[allocation.largeSizeAllocationIndex] = {};
}

DeviceHeapAllocation DeviceMemoryAllocator::allocateDeviceOnlyMemory(uint32_t memoryTypeIndex, VkDeviceSize sizeInBytes,
                                                                     VkDeviceSize alignment) {
  mgAssert(memoryTypeIndex < _nrOfHeapTypes);
  mgAssert(NR_OF_HEAPS > 1);

  if (sizeInBytes > _maxHeapSize)
    return allocateLargeDeviceOnlyMemory(memoryTypeIndex, sizeInBytes, alignment);

  uint32_t heapIndex = 0;
  if (_useDifferentHeapsForSmallAllocations && sizeInBytes > _smallSizeAllocationThreshold)
    heapIndex++;

  for (; heapIndex < NR_OF_HEAPS; heapIndex++) {
    mg::_Node *baseNode = &_memoryPools[memoryTypeIndex][heapIndex];
    if (baseNode->next == nullptr && _allocationInfos[memoryTypeIndex][heapIndex].totalSize == 0) {
      VkMemoryAllocateInfo vkMemoryAllocateInfo = {};
      vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      vkMemoryAllocateInfo.allocationSize = _heapSizes[heapIndex];
      vkMemoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

      checkResult(vkAllocateMemory(mg::vkContext.device, &vkMemoryAllocateInfo, nullptr,
                                   &_deviceMemories[memoryTypeIndex][heapIndex]));
      _allocationInfos[memoryTypeIndex][heapIndex].totalSize = _heapSizes[heapIndex];

      baseNode->next = new mg::_Node{};
      baseNode->next->size = _heapSizes[heapIndex];
    }

    for (mg::_Node *prevNode = baseNode, *currentNode = baseNode->next; currentNode != nullptr;
         prevNode = currentNode, currentNode = currentNode->next) {
      const auto currentNodeAlignedOffset = mg::alignUpPowerOfTwo(currentNode->offset, alignment);
      const auto currentNodeFreeSpace = currentNode->size - (currentNodeAlignedOffset - currentNode->offset);

      if (sizeInBytes <= currentNodeFreeSpace) {
        DeviceHeapAllocation allocation = {};
        allocation.memoryTypeIndex = memoryTypeIndex;
        allocation.deviceMemory = _deviceMemories[memoryTypeIndex][heapIndex];
        allocation.size = sizeInBytes;
        allocation.offset = currentNodeAlignedOffset;

        // same offset
        if (currentNode->offset == currentNodeAlignedOffset) {
          // perfect fit
          if (sizeInBytes == currentNodeFreeSpace) {
            prevNode->next = currentNode->next;
            delete currentNode;
          } else {
            currentNode->size = currentNode->size - sizeInBytes;
            currentNode->offset += sizeInBytes;
          }
        } else { // different offset
          currentNode->size = currentNodeAlignedOffset - currentNode->offset;
          if (currentNodeFreeSpace != sizeInBytes) {
            if (currentNode->next == nullptr) {
              currentNode->next = new mg::_Node();
            }
            currentNode->next->size += currentNodeFreeSpace - sizeInBytes;
            currentNode->next->offset = currentNodeAlignedOffset + sizeInBytes;
          }
        }
        _allocationInfos[memoryTypeIndex][heapIndex].currentSize += sizeInBytes;
        _allocationInfos[memoryTypeIndex][heapIndex].totalNrOfAllocations++;
        _allocationInfos[memoryTypeIndex][heapIndex].allocationNotFreed++;

        return allocation;
      }
    }
  }
  mgAssertDesc(false, "could not allocate device memory from heap");
  return {};
}

void DeviceMemoryAllocator::freeDeviceOnlyMemory(const DeviceHeapAllocation &allocation) {
  // larger allocation, does not belong to a heap
  if (allocation.largeSizeAllocationIndex != -1) {
    const auto index = allocation.largeSizeAllocationIndex;
    mgAssert(allocation.deviceMemory == _largeSizeAllocations[index].deviceMemory);
    mgAssert(_largeSizeAllocations[index].generationIndex == allocation.largeSizeGenerationIndex);
    vkFreeMemory(mg::vkContext.device, allocation.deviceMemory, nullptr);
    _largeSizeAllocations[allocation.largeSizeAllocationIndex] = {};
    return;
  }

  const auto memoryTypeIndex = allocation.memoryTypeIndex;
  mgAssert(memoryTypeIndex < _nrOfHeapTypes);
  int32_t heapIndex = -1;
  for (uint32_t i = 0; i < NR_OF_HEAPS; i++) {
    if (_deviceMemories[memoryTypeIndex][i] == allocation.deviceMemory) {
      heapIndex = i;
      break;
    }
  }
  mgAssert(heapIndex > -1);
  _allocationInfos[memoryTypeIndex][heapIndex].currentSize -= allocation.size;
  _allocationInfos[memoryTypeIndex][heapIndex].allocationNotFreed--;
  mgAssert(_allocationInfos[memoryTypeIndex][heapIndex].currentSize >= 0);
  mgAssert(_allocationInfos[memoryTypeIndex][heapIndex].allocationNotFreed >= 0);

  mg::_Node *baseNode = &_memoryPools[memoryTypeIndex][heapIndex];
  mg::_Node *prevNode = baseNode;
  mg::_Node *currentNode = baseNode->next;
  for (; currentNode != nullptr; prevNode = currentNode, currentNode = currentNode->next) {
    mgAssert(allocation.offset != currentNode->offset);
    if (allocation.offset < currentNode->offset) {
      if (allocation.offset + allocation.size == currentNode->offset) {
        currentNode->offset = allocation.offset;
        currentNode->size += allocation.size;
      } else {
        currentNode = nullptr;
      }
      break;
    }
  }
  if (currentNode == nullptr) {
    currentNode = new _Node{};
    currentNode->offset = allocation.offset;
    currentNode->size = allocation.size;
    currentNode->next = prevNode->next;
    prevNode->next = currentNode;
  }
  if (prevNode != baseNode && prevNode->offset + prevNode->size == currentNode->offset) {
    prevNode->size += currentNode->size;
    prevNode->next = currentNode->next;
    delete currentNode;
  }
}

std::vector<GuiAllocation> DeviceMemoryAllocator::getAllocationForGUI() {
  std::vector<GuiAllocation> guiAllocations;
  for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < _nrOfHeapTypes; memoryTypeIndex++) {
    for (uint32_t heapIndex = 0; heapIndex < NR_OF_HEAPS; heapIndex++) {
      mg::_Node *baseNode = &_memoryPools[memoryTypeIndex][heapIndex];
      _Node *prevNode = baseNode;
      if (_allocationInfos[memoryTypeIndex][heapIndex].totalSize == 0 && baseNode->next == nullptr)
        continue;

      GuiAllocation guiAllocation = {};
      guiAllocation.memoryTypeIndex = memoryTypeIndex;
      guiAllocation.heapIndex = heapIndex;
      guiAllocation.totalSize = uint32_t(_allocationInfos[memoryTypeIndex][heapIndex].totalSize);
      guiAllocation.totalNrOfAllocation = uint32_t(_allocationInfos[memoryTypeIndex][heapIndex].totalNrOfAllocations);
      guiAllocation.allocationNotFreed = uint32_t(_allocationInfos[memoryTypeIndex][heapIndex].allocationNotFreed);

      for (_Node *currentNode = baseNode->next; currentNode != nullptr;
           prevNode = currentNode, currentNode = currentNode->next) {
        VkDeviceSize space = 0;
        space = currentNode->offset - (prevNode->offset + prevNode->size);
        if (space) {
          SubAllocationGui subAllocationGui = {};
          subAllocationGui.free = false;
          subAllocationGui.offset = uint32_t(currentNode->offset - space);
          subAllocationGui.size = uint32_t(space);
          guiAllocation.elements.push_back(subAllocationGui);
        }
        SubAllocationGui subAllocationGui = {};
        subAllocationGui.free = true;
        subAllocationGui.offset = uint32_t(currentNode->offset);
        subAllocationGui.size = uint32_t(currentNode->size);
        guiAllocation.elements.push_back(subAllocationGui);
      }
      auto space = _allocationInfos[memoryTypeIndex][heapIndex].totalSize - (prevNode->offset + prevNode->size);
      if (space) {
        SubAllocationGui subAllocationGui = {};
        subAllocationGui.free = false;
        subAllocationGui.offset = uint32_t(_allocationInfos[memoryTypeIndex][heapIndex].totalSize - space);
        subAllocationGui.size = uint32_t(space);
        guiAllocation.elements.push_back(subAllocationGui);
      }
      guiAllocations.push_back(guiAllocation);
    }
  }
  for (uint32_t i = 0; i < uint32_t(_largeSizeAllocations.size()); i++) {
    if (_largeSizeAllocations[i].size > 0) {
      GuiAllocation guiAllocation = {};
      guiAllocation.totalNrOfAllocation = 1;
      guiAllocation.totalSize = uint32_t(_largeSizeAllocations[i].size);
      guiAllocation.largeDeviceAllocation = true;
      guiAllocation.showFullSize = true;
      guiAllocation.memoryTypeIndex = _largeSizeAllocations[i].memoryIndex;

      SubAllocationGui subAllocationGui = {};
      subAllocationGui.free = false;
      subAllocationGui.offset = 0;
      subAllocationGui.size = uint32_t(_largeSizeAllocations[i].size);
      guiAllocation.elements.push_back(subAllocationGui);
      guiAllocations.push_back(guiAllocation);
    }
  }
  return guiAllocations;
}

} // namespace mg