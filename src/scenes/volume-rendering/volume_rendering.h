#pragma once

namespace mg {
struct RenderContext;
struct FrameData;
struct Camera;
} // namespace mg

struct VolumeInfo;

void drawFrontAndBack(const mg::RenderContext &renderContext, const VolumeInfo &volumeInfo);
void drawVolume(const mg::RenderContext &renderContext, const mg::Camera &camera, const VolumeInfo &volumeInfo,
                const mg::FrameData &frameData);

void drawDenoise(const mg::RenderContext &renderContext);
