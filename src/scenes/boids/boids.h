#pragma once

#include "mg/meshContainer.h"
#include "rendering/rendering.h"
#include "utils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace mg {
struct FrameData;
}

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
} // namespace bds
