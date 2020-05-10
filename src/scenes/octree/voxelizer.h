#pragma once
#include <glm/glm.hpp>
#include <vector>

namespace mg {
struct RenderContext;
struct MeshId;
struct EmptyRenderPass;

std::vector<glm::uvec4> createOctreeFromMesh(const RenderContext &renderContext, const EmptyRenderPass emptyRenderPass, const MeshId &id);
void renderCubes(const RenderContext &renderContext, std::vector<glm::uvec4> vec, mg::MeshId meshid);

std::vector<uint32_t> buildOctree(const std::vector<glm::uvec4> &vec, glm::uvec3 pos, uint32_t size);
}; // namespace mg