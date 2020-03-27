#pragma once
#include "mg/storageContainer.h"
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

struct Storages {
  mg::StorageId u, v, u0, v0, d, s;
};

Storages createStorages(size_t N);
void destroyStorages(Storages *storages);
void simulateNavierStroke(const Storages &storages, const mg::FrameData &frameData);
void renderNavierStroke(const mg::RenderContext &renderContext, const Storages &storages);
} // namespace mg