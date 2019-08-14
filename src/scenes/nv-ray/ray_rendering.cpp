#include "ray_rendering.h"
#include "mg/camera.h"
#include "mg/geometryUtils.h"
#include "mg/mgSystem.h"
#include "mg/window.h"
#include "ray_renderpass.h"
#include "ray_utils.h"
#include "rendering/rendering.h"

namespace RAY {
enum tpye { GEN, MISS, CLOSEST_HIT };
}

static mg::Pipeline createRayPipeline() {
  using namespace mg::shaders::procedural;

  mg::RayTracingPipelineStateDesc rayTracingPipelineStateDesc = {};
  rayTracingPipelineStateDesc.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutRayTracing;
  rayTracingPipelineStateDesc.depth = 1;
  rayTracingPipelineStateDesc.shaderName = "procedural";

  const auto pipeline = mg::mgSystem.pipelineContainer.createRayTracingPipeline(rayTracingPipelineStateDesc);

  return pipeline;
}

void traceTriangle(const mg::RenderContext &renderContext, const RayInfo &rayInfo) {
  using namespace mg::shaders::procedural;

  auto pipeline = createRayPipeline();
  constexpr uint32_t groupCount = 3;
  const uint32_t groupSize = rayInfo.rayTracingProperties.shaderGroupHandleSize;
  const uint32_t bindingTableSize = groupSize * groupCount;

  VkBuffer bindingTableBuffer;
  VkDeviceSize bindingTableOffset;
  uint8_t *data = (uint8_t *)mg::mgSystem.linearHeapAllocator.allocateBuffer(bindingTableSize, &bindingTableBuffer,
                                                                             &bindingTableOffset);

  uint8_t *shaderHandleStorage = (uint8_t *)malloc(bindingTableSize);
  mg::checkResult(mg::nv::vkGetRayTracingShaderGroupHandlesNV(mg::vkContext.device, pipeline.pipeline, 0, groupCount,
                                                              bindingTableSize,
                                                              shaderHandleStorage));

  memcpy(data, (char *)shaderHandleStorage, bindingTableSize);

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->projInverse = glm::inverse(renderContext.projection);
  ubo->viewInverse = glm::inverse(renderContext.view);

  auto storageImage = mg::mgSystem.storageContainer.getStorage(rayInfo.storageImageId);

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.image = storageImage.descriptorSet;
  descriptorSets.topLevelAS = rayInfo.topLevelASDescriptorSet;

  uint32_t offsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(offsets), offsets);

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipeline.pipeline);

  // clang-format off
  mg::nv::vkCmdTraceRaysNV(mg::vkContext.commandBuffer, 
    bindingTableBuffer, bindingTableOffset, 
    bindingTableBuffer, bindingTableOffset + groupSize, groupSize, 
    bindingTableBuffer, bindingTableOffset + groupSize * 2, groupSize, VK_NULL_HANDLE, 0, 0, mg::vkContext.screen.width, mg::vkContext.screen.height, 1);
  // clang-format on
}


static mg::Pipeline createImageStoragePipeline(const mg::RenderContext &renderContext) {
  using namespace mg::shaders::imageStorage;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.vkPipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutRayTracing;
  pipelineStateDesc.rasterization.cullMode = VK_CULL_MODE_NONE;
  pipelineStateDesc.graphics.subpass = renderContext.subpass;
  pipelineStateDesc.depth.TestEnable = VK_FALSE;
  pipelineStateDesc.depth.writeEnable = VK_FALSE;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = "imageStorage";

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
