#include "deferred_rendering.h"

#include "deferred_renderpass.h"
#include "deferred_utils.h"
#include "mg/camera.h"
#include "mg/meshLoader.h"
#include "mg/mgSystem.h"
#include "rendering/rendering.h"
#include "vulkan/pipelineContainer.h"
#include "vulkan/vkContext.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>

static mg::Pipeline createMRTPipeline(const mg::RenderContext &renderContext) {
  using namespace mg::shaders::mrt;

  const auto pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.vkPipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  pipelineStateDesc.rasterization.cullMode = VK_CULL_MODE_FRONT_BIT;
  pipelineStateDesc.graphics.subpass = renderContext.subpass;
  pipelineStateDesc.graphics.nrOfColorAttachments = 3;
  pipelineStateDesc.blend.blendEnable = VK_FALSE;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = "mrt";
  createPipelineInfo.vertexInputState = InputAssembler::vertexInputState;
  createPipelineInfo.vertexInputStateCount = mg::countof(InputAssembler::vertexInputState);

  const auto mrtPipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);
  return mrtPipeline;
}

void renderMRT(const mg::RenderContext &renderContext, const mg::ObjMeshes &objMeshes) {
  using namespace mg::shaders::mrt;

  using VertexInputData = InputAssembler::VertexInputData;

  const auto mrtPipeline = createMRTPipeline(renderContext);

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *dynamic = (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);

  dynamic->projection = renderContext.projection;
  dynamic->view = renderContext.view;
  dynamic->model = glm::mat4(1);
  dynamic->mNormal = glm::mat4(glm::transpose(glm::inverse(glm::mat3(renderContext.view))));

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mrtPipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets), dynamicOffsets);
  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mrtPipeline.pipeline);

  for (uint32_t i = 0; i < objMeshes.meshes.size(); i++) {
    const auto mesh = mg::getMesh(objMeshes.meshes[i].id);
    mgAssert(objMeshes.meshes[i].materialId < objMeshes.materials.size());
    const auto material = objMeshes.materials[objMeshes.meshes[i].materialId];

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(mg::vkContext.commandBuffer, 0, 1, &mesh.buffer, &offset);
    vkCmdPushConstants(mg::vkContext.commandBuffer, mrtPipeline.layout, VK_SHADER_STAGE_ALL, 0, sizeof(material.diffuse),
                       (void *)&material.diffuse);
    vkCmdDraw(mg::vkContext.commandBuffer, mesh.indexCount, 1, 0, 0);
  }
}

static mg::Pipeline createSSAOPipeline(const mg::RenderContext &renderContext) {
  using namespace mg::shaders::ssao;

  const auto pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.vkPipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  pipelineStateDesc.graphics.subpass = renderContext.subpass;
  pipelineStateDesc.rasterization.cullMode = VK_CULL_MODE_BACK_BIT;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = "ssao";

  const auto ssaoPipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);
  return ssaoPipeline;
}

void renderSSAO(const mg::RenderContext &renderContext, const DeferredRenderPass &deferredRenderPass, const Noise &noise) {
  using namespace mg::shaders::ssao;
  
  const auto ssaoPipeline = createSSAOPipeline(renderContext);

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *dynamic = (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);

  std::memcpy(dynamic->kernel, noise.ssaoKernel.kernel, mg::sizeofArrayInBytes(noise.ssaoKernel.kernel));
  dynamic->projection = renderContext.projection;
  dynamic->noiseScale = noise.ssaoKernel.noiseScale;

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.textures = mg::getTextureDescriptorSet();

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ssaoPipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets), dynamicOffsets);

  TextureIndices textureIndices = {};
  textureIndices.normalIndex = mg::getTexture2DDescriptorIndex(deferredRenderPass.normal);
  textureIndices.wordViewPositionIndex = mg::getTexture2DDescriptorIndex(deferredRenderPass.worldViewPosition);
  textureIndices.noiseIndex = mg::getTexture2DDescriptorIndex(noise.noiseTexture);
  vkCmdPushConstants(mg::vkContext.commandBuffer, ssaoPipeline.layout, VK_SHADER_STAGE_ALL, 0, sizeof(TextureIndices),
                     &textureIndices);

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ssaoPipeline.pipeline);
  vkCmdDraw(mg::vkContext.commandBuffer, 3, 1, 0, 0);
}

