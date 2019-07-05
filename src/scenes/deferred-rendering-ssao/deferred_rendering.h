#pragma once
#include <glm/glm.hpp>

namespace mg {
struct RenderContext;
struct Camera;
struct ObjMeshes;
} // namespace mg


void renderMRT(const mg::RenderContext &renderContext, const mg::ObjMeshes &objMeshes);
void renderSSAO(const mg::RenderContext &renderContext);
void renderBlurSSAO(const mg::RenderContext &renderContext);
void renderFinalDeferred(const mg::RenderContext &renderContext);
