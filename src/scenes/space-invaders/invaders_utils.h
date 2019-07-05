#pragma once
#include <cassert>
#include <chrono>
#include <cstdint>

#if defined(_WIN32)
#define aligned_malloc _aligned_malloc
#define aligned_free _aligned_free
#else
#define aligned_malloc(size, alignment) aligned_alloc(alignment, size)
#define aligned_free free
#endif

static constexpr uint32_t SIMD_ALIGNMENT = 32;
static constexpr uint32_t SIMD_NR_FLOATS = 8;

struct vec2 {
  float x, y;
};
struct ivec2 {
  int32_t x, y;
};
struct MinMax {
  float xmin, xmax;
  float ymax;
};
struct Limits {
  float left, right, down;
};

inline uint32_t alignUpPowerOfTwo(uint32_t n, uint32_t alignment) { return (n + (alignment - 1)) & ~(alignment - 1); }
inline float sqrtDistance(vec2 p0, vec2 p1) { return ((p0.x - p1.x) * (p0.x - p1.x) + (p0.y - p1.y) * (p0.y - p1.y)); }
inline bool inRange(float value, float left, float right) { return ((value - right) * (value - left) <= 0); }

// Fowler-Noll-Vo_hash_function
// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
static constexpr uint64_t FNV_ffset_basis = 14695981039346656037ULL;
static constexpr uint64_t FNV_prime = 1099511628211ULL;

inline uint64_t hashBytes(const unsigned char *first, const unsigned char *last) {
  assert(sizeof(size_t) == 8 && "size_t is not 8 bytes, build for 64 bits");
  assert(uint64_t(last - first) % 8 == 0);
  uint64_t hash = FNV_ffset_basis;
  for (; first != last; ++first) {
    hash = hash ^ static_cast<uint64_t>(*first);
    hash = hash * FNV_prime;
  }
  return hash;
}

inline uint64_t hashGridPosition(ivec2 gridCoordinates) {
  return hashBytes((const unsigned char *)&gridCoordinates, ((const unsigned char *)&gridCoordinates) + sizeof(gridCoordinates));
}

uint32_t ctz(uint32_t value);
