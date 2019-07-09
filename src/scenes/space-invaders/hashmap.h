#pragma once
#include "array.h"
#include <cassert>

constexpr uint32_t HASHMAP_INITAL_SIZE = 64;
constexpr uint32_t EMPTY = 0xFFFFFFFFu;

struct HashmapEntry {
  uint32_t id;
  uint32_t next;
};

struct Hashmap {
  void init(Allocator *allocator, uint32_t initialSize);

  void add(uint64_t hash, uint32_t id);
  void getEntries(uint64_t hash, Array<uint32_t> *ids) const;
  void clear();
  uint32_t getBucket(uint64_t hash) const;

private:
  Array<uint32_t> _buckets;
  Array<HashmapEntry> _entries;
  uint32_t _size = 0;
};
