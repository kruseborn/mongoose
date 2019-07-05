#include "transforms.h"
#include "hashmap.h"
#include "types.h"
#include <algorithm>
#include <cassert>
#include <immintrin.h>

void transformPositions(float *xPositions, float *yPositions, uint32_t size, vec2 direction, float speed, float dt) {
  assert(xPositions);
  assert(yPositions);
  const uint32_t vsize = size / SIMD_NR_FLOATS;

  const float xMul = direction.x * speed * dt;
  const float yMul = direction.y * speed * dt;

  __m256 vxMul = _mm256_set1_ps(xMul);
  __m256 vyMul = _mm256_set1_ps(yMul);
  uint32_t vi = 0;
  for (uint32_t j = 0; j < vsize; j++, vi += SIMD_NR_FLOATS) {
    __m256 vx = _mm256_load_ps(&xPositions[vi]);
    __m256 vy = _mm256_load_ps(&yPositions[vi]);

    vx = _mm256_add_ps(vx, vxMul);
    vy = _mm256_add_ps(vy, vyMul);

    _mm256_store_ps(&xPositions[vi], vx);
    _mm256_store_ps(&yPositions[vi], vy);
  }
  for (; vi < size; vi++) {
    xPositions[vi] += xMul;
    yPositions[vi] += yMul;
  }
}

MinMax transformPositionsCalculateLimits(float *xPositions, float *yPositions, uint32_t size, vec2 direction, float speed, float dt) {
  assert(xPositions);
  assert(yPositions);

  const uint32_t vsize = size / SIMD_NR_FLOATS;
  const float xmul = direction.x * speed * dt;
  const float ymul = direction.y * speed * dt;
  constexpr auto maxFloat = std::numeric_limits<float>::max();
  constexpr auto minFloat = std::numeric_limits<float>::lowest();

  __m256 vxmul = _mm256_set1_ps(xmul);
  __m256 vymul = _mm256_set1_ps(ymul);
  __m256 vxmax = _mm256_set1_ps(minFloat);
  __m256 vxmin = _mm256_set1_ps(maxFloat);
  __m256 vymax = _mm256_set1_ps(minFloat);
  uint32_t vi = 0;
  for (uint32_t j = 0; j < vsize; j++, vi += SIMD_NR_FLOATS) {
    __m256 vx = _mm256_load_ps(&xPositions[vi]);
    __m256 vy = _mm256_load_ps(&yPositions[vi]);

    vx = _mm256_add_ps(vx, vxmul);
    vy = _mm256_add_ps(vy, vymul);

    vxmax = _mm256_max_ps(vxmax, vx);
    vxmin = _mm256_min_ps(vxmin, vx);
    vymax = _mm256_max_ps(vymax, vy);

    _mm256_store_ps(&xPositions[vi], vx);
    _mm256_store_ps(&yPositions[vi], vy);
  }
  alignas(SIMD_ALIGNMENT) float maxXValues[SIMD_NR_FLOATS];
  alignas(SIMD_ALIGNMENT) float minXValues[SIMD_NR_FLOATS];
  alignas(SIMD_ALIGNMENT) float maxYValues[SIMD_NR_FLOATS];

  _mm256_store_ps(maxXValues, vxmax);
  _mm256_store_ps(minXValues, vxmin);
  _mm256_store_ps(maxYValues, vymax);

  MinMax minMax;
  minMax.xmax = minFloat;
  minMax.xmin = maxFloat;
  minMax.ymax = minFloat;

  for (uint32_t j = 0; j < SIMD_NR_FLOATS; j++) {
    minMax.xmax = std::max(minMax.xmax, maxXValues[j]);
    minMax.xmin = std::min(minMax.xmin, minXValues[j]);
    minMax.ymax = std::max(minMax.ymax, maxYValues[j]);
  }
  for (; vi < size; vi++) {
    xPositions[vi] += xmul;
    yPositions[vi] += ymul;
    minMax.xmax = std::max(minMax.xmax, xPositions[vi]);
    minMax.xmin = std::min(minMax.xmin, xPositions[vi]);
    minMax.ymax = std::max(minMax.ymax, yPositions[vi]);
  }
  return minMax;
}
void addTransformToGrid(const float *x, const float *y, uint32_t size, Hashmap *hashmap, float cellSize) {
  assert(x);
  assert(y);
  assert(hashmap);
  for (uint32_t i = 0; i < size; i++) {
    ivec2 centerGridCoordinates;
    centerGridCoordinates.x = int32_t(x[i] / cellSize);
    centerGridCoordinates.y = int32_t(y[i] / cellSize);
    const auto hash = hashGridPosition(centerGridCoordinates);
    hashmap->add(hash, i);
  }
}

void changeAlienDirectionIfNeeded(Aliens *aliens, MinMax minMax, Limits limits, float dt) {
  assert(aliens);
  const auto xdir = AlienXDirs[aliens->dirIndex];
  const auto ydir = AlienYDirs[aliens->dirIndex];

  aliens->currentDistanceMoveInY += ydir * aliens->speed * dt;
  const bool changeToXDir = ydir && uint32_t(aliens->currentDistanceMoveInY >= limits.down);

  const bool outsideLeft = minMax.xmin <= limits.left;
  const bool outsideRight = minMax.xmax >= limits.right;
  const bool changeToYDirFromLeft = (outsideLeft && xdir == -1);
  const bool changeToYDirFromRight = (outsideRight && xdir == 1);
  const bool changeToYDir = changeToYDirFromLeft || changeToYDirFromRight;

  const uint32_t changeDir = uint32_t(changeToXDir || changeToYDir);

  aliens->currentDistanceMoveInY *= !changeToYDir;
  aliens->dirIndex = (aliens->dirIndex + changeDir) % ALIEN_DIR_SIZE;
}