#pragma once

namespace mg {
struct RenderContext;
struct Camera;
} // namespace mg
struct World;

struct RayInfo;
void traceTriangle(const World &world, const mg::Camera &camera, const mg::RenderContext &renderContext, const RayInfo &rayInfo);
void drawImageStorage(const mg::RenderContext &renderContext, const RayInfo &rayInfo);