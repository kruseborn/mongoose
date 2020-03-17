#include "boids.h"
#include "mg/window.h"

namespace bds {
Boids create() {
  Boids boids{};
  boids.colors.push_back({1, 1, 1, 1});
  boids.rotations.push_back(0.f);
  boids.positions.push_back({0, 0, 0});

  return boids;
}

void update(Boids &boids, const mg::FrameData &frameData) {
  float rotAngle = frameData.dt * 20.0f;

  for (size_t i = 0; i < boids.rotations.size(); ++i) {
    boids.rotations[i] += rotAngle;
  }
}

void render(Boids &boids, const mg::FrameData &frameData, mg::RenderContext renderContext, mg::MeshId cubeId) {
  for (size_t i = 0; i < boids.rotations.size(); ++i) {
    glm::mat4 rotation = glm::rotate(glm::mat4(1), boids.rotations[i], {1, 1, 0});
    renderCube(renderContext, cubeId, rotation, boids.colors[i]);
  }
}
} // namespace boids