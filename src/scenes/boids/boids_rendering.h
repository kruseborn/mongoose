#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <span>
namespace mg {
struct RenderContext;
struct MeshId;
struct FrameData;

void renderCubes(const RenderContext &renderContext, const FrameData &frameData, std::span<glm::vec2> positions,
                 const mg::MeshId &cubeId);

}; // namespace mg