#include "navier_stoke.h"

#include "mg/mgSystem.h"
#include "mg/window.h"
#include "rendering/rendering.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

// incrompressible fluid: water
// compressible fluid: air

namespace mg {

static size_t gridSize(size_t N) { return (N + 2) * (N + 2); }

static void diffuse(int32_t N, int32_t b, mg::StorageId x, mg::StorageId x0, float diff, float dt) {
  using namespace mg::shaders::diffuse;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.compute.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutStorage;

  const auto pipeline = mg::mgSystem.pipelineContainer.createComputePipeline(pipelineStateDesc, {.shaderName = shader});

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->dt = dt;
  ubo->N = N;
  ubo->diff = diff;
  ubo->b = b;

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.x = mg::mgSystem.storageContainer.getStorage(x).descriptorSet;
  descriptorSets.x0 = mg::mgSystem.storageContainer.getStorage(x0).descriptorSet;

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  vkCmdDispatch(mg::vkContext.commandBuffer, uint32_t(gridSize(N)) / 256, 1, 1);

  // setup a memory barrier, device memory has to be finished with writing when reading occurs on
  // the host
  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

static void advect(int32_t N, int32_t b, mg::StorageId d, mg::StorageId d0, mg::StorageId u, mg::StorageId v, float dt) {
  using namespace mg::shaders::advec;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.compute.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutStorage;

  const auto pipeline = mg::mgSystem.pipelineContainer.createComputePipeline(pipelineStateDesc, {.shaderName = shader});

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->dt = dt;
  ubo->N = N;
  ubo->b = b;

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.u = mg::mgSystem.storageContainer.getStorage(u).descriptorSet;
  descriptorSets.v = mg::mgSystem.storageContainer.getStorage(v).descriptorSet;
  descriptorSets.d = mg::mgSystem.storageContainer.getStorage(d).descriptorSet;
  descriptorSets.d0 = mg::mgSystem.storageContainer.getStorage(d0).descriptorSet;

  vkCmdBindPipeline(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  vkCmdDispatch(vkContext.commandBuffer, uint32_t(gridSize(N)) / 256, 1, 1);

  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

static void preProjectCompute(int32_t N, mg::StorageId u, mg::StorageId v, mg::StorageId p, mg::StorageId div) {
  using namespace mg::shaders::preProject;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.compute.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutStorage;

  const auto pipeline = mg::mgSystem.pipelineContainer.createComputePipeline(pipelineStateDesc, {.shaderName = shader});

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->N = N;

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.u = mg::mgSystem.storageContainer.getStorage(u).descriptorSet;
  descriptorSets.v = mg::mgSystem.storageContainer.getStorage(v).descriptorSet;
  descriptorSets.p = mg::mgSystem.storageContainer.getStorage(p).descriptorSet;
  descriptorSets.div = mg::mgSystem.storageContainer.getStorage(div).descriptorSet;

  vkCmdBindPipeline(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  vkCmdDispatch(vkContext.commandBuffer, uint32_t(gridSize(N)) / 256, 1, 1);

  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}
static void projectCompute(int32_t N, mg::StorageId u, mg::StorageId v, mg::StorageId p, mg::StorageId div) {
  using namespace mg::shaders::project;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.compute.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutStorage;

  const auto pipeline = mg::mgSystem.pipelineContainer.createComputePipeline(pipelineStateDesc, {.shaderName = shader});

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->N = N;

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.u = mg::mgSystem.storageContainer.getStorage(u).descriptorSet;
  descriptorSets.v = mg::mgSystem.storageContainer.getStorage(v).descriptorSet;
  descriptorSets.p = mg::mgSystem.storageContainer.getStorage(p).descriptorSet;
  descriptorSets.div = mg::mgSystem.storageContainer.getStorage(div).descriptorSet;

  vkCmdBindPipeline(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  vkCmdDispatch(vkContext.commandBuffer, uint32_t(gridSize(N)) / 256, 1, 1);

  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

static void postProjectCompute(int32_t N, mg::StorageId u, mg::StorageId v, mg::StorageId p, mg::StorageId div) {
  using namespace mg::shaders::postProject;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.compute.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutStorage;

  const auto pipeline = mg::mgSystem.pipelineContainer.createComputePipeline(pipelineStateDesc, {.shaderName = shader});

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->N = N;

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.u = mg::mgSystem.storageContainer.getStorage(u).descriptorSet;
  descriptorSets.v = mg::mgSystem.storageContainer.getStorage(v).descriptorSet;
  descriptorSets.p = mg::mgSystem.storageContainer.getStorage(p).descriptorSet;
  descriptorSets.div = mg::mgSystem.storageContainer.getStorage(div).descriptorSet;

  vkCmdBindPipeline(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  vkCmdDispatch(vkContext.commandBuffer, uint32_t(gridSize(N)) / 256, 1, 1);

  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

static void project(int32_t N, mg::StorageId u, mg::StorageId v, mg::StorageId p, mg::StorageId div) {
  preProjectCompute(N, u, v, p, div);
  projectCompute(N, u, v, p, div);
  postProjectCompute(N, u, v, p, div);
}

static void step(int32_t N, mg::StorageId u, mg::StorageId v, mg::StorageId u0, mg::StorageId v0, mg::StorageId d,
                 mg::StorageId s, float visc, float dt) {
  diffuse(N, 1, u0, u, visc, dt);
  diffuse(N, 2, v0, v, visc, dt);

  project(N, u0, v0, u, v);

  advect(N, 1, u, u0, u0, v0, dt);
  advect(N, 2, v, v0, u0, v0, dt);

  project(N, u, v, u0, v0);
  diffuse(N, 0, s, d, 0, dt);
  advect(N, 0, d, s, u, v, dt);

  // barrier for fragment shader to read compute shader output
  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

static void updateFromGui(int32_t N, mg::StorageId d, mg::StorageId u, mg::StorageId v, const mg::FrameData &frameData) {
  int32_t i = int(frameData.mouse.xy.x * N);
  int32_t j = int((1.0f - frameData.mouse.xy.y) * N);
  auto delta = frameData.mouse.xy - frameData.mouse.prevXY;

  using namespace mg::shaders::addSource;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.compute.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutStorage;

  const auto pipeline = mg::mgSystem.pipelineContainer.createComputePipeline(pipelineStateDesc, {.shaderName = shader});

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->N = N;
  ubo->i = i;
  ubo->j = j;
  ubo->delta = delta;

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.u = mg::mgSystem.storageContainer.getStorage(u).descriptorSet;
  descriptorSets.v = mg::mgSystem.storageContainer.getStorage(v).descriptorSet;
  descriptorSets.d = mg::mgSystem.storageContainer.getStorage(d).descriptorSet;

  vkCmdBindPipeline(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  vkCmdDispatch(vkContext.commandBuffer, 1, 1, 1);

  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

void simulateNavierStoke(const Storages &storages, const mg::FrameData &frameData, uint32_t N) {
  const float dt = 0.1f;
  if (frameData.mouse.left)
    updateFromGui(N, storages.d, storages.u, storages.v, frameData);
  step(N, storages.u, storages.v, storages.u0, storages.v0, storages.d, storages.s, 0, dt);
}

void renderNavierStoke(const mg::RenderContext &renderContext, const Storages &storages) {
  renderFluid(renderContext, storages.d);
}

Storages createStorages(size_t N) {
  std::vector<float> empty(gridSize(N));
  const auto sizeInBytes = mg::sizeofContainerInBytes(empty);

  return {.u = mg::mgSystem.storageContainer.createStorage(empty.data(), sizeInBytes),
          .v = mg::mgSystem.storageContainer.createStorage(empty.data(), sizeInBytes),
          .u0 = mg::mgSystem.storageContainer.createStorage(empty.data(), sizeInBytes),
          .v0 = mg::mgSystem.storageContainer.createStorage(empty.data(), sizeInBytes),
          .d = mg::mgSystem.storageContainer.createStorage(empty.data(), sizeInBytes),
          .s = mg::mgSystem.storageContainer.createStorage(empty.data(), sizeInBytes)};
}

void destroyStorages(Storages *storages) {
  mg::mgSystem.storageContainer.removeStorage(storages->u);
  mg::mgSystem.storageContainer.removeStorage(storages->v);
  mg::mgSystem.storageContainer.removeStorage(storages->u0);
  mg::mgSystem.storageContainer.removeStorage(storages->v0);
  mg::mgSystem.storageContainer.removeStorage(storages->d);
  mg::mgSystem.storageContainer.removeStorage(storages->s);
  *storages = {};
}

} // namespace mg