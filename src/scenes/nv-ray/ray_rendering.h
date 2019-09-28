#pragma once

namespace mg {
struct RenderContext;
} // namespace mg
struct World;

struct RayInfo;
void traceTriangle(const World &world, const mg::RenderContext &renderContext, const RayInfo &rayInfo);
void drawImageStorage(const mg::RenderContext &renderContext, const RayInfo &rayInfo);