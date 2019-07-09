#include "hashmap.h"

void Hashmap::init(Allocator *allocator, uint32_t initialSize) {
  assert(allocator);

  _buckets.init(allocator, 0);
  _entries.init(allocator, 0, 32);

  uint32_t size = HASHMAP_INITAL_SIZE;
  while (size < initialSize)
    size <<= 1;
  _buckets.resize(size);
  _size = 0;
  memset(_buckets.data(), EMPTY, sizeof(uint32_t) * _buckets.size());
}

void Hashmap::getEntries(uint64_t hash, Array<uint32_t> *ids) const {
  const auto bucket = getBucket(hash);
  auto entryId = _buckets[bucket];
  if (entryId == EMPTY)
    return;
  auto currentEntry = _entries[entryId];
  ids->pushBack(currentEntry.id);
  while (currentEntry.next != EMPTY) {
    currentEntry = _entries[currentEntry.next];
    ids->pushBack(currentEntry.id);
  }
}

void Hashmap::add(uint64_t hash, uint32_t id) {
  const auto bucket = getBucket(hash);
  HashmapEntry entry = {};
  entry.next = EMPTY;
  entry.id = id;
  _entries.pushBack(entry);

  if (_buckets[bucket] == EMPTY) {
    _size++;
    _buckets[bucket] = _entries.size() - 1;
  } else {
    auto entryId = _buckets[bucket];
    auto *currentEntry = &_entries[entryId];
    assert(currentEntry->id != id);
    while (currentEntry->next != EMPTY) {
      currentEntry = &_entries[currentEntry->next];
      assert(currentEntry->id != id);
    }
    currentEntry->next = _entries.size() - 1;
  }
}

void Hashmap::clear() {
  memset(_buckets.data(), EMPTY, sizeof(_buckets.front()) * _buckets.size());
  _entries.clear();
  _size = 0;
}

uint32_t Hashmap::getBucket(uint64_t hash) const {
  assert(_buckets.size());
  return uint32_t(hash & (_buckets.size() - 1));
}
