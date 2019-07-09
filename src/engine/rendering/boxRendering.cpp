#include "rendering.h"

#include "mg/mgSystem.h"
#include "vulkan/pipelineContainer.h"
#include <cmath>

namespace mg {

static mg::Pipeline createSolidRendering(const mg::RenderContext &renderContext) {
  using namespace mg::shaders::textureRendering;

  const auto pipelineLayout = mg::vkContext.pipelineLayout;
  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.vkPipelineLayout = mg::getPipelineLayout(shaderResource::resources, mg::countof(shaderResource::resources));
  pipelineStateDesc.graphics.subpass = renderContext.subpass;
  pipelineStateDesc.depth.TestEnable = VK_FALSE;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = "solid";
  createPipelineInfo.vertexInputState = InputAssembler::vertexInputState;
  createPipelineInfo.vertexInputStateCount = mg::countof(InputAssembler::vertexInputState);

  const auto pipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);
  return pipeline;
}

void renderSolidBox(const mg::RenderContext &renderContext, const glm::vec4 &position) {
  const auto texturePipeline = createSolidRendering(renderContext);
  using namespace mg::shaders::solid;

  using Ubo = UBO;
  using VertexInputData = InputAssembler::VertexInputData;
  using DescriptorSets = shaderResource::DescriptorSets;

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo = (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->mvp = glm::mat4(1);
  ubo->color = {1, 0, 0, 1};

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;

  glm::vec4 vertexInputData[4] = {
      {position.x, position.y, 0, 0},
      {position.x + position.z, position.y, 1, 0},
      {position.x + position.z, position.y + position.w, 1, 1},
      {position.x, position.y + position.w, 0, 1},
  };
  VkBuffer vertexBuffer;
  VkDeviceSize vertexBufferOffset;
  VertexInputData *vertices = (VertexInputData *)mg::mgSystem.linearHeapAllocator.allocateBuffer(
      mg::sizeofArrayInBytes(vertexInputData), &vertexBuffer, &vertexBufferOffset);
  memcpy(vertices, vertexInputData, sizeof(vertexInputData));

  constexpr uint32_t indicesInputData[6] = {0, 1, 2, 2, 3, 0};
  VkBuffer indexBuffer;
  VkDeviceSize indexBufferOffset;
  uint32_t *indices = (uint32_t *)mg::mgSystem.linearHeapAllocator.allocateBuffer(mg::sizeofArrayInBytes(indicesInputData),
                                                                                  &indexBuffer, &indexBufferOffset);
  memcpy(indices, indicesInputData, sizeof(indicesInputData));

  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, texturePipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, 1, &uniformOffset);
  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, texturePipeline.pipeline);

  vkCmdBindVertexBuffers(mg::vkContext.commandBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);
  vkCmdBindIndexBuffer(mg::vkContext.commandBuffer, indexBuffer, indexBufferOffset, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(mg::vkContext.commandBuffer, mg::countof(indicesInputData), 1, 0, 0, 0);
}

static mg::Pipeline createTextureRenderingPipeline(const mg::RenderContext &renderContext) {
  using namespace mg::shaders::textureRendering;

  const auto pipelineLayout = mg::vkContext.pipelineLayout;
  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.vkPipelineLayout = mg::getPipelineLayout(shaderResource::resources, mg::countof(shaderResource::resources));
  pipelineStateDesc.graphics.subpass = renderContext.subpass;
  pipelineStateDesc.depth.TestEnable = VK_FALSE;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = "textureRendering";
  createPipelineInfo.vertexInputState = InputAssembler::vertexInputState;
  createPipelineInfo.vertexInputStateCount = mg::countof(InputAssembler::vertexInputState);

  const auto pipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);
  return pipeline;
}

