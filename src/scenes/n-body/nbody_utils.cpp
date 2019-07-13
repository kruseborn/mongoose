#include "nbody_utils.h"
#include "mg/mgSystem.h"
#include <cstdlib>
#include <glm/glm.hpp>
#include <random>

struct Particle {
  glm::vec4 position; // xyz = position, w = mass
  glm::vec4 velocity; // xyz = velocity, w = gradient texture position
};

void initParticles(ComputeData *computeData) {
  constexpr uint32_t patriclesPerAttractor = 256;
  constexpr uint32_t attractorsSize = 1;
  constexpr uint32_t nrOfParticles = patriclesPerAttractor * attractorsSize;
  glm::vec3 attractors[attractorsSize] = {
      glm::vec3(0.0f, 0.0f, 0.0f),  /*glm::vec3(-5.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 5.0f),
      glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 4.0f, 0.0f),  glm::vec3(0.0f, -8.0f, 0.0f),*/
  };

  Particle particleBuffer[nrOfParticles];
  computeData->nrOfParticles = nrOfParticles;

  std::default_random_engine rndEngine(0);
  std::normal_distribution<float> rndDist(0.0f, 1.0f);

  // First particle in group as heavy center of gravity
  for (uint32_t i = 0; i < attractorsSize; i++) {
    Particle &particle = particleBuffer[i * patriclesPerAttractor];
    particle.position = glm::vec4(attractors[i] * 1.5f, 90000.0f);
    particle.velocity = glm::vec4(0.0f);
    // Color gradient offset
    particle.velocity.w = (float)i * 1.0f / static_cast<uint32_t>(attractorsSize);
  }

  for (uint32_t i = 0; i < attractorsSize; i++) {
    for (uint32_t j = 1; j < patriclesPerAttractor; j++) {
      Particle &particle = particleBuffer[i * patriclesPerAttractor + j];

      // Position
      glm::vec3 position(attractors[i] + glm::vec3(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine)) * 0.75f);
      const auto len = glm::length(glm::normalize(position - attractors[i]));
      position.y *= 2.0f - (len * len);

      // Velocity
      glm::vec3 angular = glm::vec3(0.5f, 1.5f, 0.5f) * (((i % 2) == 0) ? 1.0f : -1.0f);
      glm::vec3 velocity = glm::cross((position - attractors[i]), angular) +
                           glm::vec3(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine) * 0.025f);

      const auto mass = (rndDist(rndEngine) * 0.5f + 0.5f) * 75.0f;
      particle.position = glm::vec4(position, mass);
      particle.velocity = glm::vec4(velocity, 0.0f);

      // Color gradient offset
      particle.velocity.w = (float)i / float(attractorsSize);
    }
  }
  computeData->storageId = mg::mgSystem.storageContainer.createStorage(particleBuffer, sizeof(particleBuffer));
}