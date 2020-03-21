#pragma once
#include <cstdint>

namespace mg {
struct FrameData;
struct RenderContext;

struct FluidTimes {
  size_t count;
  float diffuse, project, advec;
  uint32_t diffuseSum, projectSum, advecSum;
};

extern FluidTimes fluidTimes;

void simulateNavierStroke(const mg::FrameData &frameData);
void renderNavierStroke(const mg::RenderContext &renderContext);
}// namespace mg