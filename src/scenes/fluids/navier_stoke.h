#pragma once
#include "mg/storageContainer.h"
#include <cstdint>

namespace mg {
struct FrameData;
struct RenderContext;

struct Storages {
  mg::StorageId u, v, u0, v0, d, s;
};

Storages createStorages(size_t N);
void destroyStorages(Storages *storages);
void simulateNavierStoke(const Storages &storages, const mg::FrameData &frameData, uint32_t N);
void renderNavierStoke(const mg::RenderContext &renderContext, const Storages &storages);
} // namespace mg