static mg::Pipeline createSSAOBlurPipeline(const mg::RenderContext &renderContext) {
  using namespace mg::shaders::ssaoBlur;

  const auto pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.vkPipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  pipelineStateDesc.graphics.subpass = renderContext.subpass;
  pipelineStateDesc.rasterization.cullMode = VK_CULL_MODE_NONE;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = "ssaoBlur";

  const auto pipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);
  return pipeline;
}

void renderBlurSSAO(const mg::RenderContext &renderContext, const DeferredRenderPass &deferredRenderPass) {
  using namespace mg::shaders::ssaoBlur;
  auto ssaoBlurPipeline = createSSAOBlurPipeline(renderContext);

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *dynamic = (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  dynamic->size = glm::vec2(mg::vkContext.screen.width, mg::vkContext.screen.height);

  const auto ssaoTexture = mg::getTexture(deferredRenderPass.ssao);

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.textures = mg::getTextureDescriptorSet();

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ssaoBlurPipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets), dynamicOffsets);

  TextureIndices textureIndices = {};
  textureIndices.ssaoIndex = mg::getTexture2DDescriptorIndex(deferredRenderPass.ssao);
  vkCmdPushConstants(mg::vkContext.commandBuffer, ssaoBlurPipeline.layout, VK_SHADER_STAGE_ALL, 0,
                     sizeof(TextureIndices),
                     &textureIndices);

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ssaoBlurPipeline.pipeline);
  vkCmdDraw(mg::vkContext.commandBuffer, 3, 1, 0, 0);
}

static mg::Pipeline createFinalDeferred(const mg::RenderContext &renderContext) {
  using namespace mg::shaders::final;

  const auto pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.vkPipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  pipelineStateDesc.graphics.subpass = renderContext.subpass;
  pipelineStateDesc.rasterization.cullMode = VK_CULL_MODE_NONE;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = "final";

  const auto pipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);
  return pipeline;
}

static void setLightPositions(mg::shaders::final::Ubo *dynamic) {
  std::uniform_real_distribution<float> x(-300.0, 300.0);
  std::uniform_real_distribution<float> y(40, 60);
  std::uniform_real_distribution<float> z(-200, 200);

  std::uniform_real_distribution<float> color(0.0, 1.0);
  std::uniform_real_distribution<float> radius(1000, 3000);

  std::default_random_engine random(101);

  for (uint32_t i = 0; i < 32; i++) {
    dynamic->lights[i].position = glm::vec4(x(random), y(random), z(random), 1.0f);
    dynamic->lights[i].color = glm::vec4(color(random), color(random), color(random), radius((random)));
  }
}

void renderFinalDeferred(const mg::RenderContext &renderContext, const DeferredRenderPass &deferredRenderPass) {
  using namespace mg::shaders::final;

  const auto deferredPipeline = createFinalDeferred(renderContext);

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *dynamic = (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);

  dynamic->view = renderContext.view;
  dynamic->projection = renderContext.projection;
  setLightPositions(dynamic);

  const auto normalTexture = mg::getTexture(deferredRenderPass.normal);
  const auto albedoTexture = mg::getTexture(deferredRenderPass.albedo);
  const auto ssaoBlured = mg::getTexture(deferredRenderPass.ssaoBlur);
  const auto wordViewPosition = mg::getTexture(deferredRenderPass.worldViewPosition);

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.textures = mg::getTextureDescriptorSet();

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredPipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets), dynamicOffsets);

  TextureIndices textureIndices = {};
  textureIndices.diffuseIndex = mg::getTexture2DDescriptorIndex(deferredRenderPass.albedo);
  textureIndices.normalIndex = mg::getTexture2DDescriptorIndex(deferredRenderPass.normal);
  textureIndices.ssaoBluredIndex = mg::getTexture2DDescriptorIndex(deferredRenderPass.ssaoBlur);
  textureIndices.worldViewPostionIndex = mg::getTexture2DDescriptorIndex(deferredRenderPass.worldViewPosition);

  vkCmdPushConstants(mg::vkContext.commandBuffer, deferredPipeline.layout, VK_SHADER_STAGE_ALL, 0,
                     sizeof(TextureIndices), &textureIndices);

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredPipeline.pipeline);
  vkCmdDraw(mg::vkContext.commandBuffer, 3, 1, 0, 0);
}
