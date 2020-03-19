#include "boids.h"
#include "mg/window.h"
#include <random>

namespace constants {
static const float width = 100.f;
static const float height = 100.f;

static const int numBoids = 130;

static const float perception = 15.f;
static const float maxSpeed = 0.1f;
static const float maxForce = 0.001f;

static const float wSeparation = 1.f;
static const float wCohesion = 1.f;
static const float wAlignment = 1.f;

} // namespace constants

static void applyMaxForce(glm::vec3 &vec) {
  if (glm::length(vec) > constants::maxForce)
    vec = glm::normalize(vec) * constants::maxForce;
}

static glm::vec3 separation(size_t self, const std::vector<size_t> &neighbors, bds::Boids &boids) {
  glm::vec3 offset{};
  glm::vec3 average{};
  int count = 0;

  for (auto i : neighbors) {
    float distance = glm::distance(boids.positions[i], boids.positions[self]);
    if (distance < constants::perception * 0.7) {
      average += (boids.positions[self] - boids.positions[i]) / distance;
      count++;
    }
  }

  if (count > 0) {
    average /= count;
    if (glm::length(average) != 0.f)
      average = glm::normalize(average) * constants::maxSpeed;
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
      desired = glm::normalize(desired) * constants::maxSpeed;

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
    average /= (float)neighbors.size();
    if (glm::length(average) != 0.f)
      average = glm::normalize(average) * constants::maxSpeed;
    
    offset = average - boids.velocities[self];
    applyMaxForce(offset);
  }

  return offset;
}

void applyBehaviour(size_t index, const std::vector<size_t> &neighbors, bds::Boids &boids) {
  glm::vec3 sep = separation(index, neighbors, boids);
  glm::vec3 coh = cohesion(index, neighbors, boids);
  glm::vec3 ali = alignment(index, neighbors, boids);

  boids.offsets[index] += sep;
  boids.offsets[index] += coh;
  boids.offsets[index] += ali;
}

void updatePositions(bds::Boids &boids, float dt) {
  for (size_t i = 0; i < constants::numBoids; ++i) {
    glm::vec3 &position = boids.positions[i];
    glm::vec3 &velocity = boids.velocities[i];
    glm::vec3 &acceleration = boids.offsets[i];

    velocity += acceleration * dt * 2000.f;

    if (glm::length(velocity) > constants::maxSpeed)
      velocity = glm::normalize(velocity) * constants::maxSpeed;

    position += velocity * dt * 2000.f;
    acceleration = {};
  }
}

void moveInside(glm::vec3 &position) {
  for (size_t i = 0; i < constants::numBoids; ++i) {
    if (position.x > constants::width)
      position.x = -constants::width;
    if (position.x < -constants::width)
      position.x = constants::width;
    if (position.y > constants::height)
      position.y = -constants::height;
    if (position.y < -constants::height)
      position.y = constants::height;
    if (position.z < -constants::height)
      position.z = constants::height;
    if (position.z < -constants::height)
      position.z = constants::height;
  }
}

namespace bds {
Boids create() {
  Boids boids{};
  std::mt19937 gen(1337);

  for (size_t i = 0; i < constants::numBoids; ++i) {
    std::uniform_real_distribution<float> distPos(0.f, 100.f);
    std::uniform_real_distribution<float> distVel(0.f, 20.f);

    float posX = distPos(gen) * (i % 2 == 0 ? -1.f : 1.f);
    float posY = distPos(gen) * (i % 3 == 0 ? -1.f : 1.f);

    float velocityX = (distVel(gen) - 0.25f) * 0.01f;
    float velocityY = (distVel(gen) - 0.25f) * 0.01f;

    boids.colors.push_back(glm::vec4{1.f, 1.f, 1.f, 1.f});
    boids.positions.push_back({posX, posY, 0.f});
    boids.offsets.push_back({0, 0, 0});
    boids.velocities.push_back({velocityX, velocityY, 0.f});
  }

  return boids;
}

void update(Boids &boids, const mg::FrameData &frameData) {
  std::vector<size_t> neighbors;
  neighbors.reserve(constants::numBoids);

  for (size_t i = 0; i < constants::numBoids; ++i) {
    neighbors.clear();

    for (size_t j = 0; j < constants::numBoids; ++j) {
      if (i != j && glm::distance(boids.positions[i], boids.positions[j]) < constants::perception)
        neighbors.push_back(j);
    }

    applyBehaviour(i, neighbors, boids);
  }

  updatePositions(boids, frameData.dt);

  for (size_t i = 0; i < constants::numBoids; ++i)
    moveInside(boids.positions[i]);
}

void render(Boids &boids, const mg::FrameData &frameData, mg::RenderContext renderContext, mg::MeshId cubeId) {
  std::vector<glm::mat4> transforms;
  for (size_t i = 0; i < constants::numBoids; ++i) {
    glm::mat4 rotation = glm::rotate(glm::mat4(1), 0.f, {1, 1, 0});
    glm::mat4 translation = glm::translate(glm::mat4(1), boids.positions[i]);
    transforms.push_back(translation * rotation);
  }

  renderCubes(renderContext, cubeId, transforms, boids.colors);
}
} // namespace bds