#pragma once

namespace mg {
struct RenderContext;
struct Camera;
struct FrameData;
} // namespace mg

struct ComputeData;
struct NBodyRenderPass;

void simulatePartices(const ComputeData &computeData, const mg::FrameData &frameData);
void renderParticels(mg::RenderContext &renderContext, const ComputeData &computeData, const mg::Camera &camera);
void renderToneMapping(const mg::RenderContext &renderContext, const NBodyRenderPass &nBodyRenderPass);



