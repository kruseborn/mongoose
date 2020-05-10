#pragma once

#include <algorithm>
#include <cstring>
#include <memory>
#include <vector>

template <typename Type> class ChunkedAllocator {
  static const size_t ChunkSize = 4096;

  struct InsertionPoint {
    size_t idx;
    Type data;
  };

  size_t _size;
  std::vector<std::unique_ptr<Type[]>> _data;
  std::vector<InsertionPoint> _insertions;

public:
  ChunkedAllocator() : _size(0) {
  }

  size_t size() {
    return _size;
  }

  size_t insertionCount() {
    return _insertions.size();
  }

  Type &operator[](size_t i) {
    return _data[i / ChunkSize][i % ChunkSize];
  }

  const Type &operator[](size_t i) const {
    return _data[i / ChunkSize][i % ChunkSize];
  }

  void push_back(Type t) {
    if ((_size % ChunkSize) == 0)
      _data.emplace_back(new Type[ChunkSize]);

    _data[_size / ChunkSize][_size % ChunkSize] = std::move(t);
    _size++;
  }

  void insert(size_t index, Type data) {
    _insertions.emplace_back(InsertionPoint{index, std::move(data)});
  }

  std::vector<Type> finalize() {
    std::sort(_insertions.begin(), _insertions.end(),
              [](const InsertionPoint &a, const InsertionPoint &b) { return a.idx < b.idx; });

    size_t length = _size + _insertions.size();
    std::vector<Type> result(length);

    size_t insertionIdx = 0;
    size_t outputOffset = 0;
    size_t inputOffset = 0;
    while (inputOffset < _size) {
      size_t dataIdx = inputOffset / ChunkSize;
      size_t dataOffset = inputOffset % ChunkSize;

      // Clear out data blocks once we've copied them completely
      if (dataOffset == 0 && dataIdx > 0)
        _data[dataIdx - 1].reset();

      size_t copySize = std::min(ChunkSize - dataOffset, _size - inputOffset);
      if (insertionIdx < _insertions.size())
        copySize = std::min(copySize, _insertions[insertionIdx].idx - inputOffset);

      if (copySize > 0) {
        std::memcpy(result.data() + outputOffset, _data[dataIdx].get() + dataOffset, copySize * sizeof(Type));
        inputOffset += copySize;
        outputOffset += copySize;
      }

      if (insertionIdx < _insertions.size() && _insertions[insertionIdx].idx == inputOffset)
        result[outputOffset++] = std::move(_insertions[insertionIdx++].data);
    }

    _insertions.clear();
    _data.clear();
    _size = 0;

    return result;
  }
};

