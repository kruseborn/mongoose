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

void init();

void destroy();
Sb simulate();

void renderMC(const RenderContext &renderContext, Sb sb);
void render2(const RenderContext &renderContext);
void renderGUI(const mg::FrameData &frameData);


}