#include "boids_simd.h"
#include "mg/window.h"
#include "v8f.h"
#include <random>

namespace {
static constexpr float width = 100.f;
static constexpr float height = 100.f;

static constexpr float fieldOfView = 15.f;
static constexpr float maxSpeed = 0.1f;
static constexpr float maxForce = 0.001f;

static constexpr uint32_t numFloats = 8;

static const v8f maxSpeeds = v8f(maxSpeed);
static const v8f maxForces = v8f(maxForce);
static const v8f zeros = v8f(0.f);
} // namespace

static float separation(size_t self, const std::vector<size_t> &neighbors, bds_simd::Boids &boids) {
  float x{}, y{};
  int count = 0;

  for (auto i : neighbors) {
    float dx = boids._px[self] - boids._px[i];
    float dy = boids._py[self] - boids._py[i];
    float distance = sqrt(dx * dx + dy * dy);

    if (distance < fieldOfView * 0.7) {
      x += dx / distance;
      y += dy / distance;
      count++;
    }
  }

  boids._sepx[self] = x;
  boids._sepy[self] = y;

  return (float)count;
}

static void cohesion(size_t self, const std::vector<size_t> &neighbors, bds_simd::Boids &boids) {
  float x{}, y{};

  for (auto i : neighbors) {
    x += boids._px[i];
    y += boids._py[i];
  }

  boids._cohx[self] = x;
  boids._cohy[self] = y;
}

static void alignment(size_t self, const std::vector<size_t> &neighbors, bds_simd::Boids &boids) {
  float x{}, y{};

  for (auto i : neighbors) {
    x += boids._vx[i];
    y += boids._vy[i];
  }

  boids._alix[self] = x;
  boids._aliy[self] = y;
}

static void cap(v8f &vx, v8f &vy, const v8f &max) {
  v8f len = length(vx, vy);
  v8f mask = len > max;
  len = max / len;

  vx = select(mask, vx * len, vx);
  vy = select(mask, vy * len, vy);
}

static void applyMaxSpeed(v8f &vx, v8f &vy, v8f mask) {
  v8f len = length(vx, vy);
  mask = mask && len != zeros;
  len = maxSpeeds / len;

  vx = select(mask, vx * len, vx);
  vy = select(mask, vy * len, vy);
}

static void separations(bds_simd::Boids &boids, float *counts) {
  size_t vi = 0;

  for (size_t i = 0; i < bds_simd::vsize; i++, vi += numFloats) {
    v8f counts(&counts[vi]);
    v8f hasNeighbors = counts > zeros;

    v8f sepx(&boids._sepx[vi]);
    v8f sepy(&boids._sepy[vi]);

    sepx = select(hasNeighbors, sepx / counts, zeros);
    sepy = select(hasNeighbors, sepy / counts, zeros);

    applyMaxSpeed(sepx, sepy, hasNeighbors);

    sepx = if_sub(hasNeighbors, sepx, &boids._vx[vi]);
    sepy = if_sub(hasNeighbors, sepy, &boids._vy[vi]);

    cap(sepx, sepy, maxForces);

    sepx.store(&boids._offx[vi]);
    sepy.store(&boids._offy[vi]);
  }
}

static void cohesions(bds_simd::Boids &boids, float *counts) {
  size_t vi = 0;

  for (size_t i = 0; i < bds_simd::vsize; i++, vi += numFloats) {
    v8f counts(&counts[vi]);
    v8f hasNeighbors = counts > zeros;

    v8f cohx(&boids._cohx[vi]);
    v8f cohy(&boids._cohy[vi]);

    cohx = select(hasNeighbors, cohx / counts - &boids._px[vi], zeros);
    cohy = select(hasNeighbors, cohy / counts - &boids._py[vi], zeros);

    applyMaxSpeed(cohx, cohy, hasNeighbors);

    cohx = if_sub(hasNeighbors, cohx, &boids._vx[vi]);
    cohy = if_sub(hasNeighbors, cohy, &boids._vy[vi]);

    cap(cohx, cohy, maxForces);

    cohx += &boids._offx[vi];
    cohy += &boids._offy[vi];

    cohx.store(&boids._offx[vi]);
    cohy.store(&boids._offy[vi]);
  }
}

static void alignments(bds_simd::Boids &boids, float *counts) {
  size_t vi = 0;

  for (size_t i = 0; i < bds_simd::vsize; i++, vi += numFloats) {
    v8f counts(&counts[vi]);
    v8f hasNeighbors = counts > zeros;

    v8f alix(&boids._alix[vi]);
    v8f aliy(&boids._aliy[vi]);

    alix = select(hasNeighbors, alix / counts, zeros);
    aliy = select(hasNeighbors, aliy / counts, zeros);

    applyMaxSpeed(alix, aliy, hasNeighbors);

    alix = if_sub(hasNeighbors, alix, &boids._vx[vi]);
    aliy = if_sub(hasNeighbors, aliy, &boids._vy[vi]);

    cap(alix, aliy, maxForces);

    alix += &boids._offx[vi];
    aliy += &boids._offy[vi];

    alix.store(&boids._offx[vi]);
    aliy.store(&boids._offy[vi]);
  }
}

