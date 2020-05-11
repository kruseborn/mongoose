#pragma once
#include "sdf.h"

namespace mg {
struct RenderContext;
struct FrameData;
struct Camera;
void renderSdf(const mg::RenderContext &renderContext, Sdf sdf, const FrameData &frameData, const Camera &camera);
};