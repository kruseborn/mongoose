#pragma once
#include "vulkan/vkContext.h"
#include <glm/glm.hpp>
#include <string>

namespace mg {
struct RenderContext;
struct Texts;

struct RenderContext {
  VkRenderPass renderPass;
  uint32_t subpass;
  glm::mat4 projection, view;
};

void renderSolidBoxes(const mg::RenderContext &renderContext, const float *xPositions, const float *yPositions,
                      const glm::vec4 *colors, glm::vec2 size, uint32_t count, bool useSameColor);
  void renderBoxWithTexture(mg::RenderContext &renderContext, const glm::vec4 &position, const std::string &id);
void renderBoxWithDepthTexture(mg::RenderContext &renderContext, const glm::vec4 &position, const std::string &id);
void renderText(const mg::RenderContext &renderContext, const mg::Texts &texts);



} // namespace