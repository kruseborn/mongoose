#include "boids.h"

float rng() {
  return rand() / float(RAND_MAX);
};
float sRng() {
  return rand() / float(RAND_MAX) - 0.5f;
};

static float maxSpeed = 3.0f;
static float maxForce = 0.1f;
static float radius = 5.0f;
struct Boids {
  static const uint32_t count = 300;
  glm::vec2 positions[count];
  glm::vec2 acceleration[count];
  glm::vec2 velocity[count];
} boids;

void init(float w, float h) {
  for (int i = 0; i < boids.count; i++)
    boids.positions[i] = {rng() * w, rng() * h};

  for (int i = 0; i < boids.count; i++) {
    boids.acceleration[i] = {};
  }
  for (int i = 0; i < boids.count; i++) {
    boids.velocity[i] = {sRng(), sRng()};
    boids.velocity[i] = glm::normalize(boids.velocity[i]) * (rng() * 2.0f + 2.0f);
  }
}

glm::vec2 limit(glm::vec2 vec, float max) {
  float sqrtMag = vec[0] * vec[0] + vec[1] * vec[1];
  if (sqrtMag > max * max) {
    vec /= sqrt(sqrtMag);
    vec *= max;
  }
  return vec;
}

glm::vec3 limit(glm::vec3 vec, float max) {
  float sqrtMag = vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2];
  if (sqrtMag > max * max * max) {
    vec /= sqrt(sqrtMag);
    vec *= max;
  }
  return vec;
}

void applyBoidsRules() {
  // For every boid in the system, check if it's too close
  for (uint32_t i = 0; i < boids.count; i++) {
    glm::vec2 alignment = {};
    glm::vec2 cohesion = {};
    glm::vec2 separation = {};
    uint32_t count = 0;
    for (uint32_t j = 0; j < boids.count; j++) {
      if (i == j)
        continue;
      
      glm::vec2 dir = boids.positions[i] - boids.positions[j];
      const float dis = glm::distance(boids.positions[i], boids.positions[j]);
      if (dis < 50) {
        alignment += boids.velocity[j];
        cohesion += boids.positions[j];
        separation += dir / dis;
        count++;
      }
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

void updatePositions(float w, float h, float dt) {
  for (uint32_t i = 0; i < boids.count; i++) {
    boids.positions[i] += boids.velocity[i];
    boids.velocity[i] += boids.acceleration[i];
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
  }
}

std::span<glm::vec2> simulate(float w, float h, float dt) {
  applyBoidsRules();
  updatePositions(w, h, dt);

  return {boids.positions, boids.count};
}