#include "boids.h"
#include <unordered_map>
#include <array>

inline static float rng() {
  return rand() / float(RAND_MAX);
};
inline static float sRng() {
  return rand() / float(RAND_MAX) - 0.5f;
};

static const float maxSpeed = 3.0f;
static const float maxForce = 0.1f;
static const float radius = 5.0f;
static const float cellSize = 50.0f;
static const float invCellSize = 1 / cellSize;

struct Boids {
  static const uint32_t count = 65520;
  glm::vec3 positions[count];
  glm::vec3 acceleration[count];
  glm::vec3 velocity[count];
} boids;

struct Cell {
  static const uint16_t maxSize = 500;
  uint16_t uids[maxSize];
  uint16_t count = 0;
};

// Fowler-Noll-Vo_hash_function
// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
static constexpr uint64_t FNV_ffset_basis = 14695981039346656037ULL;
static constexpr uint64_t FNV_prime = 1099511628211ULL;

inline uint64_t hashBytes(const unsigned char *first, const unsigned char *last) {
  uint64_t hash = FNV_ffset_basis;
  for (; first != last; ++first) {
    hash = hash ^ static_cast<uint64_t>(*first);
    hash = hash * FNV_prime;
  }
  return hash;
}

namespace std {
template <> struct hash<glm::uvec3> {
  std::size_t operator()(glm::uvec3 const &v) const noexcept {
    return (v.x * 18397) + (v.y * 20483) + (v.z * 29303);
    //return hashBytes((const unsigned char *)&v[0], (const unsigned char *)&v[0] + sizeof(glm::ivec3));
  }
};
} // namespace std

inline static float distanceSqr(glm::vec3 a, glm::vec3 b) {
  float dx = a[0] - b[0];
  float dy = a[1] - b[1];
  float dz = a[2] - b[2];

  return dx * dx + dy * dy + dz * dz;
}

std::unordered_map<glm::uvec3, Cell> grid;

void init(float w, float h, float d) {
  for (int i = 0; i < boids.count; i++)
    boids.positions[i] = {rng() * w, rng() * h, rng() * d};

  for (int i = 0; i < boids.count; i++) {
    boids.acceleration[i] = {};
  }
  for (int i = 0; i < boids.count; i++) {
    boids.velocity[i] = {sRng(), sRng(), sRng()};
    boids.velocity[i] = glm::normalize(boids.velocity[i]) * (rng() * 2.0f + 2.0f);
  }
}

inline glm::vec3 limit(glm::vec3 vec, float max) {
  float sqrtMag = vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2];
  if (sqrtMag > max * max * max) {
    vec /= sqrt(sqrtMag);
    vec *= max;
  }
  return vec;
}

inline glm::ivec3 positionToGridCoord(glm::vec3 position) {
  glm::vec3 tmp = position * invCellSize + 0.5f;
  glm::uvec3 coord = {tmp.x, tmp.y, tmp.z};
  return coord;
}

void applyBoidsRulesGrid() {
  for (uint32_t i = 0; i < boids.count; i++) {
    glm::vec3 alignment = {};
    glm::vec3 cohesion = {};
    glm::vec3 separation = {};
    uint32_t count = 0;

    glm::ivec3 coord = positionToGridCoord(boids.positions[i]);
    const Cell &cell = grid[coord];

    for (uint32_t j = 0, size = uint32_t(cell.count); j < size; j++) {
      const uint32_t neighborIndex = cell.uids[j];

      const glm::vec3 diff = boids.positions[i] - boids.positions[neighborIndex];
      const float dis = distanceSqr(boids.positions[i], boids.positions[neighborIndex]);
      float cnd = float(dis < 50 && dis > 0.1);

      alignment += boids.velocity[neighborIndex] * cnd;
      cohesion += boids.positions[neighborIndex] * cnd;
      separation += cnd * diff / float(sqrt(dis + 0.0001));
      count += uint32_t(cnd);
    }
    if (count) {
      alignment /= count;
      alignment = glm::normalize(alignment) * maxSpeed;
      alignment -= boids.velocity[i];
      alignment = limit(alignment, maxForce);

      cohesion /= count;
      cohesion -= boids.positions[i];
      cohesion = glm::normalize(cohesion) * maxSpeed;
      cohesion -= boids.velocity[i];
      cohesion = limit(cohesion, maxForce);

      separation /= count;
      separation = glm::normalize(separation) * maxSpeed;
      separation -= boids.velocity[i];
      separation = limit(separation, maxForce);
    }
    boids.acceleration[i] += alignment;
    boids.acceleration[i] += cohesion;
    boids.acceleration[i] += separation;
  }
}

void populateGrid() {
  for (uint32_t i = 0; i < boids.count; i++) {
    glm::ivec3 coord = positionToGridCoord(boids.positions[i]);
    auto &cell = grid[coord];
    cell.uids[cell.count++] = i;
    assert(cell.count < cell.maxSize);
  }
}

void updatePositions(float w, float h, float d, float dt) {
  for (uint32_t i = 0; i < boids.count; i++) {
    boids.positions[i] += boids.velocity[i] * dt * 500.0f;
    boids.velocity[i] += boids.acceleration[i] * dt * 500.0f;
    boids.velocity[i] = limit(boids.velocity[i], maxSpeed);
    boids.acceleration[i] = {};

    if (boids.positions[i].x < 0)
      boids.positions[i].x = w;
    else if (boids.positions[i].x > w)
      boids.positions[i].x = 0;

    if (boids.positions[i].y < 0)
      boids.positions[i].y = h;
    else if (boids.positions[i].y > h)
      boids.positions[i].y = 0;

    if (boids.positions[i].z < 0)
      boids.positions[i].z = d;
    else if (boids.positions[i].z > d)
      boids.positions[i].z = d;
  }
}

std::span<glm::vec3> simulate(float w, float h, float d, float dt) {
  grid.clear();

  populateGrid();
  applyBoidsRulesGrid();
  //applyBoidsRules();
  updatePositions(w, h, d, dt);

  return {boids.positions, boids.count};
}