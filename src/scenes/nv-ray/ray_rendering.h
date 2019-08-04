#pragma once

namespace mg {
struct RenderContext;
} // namespace mg

struct RayInfo;
void traceTriangle(const mg::RenderContext &renderContext, const RayInfo &rayInfo);
void drawImageStorage(const mg::RenderContext &renderContext, const RayInfo &rayInfo);



