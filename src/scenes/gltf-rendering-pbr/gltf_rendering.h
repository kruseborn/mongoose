#pragma once

#include <unordered_map>

namespace mg {
struct RenderContext;
struct Camera;
struct MeshId;
struct TextureId;
} // namespace mg

void drawGltfMesh(const mg::RenderContext &renderContext, mg::MeshId meshId, const mg::Camera &camera,
                  const std::unordered_map<std::string, mg::TextureId> &nameToTextureId);