void renderBoxWithTexture(mg::RenderContext &renderContext, const glm::vec4 &position, const std::string &id) {
  const auto texturePipeline = createTextureRenderingPipeline(renderContext);
  using namespace mg::shaders::textureRendering;

  using Ubo = UBO;
  using VertexInputData = InputAssembler::VertexInputData;
  using DescriptorSets = shaderResource::DescriptorSets;

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo = (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->mvp = glm::mat4(1);

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.samplerImage = mg::getTexture(id).descriptorSet;

  glm::vec4 vertexInputData[4] = {
      {position.x, position.y, 0, 0},
      {position.x + position.z, position.y, 1, 0},
      {position.x + position.z, position.y + position.w, 1, 1},
      {position.x, position.y + position.w, 0, 1},
  };
  VkBuffer vertexBuffer;
  VkDeviceSize vertexBufferOffset;
  VertexInputData *vertices = (VertexInputData *)mg::mgSystem.linearHeapAllocator.allocateBuffer(
      mg::sizeofArrayInBytes(vertexInputData), &vertexBuffer, &vertexBufferOffset);
  memcpy(vertices, vertexInputData, sizeof(vertexInputData));

  constexpr uint32_t indicesInputData[6] = {0, 1, 2, 2, 3, 0};
  VkBuffer indexBuffer;
  VkDeviceSize indexBufferOffset;
  uint32_t *indices = (uint32_t *)mg::mgSystem.linearHeapAllocator.allocateBuffer(mg::sizeofArrayInBytes(indicesInputData),
                                                                                  &indexBuffer, &indexBufferOffset);
  memcpy(indices, indicesInputData, sizeof(indicesInputData));

  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, texturePipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, 1, &uniformOffset);
  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, texturePipeline.pipeline);

  vkCmdBindVertexBuffers(mg::vkContext.commandBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);
  vkCmdBindIndexBuffer(mg::vkContext.commandBuffer, indexBuffer, indexBufferOffset, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(mg::vkContext.commandBuffer, mg::countof(indicesInputData), 1, 0, 0, 0);
}

static mg::Pipeline createDepthTextureRenderingPipeline(const mg::RenderContext &renderContext) {
  using namespace mg::shaders::depth;

  const auto pipelineLayout = mg::vkContext.pipelineLayout;
  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.vkPipelineLayout = mg::getPipelineLayout(shaderResource::resources, mg::countof(shaderResource::resources));
  pipelineStateDesc.graphics.subpass = renderContext.subpass;
  pipelineStateDesc.depth.TestEnable = VK_FALSE;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = "depth";
  createPipelineInfo.vertexInputState = InputAssembler::vertexInputState;
  createPipelineInfo.vertexInputStateCount = mg::countof(InputAssembler::vertexInputState);

  const auto pipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);
  return pipeline;
}

void renderBoxWithDepthTexture(mg::RenderContext &renderContext, const glm::vec4 &position, const std::string &id) {
  const auto texturePipeline = createDepthTextureRenderingPipeline(renderContext);
  using namespace mg::shaders::depth;

  using Ubo = UBO;
  using VertexInputData = InputAssembler::VertexInputData;
  using DescriptorSets = shaderResource::DescriptorSets;

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo = (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->mvp = glm::mat4(1);
  ubo->nearAndFar = {0.1f, 1000.0f};

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.samplerDepth = mg::getTexture(id).descriptorSet;

  glm::vec4 vertexInputData[4] = {
      {position.x, position.y, 0, 0},
      {position.x + position.z, position.y, 1, 0},
      {position.x + position.z, position.y + position.w, 1, 1},
      {position.x, position.y + position.w, 0, 1},
  };
  VkBuffer vertexBuffer;
  VkDeviceSize vertexBufferOffset;
  VertexInputData *vertices = (VertexInputData *)mg::mgSystem.linearHeapAllocator.allocateBuffer(
      mg::sizeofArrayInBytes(vertexInputData), &vertexBuffer, &vertexBufferOffset);
  memcpy(vertices, vertexInputData, sizeof(vertexInputData));

  constexpr uint32_t indicesInputData[6] = {0, 1, 2, 2, 3, 0};
  VkBuffer indexBuffer;
  VkDeviceSize indexBufferOffset;
  uint32_t *indices = (uint32_t *)mg::mgSystem.linearHeapAllocator.allocateBuffer(mg::sizeofArrayInBytes(indicesInputData),
                                                                                  &indexBuffer, &indexBufferOffset);
  memcpy(indices, indicesInputData, sizeof(indicesInputData));

  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, texturePipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, 1, &uniformOffset);
  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, texturePipeline.pipeline);

  vkCmdBindVertexBuffers(mg::vkContext.commandBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);
  vkCmdBindIndexBuffer(mg::vkContext.commandBuffer, indexBuffer, indexBufferOffset, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(mg::vkContext.commandBuffer, mg::countof(indicesInputData), 1, 0, 0, 0);
}

} // namespace mg