#pragma once

#include "mg/storageContainer.h"
#include <cinttypes>

namespace mg {
struct FrameData;
struct RenderContext;

struct OctreeStorages {
  mg::StorageId octree, fragmentList, buildInfo;
};

void initOctreeStorages(OctreeStorages *storages, const std::vector<glm::uvec2> &voxels);
void destroyOctreeStorages(OctreeStorages *storages);
void buildOctree(const OctreeStorages &storages, uint32_t octreeLevel, const mg::FrameData &frameData, uint32_t voxels);

}; // namespace mg