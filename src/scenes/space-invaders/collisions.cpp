#include "collisions.h"
#include "allocator.h"
#include "hashmap.h"
#include "invaders_scene.h"
#include <bitset>
#include <climits>
#include <immintrin.h>

#define BITMASK(b) (1 << ((b) % CHAR_BIT))
#define BITSLOT(b) ((b) / CHAR_BIT)
#define BITSET(a, b) ((a)[BITSLOT(b)] |= BITMASK(b))
#define BITTEST(a, b) ((a)[BITSLOT(b)] & BITMASK(b))
#define BITNSLOTS(nb) ((nb + CHAR_BIT - 1) / CHAR_BIT)

std::pair<bool, uint32_t> checkCollision(const float x1, const float y1, const float *x2, const float *y2,
                                         const Array<uint32_t> &entries, float radius) {
  assert(x2);
  assert(y2);
  const uint32_t vsize = entries.size() / SIMD_NR_FLOATS;
  const uint32_t size = entries.size();

  const auto squaredRadius = radius * radius;

  __m256 sqRad = _mm256_set1_ps(squaredRadius);
  __m256 vx1 = _mm256_set1_ps(x1);
  __m256 vy1 = _mm256_set1_ps(y1);
  uint32_t vi = 0;
  for (uint32_t j = 0; j < vsize; j++, vi += SIMD_NR_FLOATS) {
    __m256 vx2 = _mm256_load_ps(&x2[vi]);
    __m256 vy2 = _mm256_load_ps(&y2[vi]);
    __m256 vxsub = _mm256_sub_ps(vx1, vx2);
    __m256 vysub = _mm256_sub_ps(vy1, vy2);
    __m256 vxmul = _mm256_mul_ps(vxsub, vxsub);
    __m256 vymul = _mm256_mul_ps(vysub, vysub);
    __m256 dst2 = _mm256_add_ps(vxmul, vymul);

    __m256 cmp = _mm256_cmp_ps(dst2, sqRad, _CMP_LE_OQ);
    const int32_t hit = !_mm256_testz_ps(cmp, cmp);
    if (hit) {
      uint32_t pos2 = ctz(_mm256_movemask_ps(cmp));
      return {true, entries[vi + pos2]};
    }
  }
  for (; vi < size; vi++) {
    const auto squaredDistance = sqrtDistance({x1, y1}, {x2[vi], y2[vi]});
    if (squaredDistance <= squaredRadius) {
      return {true, entries[vi]};
    }
  }
  return {false, 0};
}

void getCollisionIds(const Hashmap &hashmap, const CheckCollisionInfo &checkCollisionInfo, CollisionIds *ids) {
  assert(ids);
  Array<uint32_t> entries;
  Array<float> xPositions;
  Array<float> yPositions;
  Array<char> bitArray;

  bitArray.init(&device.linearAllocator, BITNSLOTS(checkCollisionInfo.gridObjects.size));
  entries.init(&device.linearAllocator, 0, 32);
  xPositions.init(&device.linearAllocator, 0, 32);
  yPositions.init(&device.linearAllocator, 0, 32);

  const auto gridObjects = checkCollisionInfo.gridObjects;
  const auto collisionObjects = checkCollisionInfo.collisionObjects;
  const auto cellSize = checkCollisionInfo.cellSize;

  for (uint32_t i = 0; i < collisionObjects.size; i++) {
    ivec2 centerGridCoordinates;
    centerGridCoordinates.x = int32_t(collisionObjects.x[i] / cellSize);
    centerGridCoordinates.y = int32_t(collisionObjects.y[i] / cellSize);

    enum { DIR_SIZE = 9 };
    constexpr int32_t xdir[DIR_SIZE] = {-1, 0, 1, -1, 0, 1, -1, 0, 1};
    constexpr int32_t ydir[DIR_SIZE] = {-1, -1, -1, 0, 0, 0, 1, 1, 1};

    entries.clear();
    xPositions.clear();
    yPositions.clear();

    for (uint32_t j = 0; j < DIR_SIZE; j++) {
      const auto gridCoord = ivec2{centerGridCoordinates.x + xdir[j], centerGridCoordinates.y + ydir[j]};
      const auto hash = hashGridPosition(gridCoord);
      hashmap.getEntries(hash, &entries);
    }
    for (uint32_t j = 0; j < entries.size(); j++) {
      xPositions.pushBack(gridObjects.x[entries[j]]);
      yPositions.pushBack(gridObjects.y[entries[j]]);
    }
    const auto res = checkCollision(collisionObjects.x[i], collisionObjects.y[i], xPositions.data(), yPositions.data(), entries,
                                    collisionObjects.radius + gridObjects.radius);
    if (res.first && !BITTEST(bitArray, res.second)) {
      BITSET(bitArray, res.second);
      ids->collisionObjects.pushBack(i);
      ids->gridObjects.pushBack(res.second);
    }
  }
}
