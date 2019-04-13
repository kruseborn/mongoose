#pragma once
#include <glm/glm.hpp>

namespace mg {
struct RenderContext;
struct Camera;
struct TinyObjMeshes;
} // namespace mg


void renderMRT(const mg::RenderContext &renderContext, const mg::TinyObjMeshes &objMeshes);
void renderSSAO(const mg::RenderContext &renderContext);
void renderBlurSSAO(const mg::RenderContext &renderContext);
void renderFinalDeferred(const mg::RenderContext &renderContext);
