#include "boids_simd.h"
#include "mg/window.h"
#include "v8f.h"
#include <random>

namespace {
static const float width = 100.f;
static const float height = 100.f;

static const float fieldOfView = 15.f;
static const float maxSpeed = 0.1f;
static const float maxForce = 0.001f;

static constexpr uint32_t numFloats = 8;
} // namespace

static void applyMaxForce(glm::vec3 &vec) {
  if (glm::length(vec) > maxForce)
    vec = glm::normalize(vec) * maxForce;
}

static glm::vec3 separation(size_t self, const std::vector<size_t> &neighbors, bds_simd::Boids &boids) {
  glm::vec3 offset{};
  glm::vec3 average{};
  int count = 0;

  glm::vec3 positionSelf{boids.positionsX[self], boids.positionsY[self], 0.f};

  for (auto i : neighbors) {
    glm::vec3 positionOther{boids.positionsX[i], boids.positionsY[i], 0.f};
    float distance = glm::distance(positionOther, positionSelf);

    if (distance < fieldOfView * 0.7) {
      average += (positionSelf - positionOther) / distance;
      count++;
    }
  }

  if (count > 0) {
    average /= count;
    if (glm::length(average) != 0.f)
      average = glm::normalize(average) * maxSpeed;
    offset = average - glm::vec3{boids.velocitiesX[self], boids.velocitiesY[self], 0.f};
    applyMaxForce(offset);
  }

  return offset;
}

static glm::vec3 cohesion(size_t self, const std::vector<size_t> &neighbors, bds_simd::Boids &boids) {
  glm::vec3 offset{};
  glm::vec3 centerOfMass{};

  for (auto i : neighbors)
    centerOfMass += glm::vec3{boids.positionsX[i], boids.positionsY[i], 0.f};

  if (neighbors.size() > 0) {
    centerOfMass /= neighbors.size();
    glm::vec3 desired = centerOfMass - glm::vec3{boids.positionsX[self], boids.positionsY[self], 0.f};

    if (glm::length(desired) != 0.f)
      desired = glm::normalize(desired) * maxSpeed;

    offset = desired - glm::vec3{boids.velocitiesX[self], boids.velocitiesY[self], 0.f};
    applyMaxForce(offset);
  }

  return offset;
}

static glm::vec3 alignment(size_t self, const std::vector<size_t> &neighbors, bds_simd::Boids &boids) {
  glm::vec3 offset{};
  glm::vec3 average{};

  for (auto i : neighbors)
    average += glm::vec3{boids.velocitiesX[i], boids.velocitiesY[i], 0.f};

  if (neighbors.size() > 0) {
    average /= float(neighbors.size());
    if (glm::length(average) != 0.f)
      average = glm::normalize(average) * maxSpeed;

    offset = average - glm::vec3{boids.velocitiesX[self], boids.velocitiesY[self], 0.f};
    applyMaxForce(offset);
  }

  return offset;
}

static void applyBehaviour(size_t index, const std::vector<size_t> &neighbors, bds_simd::Boids &boids) {
  glm::vec3 sep = separation(index, neighbors, boids);
  glm::vec3 coh = cohesion(index, neighbors, boids);
  glm::vec3 ali = alignment(index, neighbors, boids);

  boids.offsetsX[index] = sep.x + coh.x + ali.x;
  boids.offsetsY[index] = sep.y + coh.y + ali.y;
}

const uint32_t vsize = bds_simd::numBoids / numFloats + 1;

