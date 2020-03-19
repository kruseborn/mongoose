#pragma once

#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include "mg/meshContainer.h"
#include "rendering/rendering.h"

namespace mg {
struct FrameData;
}

struct BoidsTime {
  uint64_t count = 0;
  uint32_t updatePositionTime = 0;
  uint32_t applyBehaviourTime = 0;
  uint32_t moveInside = 0;

  float textPos = 0;
  float textApply = 0;
  float textMoveInside = 0;
};

namespace bds {
struct Boids {
  std::vector<glm::vec4> colors;
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> velocities;
  std::vector<glm::vec3> offsets;
};

Boids create();

void update(Boids &boids, const mg::FrameData &frameData, BoidsTime *boidsTime);
void render(Boids &boids, const mg::FrameData &frameData, mg::RenderContext renderContext, mg::MeshId cubeId);
} // namespace boids

