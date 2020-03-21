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
static constexpr uint32_t numBoids = 1024;
static constexpr uint32_t numFloats = 8;
static constexpr uint32_t vsize = bds_simd::numBoids / numFloats;

struct Boids {
  std::vector<glm::vec4> colors;

  alignas(alignment) float _px[numBoids]{0};
  alignas(alignment) float _py[numBoids]{0};
 
  alignas(alignment) float _vx[numBoids]{0};
  alignas(alignment) float _vy[numBoids]{0};
  
  alignas(alignment) float _offx[numBoids]{0};
  alignas(alignment) float _offy[numBoids]{0};

  alignas(alignment) float _sepx[numBoids]{0};
  alignas(alignment) float _sepy[numBoids]{0};

  alignas(alignment) float _cohx[numBoids]{0};
  alignas(alignment) float _cohy[numBoids]{0};
  
  alignas(alignment) float _alix[numBoids]{0};
  alignas(alignment) float _aliy[numBoids]{0};
};

Boids create();

void update(Boids &boids, const mg::FrameData &frameData, BoidsTime *boidsTime);
void render(Boids &boids, const mg::FrameData &frameData, mg::RenderContext renderContext, mg::MeshId cubeId);
} // namespace bds_simd
