#pragma once

#include "allocator.h"
#include "invaders_utils.h"
#include <cassert>
#include <cstdint>
#include <cstring>

template <typename T> struct Array {
  Array();
  Array(const Array<T> &other);
  ~Array();

  T &operator[](uint32_t idx);
  const T &operator[](uint32_t idx) const;

  void init(Allocator *allocator, uint32_t size, uint32_t capacity = 0);
  void reserve(uint32_t capacity);
  void resize(uint32_t size);
  void pushBack(T t);

  T *data();
  void clear();
  uint32_t size() const;
  T front() const;
  T back() const;

private:
  T *_data = nullptr;
  uint32_t _size = 0;
  uint32_t _capacity = 0;
  Allocator *_allocator = nullptr;
};

template <typename T> Array<T>::Array() { static_assert(std::is_trivially_copyable<T>::value, "Array values must be pod"); }

template <typename T> void Array<T>::init(Allocator *allocator, uint32_t size, uint32_t capacity) {
  assert(allocator);
  assert(_allocator == nullptr);
  _allocator = allocator;
  reserve(capacity);
  resize(size);
}

template <typename T> Array<T>::Array(const Array<T> &other) {
  _allocator = other._allocator;
  _capacity = other._capacity;
  _size = other._size;
  _data = _allocator->allocate(other._size * sizeof(T), SIMD_ALIGNMENT);
  std::memcpy(_data, other._data, sizeof(T) * other._size);
}

template <typename T> Array<T>::~Array() {
  _allocator->deallocate(_data);
  _allocator = nullptr;
  _size = 0;
  _capacity = 0;
}

template <typename T> T &Array<T>::operator[](uint32_t idx) {
  assert(idx < _size && "array out of bounds");
  return _data[idx];
}
template <typename T> const T &Array<T>::operator[](uint32_t idx) const {
  assert(idx < _size && "array out of bounds");
  return _data[idx];
}

template <typename T> void Array<T>::reserve(uint32_t capacity) {
  assert(_allocator);
  if (capacity <= _capacity)
    return;
  _capacity = capacity;
  T *temp = (T *)_allocator->allocate(_capacity * sizeof(T), SIMD_ALIGNMENT);
  memcpy(temp, _data, sizeof(T) * _size);
  _allocator->deallocate(_data);
  _data = temp;
}

template <typename T> void Array<T>::resize(uint32_t size) {
  assert(_allocator);
  reserve(size);
  if (size > _size) {
    for (uint32_t i = _size; i < size; i++) {
      _data[i] = T{};
    }
  }
  _size = size;
}

template <typename T> void Array<T>::pushBack(T t) {
  assert(_allocator);
  if (_size >= _capacity) {
    reserve(!_capacity ? 8 : _capacity * 2);
  }
  _data[_size++] = t;
}

template <typename T> void Array<T>::clear() { _size = 0; }
template <typename T> uint32_t Array<T>::size() const { return _size; }
template <typename T> T *Array<T>::data() { return _data; }

template <typename T> T Array<T>::front() const {
  assert(_size > 0);
  return _data[0];
}
template <typename T> T Array<T>::back() const {
  assert(_size > 0);
  return _data[_size - 1];
}
