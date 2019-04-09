#pragma once

namespace mg {
struct RenderContext;
struct Camera;
struct MeshId;
} // namespace mg

void drawGltfMesh(const mg::RenderContext &renderContext, mg::MeshId meshId, const mg::Camera &camera);
