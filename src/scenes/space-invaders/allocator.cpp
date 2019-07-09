#include "allocator.h"
#include "invaders_utils.h"
#include <cassert>
#include <cstdlib>

void LinearAllocator::init(uint32_t size) {
  assert(size > 0);
  _data = (uint8_t *)aligned_malloc(size, SIMD_ALIGNMENT);
  _totalSize = size;
}
void LinearAllocator::destroy() {
  assert(_count == 0);
  aligned_free(_data);
  _data = nullptr;
  _totalSize = 0;
}

void *LinearAllocator::allocate(uint32_t size, uint32_t alignment) {
  assert(_data != nullptr);
  assert(_totalSize > 0);
  _count++;

  _currentOffset = alignUpPowerOfTwo(_currentOffset, alignment);
  const auto realSize = size + alignment;
  assert((_currentOffset + realSize) <= _totalSize && "allocate more memory for linear allocator");

  const auto oldOffset = _currentOffset;
  _currentOffset += realSize;
  return &_data[oldOffset];
}

void LinearAllocator::deallocate(void *data) {
  if (data == nullptr)
    return;
  assert(_count > 0);
  _count--;
}
void LinearAllocator::clear() {
  assert(_count == 0);
  _currentOffset = 0;
}

LinearAllocator::~LinearAllocator() {
  assert(_data == nullptr);
  assert(_totalSize == 0);
  assert(_currentOffset == 0);
}