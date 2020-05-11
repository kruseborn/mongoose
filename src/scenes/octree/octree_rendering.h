#pragma once
#include "sdf.h"
#include <glm/glm.hpp>

namespace mg {
struct RenderContext;
struct FrameData;
struct Camera;
struct OctreeStorages;
struct MeshId;

struct Menu {
  bool octree;
  bool sdf;
  bool cubes;
};

void renderSdf(const mg::RenderContext &renderContext, Sdf sdf, const FrameData &frameData, const Camera &camera);
void renderOctree(const mg::OctreeStorages &storages, const mg::RenderContext &renderContext, uint32_t octreeLevel,
                  const mg::FrameData &frameData, glm::vec3 position);

void renderVoxels(const RenderContext &renderContext, std::vector<glm::uvec2> vec, const mg::MeshId &cubeId);
void renderMenu(const mg::FrameData &frameData, void *data);

}; // namespace mg