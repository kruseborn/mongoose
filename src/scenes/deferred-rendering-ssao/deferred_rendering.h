#pragma once
#include <glm/glm.hpp>

namespace mg {
struct RenderContext;
struct Camera;
struct ObjMeshes;
} // namespace mg

struct DeferredRenderPass;
struct Noise;
void renderMRT(const mg::RenderContext &renderContext, const mg::ObjMeshes &objMeshes);
void renderSSAO(const mg::RenderContext &renderContext, const DeferredRenderPass &deferredRenderPass, const Noise &noise);
void renderBlurSSAO(const mg::RenderContext &renderContext, const DeferredRenderPass &deferredRenderPass);
void renderFinalDeferred(const mg::RenderContext &renderContext, const DeferredRenderPass &deferredRenderPass);