static void updatePositions(bds_simd::Boids &boids, float dt) {
  v8f vmul(dt * 2000.f);
  size_t vi = 0;

  for (size_t i = 0; i < vsize; i++, vi += numFloats) {
    v8f px(&boids.positionsX[vi]);
    v8f py(&boids.positionsY[vi]);
    v8f vx(&boids.velocitiesX[vi]);
    v8f vy(&boids.velocitiesY[vi]);
    v8f ox(&boids.offsetsX[vi]);
    v8f oy(&boids.offsetsY[vi]);

    vx += ox * vmul;
    vy += oy * vmul;

    // Cap at maxSpeed
    v8f len = length(vx, vy);

    v8f max(maxSpeed);
    v8f vxMax = vx * max / len;
    v8f vyMax = vy * max / len;

    v8f mask = len > max;

    vx = if_select(mask, vxMax, vx);
    vy = if_select(mask, vyMax, vy);

    px += vx * vmul;
    py += vy * vmul;

    vx.store(&boids.velocitiesX[vi]);
    vy.store(&boids.velocitiesY[vi]);

    px.store(&boids.positionsX[vi]);
    py.store(&boids.positionsY[vi]);
  }

  std::memset(&boids.offsetsX, 0, bds_simd::numBoids * sizeof(float));
  std::memset(&boids.offsetsY, 0, bds_simd::numBoids * sizeof(float));
}

static void applyBehaviours(bds_simd::Boids& boids) {
  std::vector<size_t> neighbors;
  neighbors.reserve(bds_simd::numBoids);
  float distances[8];

  for (size_t i = 0; i < bds_simd::numBoids; i++) {
    size_t vi = 0;

    v8f pxa(boids.positionsX[i]);
    v8f pya(boids.positionsY[i]);

    for (size_t j = 0; j < vsize; j++, vi += numFloats) {
      v8f pxb(&boids.positionsX[vi]);
      v8f pyb(&boids.positionsY[vi]);

      v8f distance = length(pxa - pxb, pya - pyb);
      distance.store(&distances[0]);

      for (int k = 0; k < 8; ++k) {
        if (i != vi + k && distances[k] < fieldOfView)
          neighbors.push_back(vi + k);
      }
    }

    if (neighbors.empty())
      continue;

    applyBehaviour(i, neighbors, boids);
    neighbors.clear();
  }
}

static void moveInside(bds_simd::Boids &boids) {
  size_t vi = 0;
  
  v8f wMin(-width);
  v8f wMax(width);
  v8f hMin(-height);
  v8f hMax(height);

  for (size_t j = 0; j < vsize; j++, vi += numFloats) {
    v8f x(&boids.positionsX[vi]);
    v8f y(&boids.positionsY[vi]);

    v8f maskXMaxOutside = x > wMax;
    v8f maskXMinOutside = x < wMin;
    v8f maskYMaxOutside = y > hMax;
    v8f maskYMinOutside = y < hMin;

    x = if_select(maskXMaxOutside, wMin, x);
    x = if_select(maskXMinOutside, wMax, x);
    y = if_select(maskYMaxOutside, hMin, y);
    y = if_select(maskYMinOutside, hMax, y);

    x.store(&boids.positionsX[vi]);
    y.store(&boids.positionsY[vi]);
  }
}

namespace bds_simd {
Boids create() {
  Boids boids{};
  std::mt19937 gen(1337);

  for (size_t i = 0; i < bds_simd::numBoids; ++i) {
    std::uniform_real_distribution<float> distPos(0.f, 100.f);
    std::uniform_real_distribution<float> distVel(0.f, 20.f);

    boids.positionsX[i] = distPos(gen) * (i % 2 == 0 ? -1.f : 1.f);
    boids.positionsY[i] = distPos(gen) * (i % 3 == 0 ? -1.f : 1.f);

    boids.velocitiesX[i] = (distVel(gen) - 0.25f) * 0.01f;
    boids.velocitiesY[i] = (distVel(gen) - 0.25f) * 0.01f;

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
    glm::mat4 translation = glm::translate(glm::mat4(1), glm::vec3{boids.positionsX[i], boids.positionsY[i], 0.f});
    transforms.push_back(translation * rotation);
  }

  renderCubes(renderContext, cubeId, transforms, boids.colors);
}
} // namespace bds_simd