#pragma once
#include <cinttypes>

struct Allocator {
  virtual void init(uint32_t size) = 0;
  virtual void destroy() = 0;

  virtual void *allocate(uint32_t size, uint32_t alignment) = 0;
  virtual void deallocate(void *data) = 0;
  virtual void clear() = 0;

  virtual ~Allocator(){};
};

struct LinearAllocator final : public Allocator {
  virtual void init(uint32_t size) override;
  virtual void destroy() override;

  virtual void *allocate(uint32_t size, uint32_t alignment) override;
  virtual void deallocate(void *data) override;
  virtual void clear() override;

  virtual ~LinearAllocator() override;

private:
  uint8_t *_data = nullptr;
  uint32_t _totalSize = 0;
  uint32_t _count = 0;
  uint32_t _currentOffset = 0;
};