static void updatePositions(bds_simd::Boids &boids, float dt) {
  v8f vmul(dt * 2000.f);
  size_t vi = 0;

  for (size_t i = 0; i < bds_simd::vsize; i++, vi += numFloats) {
    v8f vx(&boids._vx[vi]);
    v8f vy(&boids._vy[vi]);

    vx += &boids._offx[vi] * vmul;
    vy += &boids._offy[vi] * vmul;

    cap(vx, vy, maxSpeeds);

    vx.store(&boids._vx[vi]);
    vy.store(&boids._vy[vi]);
  }

  vi = 0;
  for (size_t i = 0; i < bds_simd::vsize; i++, vi += numFloats) {
    v8f px(&boids._px[vi]);
    v8f py(&boids._py[vi]);

    px += &boids._vx[vi] * vmul;
    py += &boids._vy[vi] * vmul;

    px.store(&boids._px[vi]);
    py.store(&boids._py[vi]);
  }
}

static void applyBehaviours(bds_simd::Boids &boids) {
  std::vector<size_t> neighbors;
  neighbors.reserve(bds_simd::numBoids);

  alignas(bds_simd::alignment) float distances[8];
  alignas(bds_simd::alignment) float counts[bds_simd::numBoids]{0};
  alignas(bds_simd::alignment) float sepCounts[bds_simd::numBoids]{0};

  for (size_t i = 0; i < bds_simd::numBoids; i++) {
    size_t vi = 0;
    neighbors.clear();

    v8f px(boids._px[i]);
    v8f py(boids._py[i]);

    for (size_t j = 0; j < bds_simd::vsize; j++, vi += numFloats) {
      v8f dist = sqDist(&boids._px[vi] - px, &boids._py[vi] - py);
      dist.store(&distances[0]);

      for (int k = 0; k < 8; ++k) {
        if (i != vi + k && distances[k] < fieldOfView * fieldOfView)
          neighbors.push_back(vi + k);
      }
    }

    if (!neighbors.empty()) {
      counts[i] = (float)neighbors.size();
      sepCounts[i] = separation(i, neighbors, boids);
      cohesion(i, neighbors, boids);
      alignment(i, neighbors, boids);
    }
  }

  separations(boids, &sepCounts[0]);
  cohesions(boids, &counts[0]);
  alignments(boids, &counts[0]);
}

static void moveInside(bds_simd::Boids &boids) {
  size_t vi = 0;

  v8f wMin(-width);
  v8f wMax(width);
  v8f hMin(-height);
  v8f hMax(height);

  for (size_t j = 0; j < bds_simd::vsize; j++, vi += numFloats) {
    v8f x(&boids._px[vi]);
    v8f y(&boids._py[vi]);

    x = select(x > wMax, wMin, x);
    x = select(x < wMin, wMax, x);
    y = select(y > hMax, hMin, y);
    y = select(y < hMin, hMax, y);

    x.store(&boids._px[vi]);
    y.store(&boids._py[vi]);
  }
}

namespace bds_simd {
Boids create() {
  Boids boids{};
  std::mt19937 gen(1337);

  for (size_t i = 0; i < bds_simd::numBoids; ++i) {
    std::uniform_real_distribution<float> distPos(0.f, 100.f);
    std::uniform_real_distribution<float> distVel(0.f, 20.f);

    boids._px[i] = distPos(gen) * (i % 2 == 0 ? -1.f : 1.f);
    boids._py[i] = distPos(gen) * (i % 2 == 0 ? -1.f : 1.f);

    boids._vx[i] = (distVel(gen) - 0.25f) * 0.01f;
    boids._vy[i] = (distVel(gen) - 0.25f) * 0.01f;

    boids.colors.push_back(glm::vec4{1.f, 1.f, 1.f, 1.f});
  }

  return boids;
}

void update(Boids &boids, const mg::FrameData &frameData, BoidsTime *boidsTime) {
  {
    auto applyBehaviourStart = mg::timer::now();
    applyBehaviours(boids);
    auto applyBehaviourEnd = mg::timer::now();
    boidsTime->applyBehaviourTime += uint32_t(mg::timer::durationInUs(applyBehaviourStart, applyBehaviourEnd));
  }

  {
    auto updatePositionTimeStart = mg::timer::now();
    updatePositions(boids, frameData.dt);
    auto updatePositionTimeEnd = mg::timer::now();
    boidsTime->updatePositionTime += uint32_t(mg::timer::durationInUs(updatePositionTimeStart, updatePositionTimeEnd));
  }

  {
    auto moveTimeStart = mg::timer::now();
    moveInside(boids);
    auto moveTimeEnd = mg::timer::now();
    boidsTime->moveInside += uint32_t(mg::timer::durationInUs(moveTimeStart, moveTimeEnd));
  }
}

void render(Boids &boids, const mg::FrameData &frameData, mg::RenderContext renderContext, mg::MeshId cubeId) {
  std::vector<glm::mat4> transforms;
  for (size_t i = 0; i < bds_simd::numBoids; ++i) {
    glm::mat4 rotation = glm::rotate(glm::mat4(1), 0.f, {1, 1, 0});
    glm::mat4 translation = glm::translate(glm::mat4(1), glm::vec3{boids._px[i], boids._py[i], 0.f});
    transforms.push_back(translation * rotation);
  }

  renderCubes(renderContext, cubeId, transforms, boids.colors);
}
} // namespace bds_simd