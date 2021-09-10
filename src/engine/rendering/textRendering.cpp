#include "rendering/rendering.h"

#include "mg/fonts.h"
#include "mg/mgSystem.h"
#include "mg/texts.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace mg {

static mg::Pipeline createFontPipeline(const mg::RenderContext &renderContext) {
  using namespace mg::shaders::fontRendering;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.rasterization.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.rasterization.vkPipelineLayout = vkContext.pipelineLayouts.pipelineLayout;
  pipelineStateDesc.rasterization.graphics.subpass = renderContext.subpass;
  pipelineStateDesc.rasterization.rasterization.cullMode = VK_CULL_MODE_NONE;
  pipelineStateDesc.rasterization.depth.TestEnable = VK_FALSE;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = shader;
  createPipelineInfo.vertexInputState = InputAssembler::vertexInputState;
  createPipelineInfo.vertexInputStateCount = mg::countof(InputAssembler::vertexInputState);

  const auto pipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);
  return pipeline;
}

static std::vector<glm::vec4> getTextsVertexBufferData(mg::FONT_TYPE fontType, const _Text texts[Texts::maxTextPerFont],
                                                       uint32_t count, const mg::Fonts &fonts) {
  size_t charVectorSize = 0;
  for (uint32_t i = 0; i < count; i++) {
    charVectorSize += texts[i].text.size();
  }

  std::vector<glm::vec4> charVectorData;
  charVectorData.reserve(charVectorSize * 24);

  for (uint32_t i = 0; i < count; i++) {
    const auto text = texts[i];
    auto position = glm::vec2{floorf(text.position.x), floorf(text.position.y)};
    // Iterate through all characters
    for (const auto &textChar : text.text) {
      const auto &character = fonts.getFontCharacter(fontType, textChar);

      const glm::vec2 corner_pos = {position.x + float(character.bearing.x),
                                    position.y - (character.size.y - float(character.bearing.y))};
      const glm::vec3 size = {float(character.size.x), float(character.size.y), 1.0f};
      const glm::ivec2 fontMapSize = fonts.getFontMapSize(fontType);

      const auto glyphPosition = float(character.textureOffset) / float(fontMapSize.x);
      const auto glyphWidth = float(character.size.x) / float(fontMapSize.x);
      const auto glyphHeight = float(character.size.y) / float(fontMapSize.y);

      charVectorData.push_back(glm::vec4{corner_pos.x, corner_pos.y + size.y, glyphPosition, 0.0f});
      charVectorData.push_back(text.color);
      charVectorData.push_back(glm::vec4{corner_pos.x, corner_pos.y, glyphPosition, glyphHeight});
      charVectorData.push_back(text.color);
      charVectorData.push_back(glm::vec4{corner_pos.x + size.x, corner_pos.y, glyphPosition + glyphWidth, glyphHeight});
      charVectorData.push_back(text.color);
      charVectorData.push_back(glm::vec4{corner_pos.x, corner_pos.y + size.y, glyphPosition, 0.0f});
      charVectorData.push_back(text.color);
      charVectorData.push_back(glm::vec4{corner_pos.x + size.x, corner_pos.y, glyphPosition + glyphWidth, glyphHeight});
      charVectorData.push_back(text.color);
      charVectorData.push_back(
          glm::vec4{corner_pos.x + size.x, corner_pos.y + size.y, glyphPosition + glyphWidth, 0.0f});
      charVectorData.push_back(text.color);
      // Now advance cursors for next glyph
      position.x += character.advance;
    }
  }
  return charVectorData;
}

static void renderCharacters(const mg::RenderContext &renderContext, const mg::FONT_TYPE fontType,
                             const std::vector<glm::vec4> &charVectorData, const mg::Fonts &fonts) {
  auto pipeline = createFontPipeline(renderContext);

  using namespace mg::shaders::fontRendering;
  using VertexInputData = InputAssembler::VertexInputData;

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;

  Ubo *dynamic =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);

  dynamic->projection =
      glm::ortho(0.0f, float(mg::vkContext.screen.width), 0.0f, float(mg::vkContext.screen.height), -10.0f, 10.0f);

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.textures = mg::getTextureDescriptorSet();  

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  TextureIndices textureIndices = {};
  textureIndices.textureIndex = mg::getTexture2DDescriptorIndex(fonts.getFontGlyphMap(fontType));
  vkCmdPushConstants(mg::vkContext.commandBuffer, pipeline.layout, VK_SHADER_STAGE_ALL, 0, sizeof(TextureIndices),
                     &textureIndices);

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

  VkBuffer buffer;
  VkDeviceSize buffer_offset;
  const uint32_t vertexCount = uint32_t(charVectorData.size() / 2); // positions and colors
  uint32_t totalSize = sizeof(VertexInputData) * vertexCount;
  auto *vertices =
      (VertexInputData *)mg::mgSystem.linearHeapAllocator.allocateBuffer(totalSize, &buffer, &buffer_offset);
  memcpy(vertices, charVectorData.data(), totalSize);

  vkCmdBindVertexBuffers(mg::vkContext.commandBuffer, 0, 1, &buffer, &buffer_offset);
  vkCmdDraw(mg::vkContext.commandBuffer, vertexCount, 1, 0, 0);
}

void renderText(const mg::RenderContext &renderContext, const mg::Texts &texts) {
  const auto &fontsToTexts = texts.fontsToTexts;
  for (uint32_t i = 0; i < uint32_t(mg::FONT_TYPE::SIZE); ++i) {
    if (texts.textsPerFont[i] == 0)
      continue;

    const auto fontType = (mg::FONT_TYPE)i;
    const auto &vertexBufferData =
        getTextsVertexBufferData(fontType, fontsToTexts[i], texts.textsPerFont[i], mg::mgSystem.fonts);
    mgAssert(vertexBufferData.size() != 0);
    renderCharacters(renderContext, fontType, vertexBufferData, mg::mgSystem.fonts);
  }
}

} // namespace mg