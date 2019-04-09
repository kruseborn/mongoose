#include "gltf_rendering.h"
#include "mg/camera.h"
#include "mg/mgSystem.h"
#include "mg/mgUtils.h"
#include "rendering/rendering.h"
#include "vulkan/pipelineContainer.h"
#include "vulkan/singleRenderPass.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void drawGltfMesh(const mg::RenderContext &renderContext, mg::MeshId meshId, const mg::Camera &camera) {
  using namespace mg::shaders::gltf;

  const auto pipelineLayout = mg::vkContext.pipelineLayout;
  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.vkPipelineLayout = mg::vkContext.pipelineLayoutMultiTexture;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = "gltf";
  createPipelineInfo.vertexInputState = InputAssembler::vertexInputState;
  createPipelineInfo.vertexInputStateCount = mg::countof(InputAssembler::vertexInputState);

  const auto pipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);

  using Ubo = UBO;
  using VertexInputData = InputAssembler::VertexInputData;
  using DescriptorSets = shaderResource::DescriptorSets;

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo = (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);

  glm::mat4 projectionMatrix, viewMatrix;

  projectionMatrix =
      glm::perspective(glm::radians(45.0f), mg::vkContext.screen.width / float(mg::vkContext.screen.height), 0.1f, 256.f);
  viewMatrix = glm::lookAt(camera.position, camera.aim, camera.up);

  ubo->model = glm::scale(glm::mat4(1), {-1, 1, 1});
  ubo->view = viewMatrix;
  ubo->projection = projectionMatrix;
  ubo->cameraPosition = glm::vec4{camera.position, 1.0f};

  const auto baseColorTexture = mg::getTexture("WaterBottle_baseColor.png");
  const auto normalTexture = mg::getTexture("WaterBottle_normal.png");
  const auto roughnessMetallic = mg::getTexture("WaterBottle_occlusionRoughnessMetallic.png");
  const auto emissive = mg::getTexture("WaterBottle_emissive.png");

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.baseColorTexture = baseColorTexture.descriptorSet;
  descriptorSets.normalTexture = normalTexture.descriptorSet;
  descriptorSets.roughnessMetallicTexture = roughnessMetallic.descriptorSet;
  descriptorSets.emissiveTexture = emissive.descriptorSet;

  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, 1, &uniformOffset);
  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

   const auto mesh = mg::getMesh(meshId);
   VkDeviceSize offset = 0;
   vkCmdBindVertexBuffers(mg::vkContext.commandBuffer, 0, 1, &mesh.buffer, &offset);
   vkCmdDraw(mg::vkContext.commandBuffer, mesh.indexCount, 1, 0, 0);
}
