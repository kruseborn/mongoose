#pragma once
#include <glm/glm.hpp>
#include <vector>

namespace mg {
struct RenderContext;
struct MeshId;
struct EmptyRenderPass;

std::vector<glm::uvec2> voxelizeMesh(const RenderContext &renderContext, const EmptyRenderPass emptyRenderPass,
                                     const MeshId &id, uint32_t octree_levels);
}; // namespace mg