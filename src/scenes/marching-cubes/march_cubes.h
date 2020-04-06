#pragma once
#include "rendering/rendering.h"
#include "mg/storageContainer.h"
#include "mg/window.h"

namespace mg {

struct MarchingCubesStorages {
  mg::StorageId density, triangles, a2iTriangleConnectionTable, aiCubeEdgeFlags;
};
struct Sb {
  VkBuffer drawIndirectBuffer;
  uint32_t drawIndirectBufferOffset;
};

struct Grid {
  uint32_t N;
  float cellSize;
  glm::vec3 corner;
};

MarchingCubesStorages createMarchingCubesStorages(size_t size);
void destroyCreateMarchingCubesStorages(MarchingCubesStorages storages);

Sb simulate(MarchingCubesStorages storages, const Grid &grid);
void renderMC(const RenderContext &renderContext, Sb sb, MarchingCubesStorages storages, const Grid &grid);

void renderGUI(const mg::FrameData &frameData);


}