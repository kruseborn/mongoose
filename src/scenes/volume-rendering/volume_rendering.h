#pragma once

namespace mg {
struct RenderContext;
struct FrameData;
struct Camera;
} // namespace mg

struct VolumeInfo;
struct VolumeRenderPass;

void drawFrontAndBack(const mg::RenderContext &renderContext, const VolumeInfo &volumeInfo);
void drawVolume(const mg::RenderContext &renderContext, const mg::Camera &camera, const VolumeInfo &volumeInfo,
                const mg::FrameData &frameData, const VolumeRenderPass &volumeRenderPass);

void drawDenoise(const mg::RenderContext &renderContext, const VolumeRenderPass &volumeRenderPass);
