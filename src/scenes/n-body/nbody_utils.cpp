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

  std::default_random_engine rndEngine(0);
  std::normal_distribution<float> urand(0.0f, 1.0f);

  // Initial particle positions
  std::normal_distribution<float> dist_distr(0.0f, 0.8f);
  std::normal_distribution<float> velocity_distr(0.0f, 0.5f);
  std::normal_distribution<float> mass_distr(0.5f, 0.1f);

  std::mt19937 rng;
  rng.seed(std::random_device()());

  for (uint32_t i = 0; i < groupSize; i++) {
    for (int j = 0; j < patriclesPerAttractor; j++) {
      double mass = std::fabs(mass_distr(rng));

      glm::vec3 angular_velocity(0, 0, -0.4);

      glm::vec3 position_base = basePositions[i];

      glm::vec3 position(position_base.x + dist_distr(rng), position_base.y + dist_distr(rng),
                         position_base.z + dist_distr(rng) * 0.1);

      glm::vec3 velocity_base(0 + velocity_distr(rng) * 0.02, 0 + velocity_distr(rng) * 0.02, 0 + velocity_distr(rng) * 0.02);
      glm::vec3 r = position - position_base;
      glm::vec3 velocity = glm::cross(r, angular_velocity) + velocity_base;

      const auto index = i * patriclesPerAttractor + j;
      particleBuffer[index].position.x = position.x;
      particleBuffer[index].position.y = position.y;
      particleBuffer[index].position.z = position.z;
      particleBuffer[index].position.w = mass;
      particleBuffer[index].velocity.x = velocity.x;
      particleBuffer[index].velocity.y = velocity.y;
      particleBuffer[index].velocity.z = velocity.z;
    }
  }

  // for (uint32_t i = 0; i < groupSize; i++) {
  //  Particle &particle = particleBuffer[i * patriclesPerAttractor];
  //  particle.position = glm::vec4(groups[i], 2000);
  //  particle.velocity = glm::vec4(0.0f);
  //}

  // for (uint32_t i = 0; i < groupSize; i++) {
  //  for (uint32_t j = 1; j < patriclesPerAttractor; j++) {
  //    Particle &particle = particleBuffer[i * patriclesPerAttractor + j];

  //    glm::vec3 position(groups[i] + glm::vec3(urand(rndEngine), urand(rndEngine), urand(rndEngine)));
  //    const auto mass = (urand(rndEngine) * 10.0f) + 10.0;
  //    particle.position = glm::vec4(position, mass);

  //    auto angular_velocity = glm::vec3(0, 0, 0.5);
  //    auto r = position - groups[i];
  //    auto velocity = (200 / glm::length(r)) * glm::normalize(glm::cross(r, angular_velocity));

  //    particle.velocity = {velocity, 0.0f};
  //  }
  //}
  computeData->storageId =
      mg::mgSystem.storageContainer.createStorage(particleBuffer.data(), mg::sizeofContainerInBytes(particleBuffer));
}