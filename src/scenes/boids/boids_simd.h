#pragma once

#include "mg/meshContainer.h"
#include "rendering/rendering.h"
#include "utils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace mg {
struct FrameData;
}

namespace bds_simd {
static constexpr uint32_t alignment = 32;
static constexpr uint32_t numBoids = 100;
static constexpr uint32_t numFloats = 8;

struct Boids {
  std::vector<glm::vec4> colors;

  alignas(alignment) float positionsX[numBoids]{0};
  alignas(alignment) float positionsY[numBoids]{0};
  alignas(alignment) float velocitiesX[numBoids]{0};
  alignas(alignment) float velocitiesY[numBoids]{0};
  alignas(alignment) float offsetsX[numBoids]{0};
  alignas(alignment) float offsetsY[numBoids]{0};
};

Boids create();

void update(Boids &boids, const mg::FrameData &frameData, BoidsTime *boidsTime);
void render(Boids &boids, const mg::FrameData &frameData, mg::RenderContext renderContext, mg::MeshId cubeId);
} // namespace bds_simd
