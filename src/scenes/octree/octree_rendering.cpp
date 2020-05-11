#include "octree_rendering.h"
#include "build_octree.h"

#include "mg/camera.h"
#include "mg/mgSystem.h"
#include "mg/window.h"
#include "rendering/rendering.h"
#include <glm/gtc/matrix_transform.hpp>

namespace mg {

void renderSdf(const mg::RenderContext &renderContext, Sdf sdf, const FrameData &frameData, const Camera &camera) {
  using namespace mg::shaders::sdf;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.rasterization.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.rasterization.vkPipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  pipelineStateDesc.rasterization.graphics.subpass = renderContext.subpass;
  pipelineStateDesc.rasterization.depth.TestEnable = VK_FALSE;
  pipelineStateDesc.rasterization.depth.writeEnable = VK_FALSE;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = shader;
  const auto pipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);
  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  auto scale = glm::scale(glm::mat4(1), {0.5f, 0.5f, 0.5f});
  auto transform = glm::translate(glm::mat4(1), {1.0f, 1.0f, 1.0f});

  ubo->worldToBox = scale * transform;
  ubo->worldToBox2 = transform * scale;

  ubo->info = {vkContext.screen.width, vkContext.screen.width, frameData.mouse.xy.x, frameData.mouse.xy.y};

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.textures = mg::getTextureDescriptorSet();
  descriptorSets.volumeTexture = mg::getTextureDescriptorSet3D();

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
  vkCmdDraw(mg::vkContext.commandBuffer, 3, 1, 0, 0);
}

void renderOctree(const OctreeStorages &storages, const mg::RenderContext &renderContext, uint32_t octreeLevel,
                  const mg::FrameData &frameData, glm::vec3 position) {
  using namespace mg::shaders::octreeTracer;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.rasterization.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.rasterization.vkPipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  pipelineStateDesc.rasterization.graphics.subpass = renderContext.subpass;
  pipelineStateDesc.rasterization.depth.TestEnable = VK_FALSE;
  pipelineStateDesc.rasterization.depth.writeEnable = VK_FALSE;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = shader;
  const auto pipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);
  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);

  ubo->screenSize = {vkContext.screen.width, vkContext.screen.height, 0, 0};
  ubo->uPosition = {position.x, position.y, position.z, 1};
  ubo->uProjection = renderContext.projection;
  ubo->uView = renderContext.view;

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.uOctree = mg::mgSystem.storageContainer.getStorage(storages.octree).descriptorSet;
  descriptorSets.textures = mg::getTextureDescriptorSet();
  descriptorSets.volumeTexture = mg::getTextureDescriptorSet3D();

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
  vkCmdDraw(mg::vkContext.commandBuffer, 3, 1, 0, 0);
}

void renderVoxels(const RenderContext &renderContext, std::vector<glm::uvec2> vec, const mg::MeshId &cubeId) {
  const auto mesh = mg::getMesh(cubeId);

  using namespace mg::shaders::cubes;
  namespace ia = mg::shaders::cubes::InputAssembler;
  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.rasterization.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.rasterization.vkPipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  pipelineStateDesc.rasterization.graphics.subpass = renderContext.subpass;
  pipelineStateDesc.rasterization.rasterization.cullMode = VK_CULL_MODE_NONE;
  pipelineStateDesc.rasterization.rasterization.polygonMode = VK_POLYGON_MODE_LINE;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = shader;
  createPipelineInfo.vertexInputState = InputAssembler::vertexInputState;
  createPipelineInfo.vertexInputStateCount = mg::countof(InputAssembler::vertexInputState);

  const auto pipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);

  VkBuffer instanceBuffer;
  VkDeviceSize instanceBufferOffset;
  ia::InstanceInputData *instance = (ia::InstanceInputData *)mg::mgSystem.linearHeapAllocator.allocateBuffer(
      (vec.size() - 1) * sizeof(ia::InstanceInputData), &instanceBuffer, &instanceBufferOffset);

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);

  ubo->mvp = renderContext.projection * renderContext.view;
  ubo->color = {1, 0, 0, 1};
  DescriptorSets descriptorSets = {.ubo = uboSet};

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  for (uint32_t i = 1; i < vec.size(); i++) {
    glm::vec3 position = {};
    glm::uvec3 v = {vec[i].x & 0xfffu, (vec[i].x >> 12u) & 0xfffu, (vec[i].x >> 24u) | ((vec[i].y >> 28u) << 8u)};

    position.x = v.x * 0.5f;
    position.y = v.y * 0.5f;
    position.z = v.z * 0.5f;
    instance[i - 1].instancing_translate = position;
  }

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
  vkCmdBindVertexBuffers(mg::vkContext.commandBuffer, mg::shaders::INSTANCE_BINDING_ID, 1, &instanceBuffer,
                         &instanceBufferOffset);
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(mg::vkContext.commandBuffer, mg::shaders::VERTEX_BINDING_ID, 1, &mesh.buffer, &offset);
  vkCmdBindIndexBuffer(mg::vkContext.commandBuffer, mesh.buffer, mesh.indicesOffset, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(mg::vkContext.commandBuffer, mesh.indexCount, uint32_t(vec.size() - 1), 0, 0, 0);
}

void renderMenu(const mg::FrameData &frameData, void *data) {
  Menu *menu = (Menu *)data;

  ImGuiIO &io = ImGui::GetIO();

  io.DisplaySize = {float(mg::vkContext.screen.width), float(mg::vkContext.screen.height)};
  io.DeltaTime = 1.0f / 60.0f;

  io.MousePos = ImVec2(frameData.mouse.xy.x * mg::vkContext.screen.width,
                       (1.0f - frameData.mouse.xy.y) * mg::vkContext.screen.height);
  io.MouseDown[0] = frameData.mouse.left;

  ImGui::NewFrame();

  if (ImGui::Begin("Octree", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::RadioButton("radio a", &menu->value, 0);
    ImGui::SameLine();
    ImGui::RadioButton("radio b", &menu->value, 1);
    ImGui::SameLine();
    ImGui::RadioButton("radio c", &menu->value, 2);
  }
  ImGui::End();
  ImGui::EndFrame();
  ImGui::Render();
}

} // namespace mg