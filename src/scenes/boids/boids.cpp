#include "boids.h"
#include "mg/window.h"
#include <random>

namespace {
static constexpr int numBoids = 1024;

static constexpr float width = 100.f;
static constexpr float height = 100.f;

static constexpr float fieldOfView = 15.f;
static constexpr float maxSpeed = 0.1f;
static constexpr float maxForce = 0.001f;
} // namespace

static void applyMaxForce(glm::vec3 &vec) {
  if (glm::length(vec) > maxForce)
    vec = glm::normalize(vec) * maxForce;
}

static glm::vec3 separation(size_t self, const std::vector<size_t> &neighbors, bds::Boids &boids) {
  glm::vec3 offset{};
  glm::vec3 average{};
  int count = 0;

  for (auto i : neighbors) {
    float distance = glm::distance(boids.positions[i], boids.positions[self]);
    if (distance < fieldOfView * 0.7) {
      average += (boids.positions[self] - boids.positions[i]) / distance;
      count++;
    }
  }

  if (count > 0) {
    average /= count;
    if (glm::length(average) != 0.f)
      average = glm::normalize(average) * maxSpeed;
    offset = average - boids.velocities[self];
    applyMaxForce(offset);
  }

  return offset;
}

static glm::vec3 cohesion(size_t self, const std::vector<size_t> &neighbors, bds::Boids &boids) {
  glm::vec3 offset{};
  glm::vec3 centerOfMass{};

  for (auto i : neighbors)
    centerOfMass += boids.positions[i];

  if (neighbors.size() > 0) {
    centerOfMass /= neighbors.size();
    glm::vec3 desired = centerOfMass - boids.positions[self];

    if (glm::length(desired) != 0.f)
      desired = glm::normalize(desired) * maxSpeed;

    offset = desired - boids.velocities[self];
    applyMaxForce(offset);
  }

  return offset;
}

static glm::vec3 alignment(size_t self, const std::vector<size_t> &neighbors, bds::Boids &boids) {
  glm::vec3 offset{};
  glm::vec3 average{};

  for (auto i : neighbors)
    average += boids.velocities[i];

  if (neighbors.size() > 0) {
    average /= float(neighbors.size());
    if (glm::length(average) != 0.f)
      average = glm::normalize(average) * maxSpeed;

    offset = average - boids.velocities[self];
    applyMaxForce(offset);
  }

  return offset;
}

static void applyBehaviour(size_t index, const std::vector<size_t> &neighbors, bds::Boids &boids) {
  glm::vec3 sep = separation(index, neighbors, boids);
  glm::vec3 coh = cohesion(index, neighbors, boids);
  glm::vec3 ali = alignment(index, neighbors, boids);

  boids.offsets[index] += sep;
  boids.offsets[index] += coh;
  boids.offsets[index] += ali;
}

static void updatePositions(bds::Boids &boids, float dt) {
  for (size_t i = 0; i < numBoids; ++i) {
    glm::vec3 &position = boids.positions[i];
    glm::vec3 &velocity = boids.velocities[i];
    glm::vec3 &acceleration = boids.offsets[i];

    velocity += acceleration * dt * 2000.f;

    if (glm::length(velocity) > maxSpeed)
      velocity = glm::normalize(velocity) * maxSpeed;

    position += velocity * dt * 2000.f;
    acceleration = {};
  }
}

static void moveInside(glm::vec3 &position) {
  for (size_t i = 0; i < numBoids; ++i) {
    if (position.x > width)
      position.x = -width;
    else if (position.x < -width)
      position.x = width;
    else if (position.y > height)
      position.y = -height;
    else if (position.y < -height)
      position.y = height;
  }
}

namespace bds {
Boids create() {
  Boids boids{};
  std::mt19937 gen(1337);

  for (size_t i = 0; i < numBoids; ++i) {
    std::uniform_real_distribution<float> distPos(0.f, 100.f);
    std::uniform_real_distribution<float> distVel(0.f, 20.f);

    float posX = distPos(gen) * (i % 2 == 0 ? -1.f : 1.f);
    float posY = distPos(gen) * (i % 2 == 0 ? -1.f : 1.f);

    float velocityX = (distVel(gen) - 0.25f) * 0.01f;
    float velocityY = (distVel(gen) - 0.25f) * 0.01f;

    boids.colors.push_back(glm::vec4{1.f, 1.f, 1.f, 1.f});
    boids.positions.push_back({posX, posY, 0.f});
    boids.offsets.push_back({0, 0, 0});
    boids.velocities.push_back({velocityX, velocityY, 0.f});
  }

  return boids;
}

void update(Boids &boids, const mg::FrameData &frameData, BoidsTime *boidsTime) {
  auto applyBehaviourStart = mg::timer::now();

  std::vector<size_t> neighbors;
  neighbors.reserve(numBoids);

  for (size_t i = 0; i < numBoids; ++i) {
    for (size_t j = 0; j < numBoids; ++j) {
      auto dist = glm::distance(boids.positions[i], boids.positions[j]);
      if (i != j && dist < fieldOfView)
        neighbors.push_back(j);
    }

    applyBehaviour(i, neighbors, boids);
    neighbors.clear();
  }

  auto applyBehaviourEnd = mg::timer::now();
  boidsTime->applyBehaviourTime += uint32_t(mg::timer::durationInUs(applyBehaviourStart, applyBehaviourEnd));

  auto updatePositionTimeStart = mg::timer::now();
  updatePositions(boids, frameData.dt);
  auto updatePositionTimeEnd = mg::timer::now();
  boidsTime->updatePositionTime += uint32_t(mg::timer::durationInUs(updatePositionTimeStart, updatePositionTimeEnd));

  auto moveTimeStart = mg::timer::now();
  for (size_t i = 0; i < numBoids; ++i)
    moveInside(boids.positions[i]);
  auto moveTimeEnd = mg::timer::now();
  boidsTime->moveInside += uint32_t(mg::timer::durationInUs(moveTimeStart, moveTimeEnd));
}

void render(Boids &boids, const mg::FrameData &frameData, mg::RenderContext renderContext, mg::MeshId cubeId) {
  std::vector<glm::mat4> transforms;
  for (size_t i = 0; i < numBoids; ++i) {
    glm::mat4 rotation = glm::rotate(glm::mat4(1), 0.f, {1, 1, 0});
    glm::mat4 translation = glm::translate(glm::mat4(1), boids.positions[i]);
    transforms.push_back(translation * rotation);
  }

  renderCubes(renderContext, cubeId, transforms, boids.colors);
}
} // namespace bds