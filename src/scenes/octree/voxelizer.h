#pragma once
#include <glm/glm.hpp>
#include <vector>

namespace mg {
struct RenderContext;
struct MeshId;
struct EmptyRenderPass;

std::vector<glm::uvec2> voxelizeMesh(const RenderContext &renderContext, const EmptyRenderPass emptyRenderPass,
                                     const MeshId &id, uint32_t octree_levels);
void renderVoxels(const RenderContext &renderContext, std::vector<glm::uvec2> vec, mg::MeshId meshid);
}; // namespace mg