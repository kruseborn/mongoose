#pragma once
#include <cstdint>

namespace mg {
struct FrameData;
struct RenderContext;

void simulateNavierStroke2(const mg::FrameData &frameData);
void renderNavierStroke2(const mg::RenderContext &renderContext);
} // namespace mg
