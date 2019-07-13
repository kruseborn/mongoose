#include "nbody_utils.h"
#include "mg/mgSystem.h"
#include <cstdlib>
#include <glm/glm.hpp>
#include <random>

struct Particle {
  glm::vec4 position;
  glm::vec4 velocity;
};

void initParticles(ComputeData *computeData) {
  constexpr uint32_t patriclesPerAttractor = 256 * 100;
  constexpr uint32_t groupSize = 2;
  constexpr uint32_t nrOfParticles = patriclesPerAttractor * groupSize;
  glm::vec3 basePositions[groupSize] = {glm::vec3(1.5, 0.1, 0.0), glm::vec3(-1.5, -0.1, 0.0)};

  std::vector<Particle> particleBuffer(nrOfParticles);
  computeData->nrOfParticles = nrOfParticles;

  std::normal_distribution<float> rand(0.0f, 1.0f);
  std::mt19937 rng;

  for (uint32_t i = 0; i < groupSize; i++) {
    for (uint32_t j = 0; j < patriclesPerAttractor; j++) {
      float mass = rand(rng) * 0.5f + 0.4f;

      const auto position_base = basePositions[i];
      const auto position =
          glm::vec3{position_base.x + rand(rng), position_base.y + rand(rng), position_base.z + rand(rng) * 0.1f};

      const auto velocity_base = glm::vec3{rand(rng) * 0.01f, rand(rng) * 0.01f, rand(rng) * 0.01f};
      const auto r = position - position_base;
      const auto angular_velocity = glm::vec3{0.f, 0.f, 0.5f};
      const auto velocity = glm::cross(r, angular_velocity) + velocity_base;

      const auto index = i * patriclesPerAttractor + j;
      particleBuffer[index].position = {position, mass};
      particleBuffer[index].velocity = {velocity, 0.0f};
    }
  }

  computeData->storageId =
      mg::mgSystem.storageContainer.createStorage(particleBuffer.data(), mg::sizeofContainerInBytes(particleBuffer));
}