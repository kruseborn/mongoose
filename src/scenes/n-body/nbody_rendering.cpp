#include "nbody_rendering.h"
#include "mg/camera.h"
#include "mg/mgSystem.h"
#include "mg/window.h"
#include "nbody_renderpass.h"
#include "nbody_utils.h"
#include "rendering/rendering.h"
#include <glm/gtc/matrix_transform.hpp>

static void transformVelocities(const ComputeData &computeData, const mg::FrameData &frameData) {
  using namespace mg::shaders::simulate_velocities;

  mg::ComputePipelineStateDesc computePipelineStateDesc = {};
  computePipelineStateDesc.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutStorage;
  computePipelineStateDesc.shaderName = "simulate_velocities.comp";

  const auto pipeline = mg::mgSystem.pipelineContainer.createComputePipeline(computePipelineStateDesc);

  auto storage = mg::mgSystem.storageContainer.getStorage(computeData.storageId);

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo = (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->dt = frameData.dt;

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.storage = storage.descriptorSet;

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets), dynamicOffsets);

  // transform velocity
  vkCmdDispatch(mg::vkContext.commandBuffer, computeData.nrOfParticles / computeData.workgroupSize, 1, 1);

  VkBufferMemoryBarrier vkBufferMemoryBarrier = {};
  vkBufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  vkBufferMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  vkBufferMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  vkBufferMemoryBarrier.buffer = storage.buffer;
  vkBufferMemoryBarrier.size = storage.size;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
                       0, nullptr, 1, &vkBufferMemoryBarrier, 0, nullptr);
}

void transformPositions(const ComputeData &computeData, const mg::FrameData &frameData) {
  using namespace mg::shaders::simulate_positions;

  mg::ComputePipelineStateDesc computePipelineStateDesc = {};
  computePipelineStateDesc.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutStorage;
  computePipelineStateDesc.shaderName = "simulate_positions.comp";

  const auto pipeline = mg::mgSystem.pipelineContainer.createComputePipeline(computePipelineStateDesc);

  auto storage = mg::mgSystem.storageContainer.getStorage(computeData.storageId);

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo = (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->dt = frameData.dt;

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.storage = storage.descriptorSet;

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets), dynamicOffsets);

  vkCmdDispatch(mg::vkContext.commandBuffer, computeData.nrOfParticles / computeData.workgroupSize, 1, 1);

  VkBufferMemoryBarrier vkBufferMemoryBarrier = {};
  vkBufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  vkBufferMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  vkBufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
  vkBufferMemoryBarrier.buffer = storage.buffer;
  vkBufferMemoryBarrier.size = storage.size;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
                       0, nullptr, 1, &vkBufferMemoryBarrier, 0, nullptr);
}

void simulatePartices(const ComputeData &computeData, const mg::FrameData &frameData) {
  transformVelocities(computeData, frameData);
  transformPositions(computeData, frameData);
}

void renderParticels(mg::RenderContext &renderContext, const ComputeData &computeData, const mg::Camera &camera) {
  using namespace mg::shaders::particle;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.vkPipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  pipelineStateDesc.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
  pipelineStateDesc.blend.colorBlendOp = VK_BLEND_OP_ADD;
  pipelineStateDesc.blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  pipelineStateDesc.blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;

  pipelineStateDesc.depth.TestEnable = VK_FALSE;
  pipelineStateDesc.depth.writeEnable = VK_FALSE;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = "particle";
  createPipelineInfo.vertexInputState = InputAssembler::vertexInputState;
  createPipelineInfo.vertexInputStateCount = mg::countof(InputAssembler::vertexInputState);

  const auto pipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);

  using VertexInputData = InputAssembler::VertexInputData;

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo = (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);

  glm::mat4 projectionMatrix, viewMatrix;

  projectionMatrix =
      glm::perspective(glm::radians(45.0f), mg::vkContext.screen.width / float(mg::vkContext.screen.height), 0.1f, 256.f);
  viewMatrix = glm::lookAt(camera.position, camera.aim, camera.up);

  ubo->modelview = viewMatrix;
  ubo->projection = projectionMatrix;
  ubo->screendim = glm::vec2{float(mg::vkContext.screen.width), float(mg::vkContext.screen.height)};

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.textures = mg::getTextureDescriptorSet();

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets), dynamicOffsets);

  TextureIndices textureIndices = {};
  textureIndices.textureIndex = mg::getTexture2DDescriptorIndex(computeData.particleId);

  vkCmdPushConstants(mg::vkContext.commandBuffer, pipeline.layout, VK_SHADER_STAGE_ALL, 0, sizeof(TextureIndices),
                     &textureIndices);

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

  const auto storageData = mg::getStorage(computeData.storageId);
  const auto count = storageData.size / (sizeof(glm::vec4) * 2);
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(mg::vkContext.commandBuffer, 0, 1, &storageData.buffer, &offset);
  vkCmdDraw(mg::vkContext.commandBuffer, count, 1, 0, 0);
}

static mg::Pipeline createToneMappingPipeline(const mg::RenderContext &renderContext) {
  using namespace mg::shaders::toneMapping;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.vkPipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  pipelineStateDesc.rasterization.cullMode = VK_CULL_MODE_NONE;
  pipelineStateDesc.graphics.subpass = renderContext.subpass;
  pipelineStateDesc.depth.TestEnable = VK_FALSE;
  pipelineStateDesc.depth.writeEnable = VK_FALSE;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = "toneMapping";

  return mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);
}

void renderToneMapping(const mg::RenderContext &renderContext, const NBodyRenderPass &nBodyRenderPass) {
  using namespace mg::shaders::toneMapping;

  const auto pipeline = createToneMappingPipeline(renderContext);

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo = (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->color = glm::vec4{0, 0, 0, 0};

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.textures = mg::getTextureDescriptorSet();

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets), dynamicOffsets);

  TextureIndices textureIndices = {};
  textureIndices.textureIndex = mg::getTexture2DDescriptorIndex(nBodyRenderPass.toneMapping);

  vkCmdPushConstants(mg::vkContext.commandBuffer, pipeline.layout, VK_SHADER_STAGE_ALL, 0, sizeof(TextureIndices),
                     &textureIndices);

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
  vkCmdDraw(mg::vkContext.commandBuffer, 3, 1, 0, 0);
}
