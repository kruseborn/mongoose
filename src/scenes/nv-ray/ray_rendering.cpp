#include "ray_rendering.h"
#include "mg/camera.h"
#include "mg/geometryUtils.h"
#include "mg/mgSystem.h"
#include "mg/window.h"
#include "ray_utils.h"
#include "rendering/rendering.h"
#include "vulkan/shaders.h"

static mg::Pipeline createRayPipeline() {
  using namespace mg::shaders::procedural;

  mg::PipelineStateDesc rayTracingPipelineStateDesc = {};
  rayTracingPipelineStateDesc.rayTracing.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutRayTracing;
  rayTracingPipelineStateDesc.rayTracing.depth = 1;

  auto &shaders = rayTracingPipelineStateDesc.rayTracing.shaders;
  auto &groups = rayTracingPipelineStateDesc.rayTracing.groups;

  strcpy(shaders[0], files.procedural_rgen);
  strcpy(shaders[1], files.procedural_rmiss);
  strcpy(shaders[2], files.procedural_proc_rint);
  strcpy(shaders[3], files.procedural_lambert_proc_rchit);
  strcpy(shaders[4], files.procedural_metal_proc_rchit);
  strcpy(shaders[5], files.procedural_dielectrics_proc_rchit);
  rayTracingPipelineStateDesc.rayTracing.shaderCount = 6;

  groups[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
  groups[0].generalShader = 0;
  groups[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
  groups[1].generalShader = 1;
  groups[2].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
  groups[2].intersectionShader = 2;
  groups[2].closestHitShader = 3;
  groups[3].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
  groups[3].intersectionShader = 2;
  groups[3].closestHitShader = 4;
  groups[4].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
  groups[4].intersectionShader = 2;
  groups[4].closestHitShader = 5;
  rayTracingPipelineStateDesc.rayTracing.groupCount = 5;

  const auto pipeline =
      mg::mgSystem.pipelineContainer.createRayTracingPipeline(rayTracingPipelineStateDesc, {.shaderName = shader});
  return pipeline;
}

void traceTriangle(const World &world, const mg::Camera &camera, const mg::RenderContext &renderContext,
                   const RayInfo &rayInfo) {
  using namespace mg::shaders::procedural;

  auto pipeline = createRayPipeline();
  constexpr uint32_t groupCount = 5; // groupCount is the number of shader handles to retrieve
  const uint32_t shaderGroupHandleSize = rayInfo.rayTracingProperties.shaderGroupHandleSize;
  const uint32_t bindingTableSize =
      shaderGroupHandleSize * groupCount; // dataSize must be at least
                                          // VkPhysicalDeviceRayTracingPropertiesNV::shaderGroupHandleSize × groupCount

  VkBuffer bindingTableBuffer;
  VkDeviceSize bindingTableOffset;
  uint8_t *data = (uint8_t *)mg::mgSystem.linearHeapAllocator.allocateBuffer(bindingTableSize, &bindingTableBuffer,
                                                                             &bindingTableOffset);

  mg::checkResult(mg::nv::vkGetRayTracingShaderGroupHandlesNV(mg::vkContext.device, pipeline.pipeline, 0, groupCount,
                                                              bindingTableSize, data));

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);

  VkBuffer storageBuffer;
  uint32_t storageOffset;
  VkDescriptorSet storageSet;

  Storage::StorageData *storage = (Storage::StorageData *)mg::mgSystem.linearHeapAllocator.allocateStorage(
      sizeof(storage) * world.positions.size(), &storageBuffer, &storageOffset, &storageSet);
  mgAssert(mg::sizeofContainerInBytes(world.positions) <= sizeof(storage->positions));
  memcpy(storage->positions, world.positions.data(), mg::sizeofContainerInBytes(world.positions));
  memcpy(storage->albedos, world.albedos.data(), mg::sizeofContainerInBytes(world.positions));

  ubo->projInverse = glm::inverse(renderContext.projection);
  ubo->viewInverse = glm::inverse(renderContext.view);
  static uint32_t frame = 0;
  static uint32_t totalNrOfSamples = 0;
  static const uint32_t nrOfSamplesPerFrame = 10;
  totalNrOfSamples += nrOfSamplesPerFrame;
  if (rayInfo.resetAccumulationImage)
    totalNrOfSamples = nrOfSamplesPerFrame;

  ubo->attrib = {++frame, world.blueNoise.index, nrOfSamplesPerFrame, totalNrOfSamples};
  ubo->sobolId = {float(rayInfo.sobolId.index),0,0,0};
  ubo->cameraPosition = {camera.position.x, camera.position.y, camera.position.z,
                         rayInfo.resetAccumulationImage ? 1.0f : 0.0f};
  ubo->cameraLookat = {camera.aim.x, camera.aim.y, camera.aim.z, rayInfo.resetAccumulationImage ? 1.0f : 0.0f};

  auto storageImage = mg::mgSystem.storageContainer.getStorage(rayInfo.storageImageId);
  auto storageAccumulationImage = mg::mgSystem.storageContainer.getStorage(rayInfo.storageAccumulationImageID);

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.image = storageImage.descriptorSet;
  descriptorSets.topLevelAS = rayInfo.topLevelASDescriptorSet;
  descriptorSets.textures = mg::getTextureDescriptorSet();
  descriptorSets.accumulationImage = storageAccumulationImage.descriptorSet;

  uint32_t offsets[] = {uniformOffset, storageOffset};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(offsets), offsets);

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipeline.pipeline);

  // clang-format off
  mg::nv::vkCmdTraceRaysNV(mg::vkContext.commandBuffer, 
    bindingTableBuffer, bindingTableOffset, // raygenShader
    bindingTableBuffer, bindingTableOffset + shaderGroupHandleSize, shaderGroupHandleSize, //missShader
    bindingTableBuffer, bindingTableOffset + shaderGroupHandleSize * 2, shaderGroupHandleSize, // hitShader
    VK_NULL_HANDLE, 0, 0, mg::vkContext.screen.width, mg::vkContext.screen.height, 1);
  // clang-format on
}

static mg::Pipeline createImageStoragePipeline(const mg::RenderContext &renderContext) {
  using namespace mg::shaders::imageStorage;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.rasterization.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.rasterization.vkPipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutRayTracing;
  pipelineStateDesc.rasterization.rasterization.cullMode = VK_CULL_MODE_NONE;
  pipelineStateDesc.rasterization.graphics.subpass = renderContext.subpass;
  pipelineStateDesc.rasterization.depth.TestEnable = VK_TRUE;
  pipelineStateDesc.rasterization.depth.writeEnable = VK_FALSE;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = shader;

  const auto pipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);
  return pipeline;
}

void drawImageStorage(const mg::RenderContext &renderContext, const RayInfo &rayInfo) {
  using namespace mg::shaders::imageStorage;

  const auto pipeline = createImageStoragePipeline(renderContext);

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *dynamic =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  dynamic->temp = glm::vec4{1, 0, 0, 1};

  auto storageImage = mg::mgSystem.storageContainer.getStorage(rayInfo.storageImageId);

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.image = storageImage.descriptorSet;

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
  vkCmdDraw(mg::vkContext.commandBuffer, 3, 1, 0, 0);
}
