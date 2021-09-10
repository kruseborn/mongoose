#include "boids_rendering.h"

#include "mg/camera.h"
#include "mg/mgSystem.h"
#include "mg/window.h"
#include "rendering/rendering.h"
#include "vulkan/shaderPipelineInput.h"
#include <glm/gtc/matrix_transform.hpp>

namespace mg {

void renderCubes(const RenderContext &renderContext, const FrameData &frameData, std::span<glm::vec3> positions,
                 const mg::MeshId &cubeId) {
  const auto mesh = mg::getMesh(cubeId);

  using namespace mg::shaders::cubes;
  namespace ia = mg::shaders::cubes::InputAssembler;
  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.rasterization.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.rasterization.vkPipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  pipelineStateDesc.rasterization.graphics.subpass = renderContext.subpass;
  pipelineStateDesc.rasterization.rasterization.cullMode = VK_CULL_MODE_NONE;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = shader;
  createPipelineInfo.vertexInputState = InputAssembler::vertexInputState;
  createPipelineInfo.vertexInputStateCount = mg::countof(InputAssembler::vertexInputState);

  const auto pipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);

  VkBuffer instanceBuffer;
  VkDeviceSize instanceBufferOffset;
  ia::InstanceInputData *instance = (ia::InstanceInputData *)mg::mgSystem.linearHeapAllocator.allocateBuffer(
      positions.size() * sizeof(ia::InstanceInputData), &instanceBuffer, &instanceBufferOffset);

  memcpy(instance, positions.data(), positions.size() * sizeof(ia::InstanceInputData));

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);

  // static float rotAngle = 0;
  // rotAngle += frameData.dt * 20.0f;
  // glm::mat4 rotation = glm::rotate(glm::mat4(1), rotAngle, {1, 1, 0});

  ubo->mvp = renderContext.projection * renderContext.view;
  ubo->color = {0.6f, 0, 0, 1};
  DescriptorSets descriptorSets = {.ubo = uboSet};

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
  vkCmdBindVertexBuffers(mg::vkContext.commandBuffer, mg::shaders::INSTANCE_BINDING_ID, 1, &instanceBuffer,
                         &instanceBufferOffset);
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(mg::vkContext.commandBuffer, mg::shaders::VERTEX_BINDING_ID, 1, &mesh.buffer, &offset);
  vkCmdBindIndexBuffer(mg::vkContext.commandBuffer, mesh.buffer, mesh.indicesOffset, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(mg::vkContext.commandBuffer, mesh.indexCount, uint32_t(positions.size()), 0, 0, 0);
}

} // namespace mg