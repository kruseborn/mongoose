#include "navier_stroke.h"

#include "mg/mgSystem.h"
#include "mg/window.h"
#include "rendering/rendering.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

// incrompressible fluid: water
// compressible fluid: air

namespace mg {

void fullBarrier() {
  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask =
      VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
      VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
      VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
      VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;

  memoryBarrier.dstAccessMask =
      VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
      VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
      VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
      VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                       VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

void endAndBegin() {
  return;
  fullBarrier();
  return;
  // create and reset fence
  VkFence fence;
  VkFenceCreateInfo vkFenceCreateInfo = {};
  vkFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  vkFenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  checkResult(vkCreateFence(vkContext.device, &vkFenceCreateInfo, nullptr, &fence));
  checkResult(vkResetFences(vkContext.device, 1, &fence));

  vkEndCommandBuffer(mg::vkContext.commandBuffer);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &mg::vkContext.commandBuffer;

  // fences are GPU to CPU syncs, CPU waits for GPU to finish it's current work on the queue for a
  // specific command buffer
  checkResult(vkQueueSubmit(vkContext.queue, 1, &submitInfo, fence));
  checkResult(vkWaitForFences(vkContext.device, 1, &fence, VK_TRUE, UINT64_MAX));

  VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {};
  vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  checkResult(vkBeginCommandBuffer(vkContext.commandBuffer, &vkCommandBufferBeginInfo));
}

static constexpr uint32_t size = (128 + 2) * (128 + 2);
FluidTimes fluidTimes = {};

Storages createStorages(size_t N) {
  std::vector<float> empty((N + 2) * (N + 2));
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

void diffuseCompute(int N, int b, mg::StorageId x, mg::StorageId x0, float diff, float dt) {
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

  vkCmdDispatch(mg::vkContext.commandBuffer, size, 1, 1);

  // setup a memory barrier, device memory has to be finished with writing when reading occurs on
  // the host
  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

void advectCompute(int N, int b, mg::StorageId d, mg::StorageId d0, mg::StorageId u, mg::StorageId v, float dt) {
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

  vkCmdDispatch(vkContext.commandBuffer, size, 1, 1);

  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

void preProjectCompute(int N, mg::StorageId u, mg::StorageId v, mg::StorageId p, mg::StorageId div) {
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

  vkCmdDispatch(vkContext.commandBuffer, size, 1, 1);

  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}
void projectCompute(int N, mg::StorageId u, mg::StorageId v, mg::StorageId p, mg::StorageId div) {
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

  vkCmdDispatch(vkContext.commandBuffer, size, 1, 1);

  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

void postProjectCompute(int N, mg::StorageId u, mg::StorageId v, mg::StorageId p, mg::StorageId div) {
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

  vkCmdDispatch(vkContext.commandBuffer, size, 1, 1);

  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

void project_C(int N, mg::StorageId u, mg::StorageId v, mg::StorageId p, mg::StorageId div) {
  preProjectCompute(N, u, v, p, div);
  endAndBegin();
  projectCompute(N, u, v, p, div);
  endAndBegin();
  postProjectCompute(N, u, v, p, div);
  endAndBegin();
}

void vel_step_c(int N, mg::StorageId u, mg::StorageId v, mg::StorageId u0, mg::StorageId v0, mg::StorageId d,
                mg::StorageId s, float visc, float dt) {
  {
    auto start = mg::timer::now();
    endAndBegin();
    diffuseCompute(N, 1, u0, u, visc, dt);
    endAndBegin();
    diffuseCompute(N, 2, v0, v, visc, dt);
    endAndBegin();
    auto end = mg::timer::now();
    fluidTimes.diffuseSum += uint32_t(mg::timer::durationInMs(start, end));
  }

  {
    auto start = mg::timer::now();
    project_C(N, u0, v0, u, v);
    endAndBegin();
    auto end = mg::timer::now();
    fluidTimes.projectSum += uint32_t(mg::timer::durationInMs(start, end));
  }

  {
    auto start = mg::timer::now();

    advectCompute(N, 1, u, u0, u0, v0, dt);
    endAndBegin();
    advectCompute(N, 2, v, v0, u0, v0, dt);
    endAndBegin();

    auto end = mg::timer::now();
    fluidTimes.advecSum += uint32_t(mg::timer::durationInMs(start, end));
  }

  project_C(N, u, v, u0, v0);
  endAndBegin();
  diffuseCompute(N, 0, s, d, 0, dt);
  endAndBegin();
  advectCompute(N, 0, d, s, u, v, dt);
  endAndBegin();

  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

static void get_from_UI_C(int N, mg::StorageId d, mg::StorageId u, mg::StorageId v, const mg::FrameData &frameData) {
  if (!frameData.mouse.left) {
    return;
  }
  int i = int(frameData.mouse.xy.x * N);
  int j = int((1.0f - frameData.mouse.xy.y) * N);
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
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

void simulateNavierStroke(const Storages &storages, const mg::FrameData &frameData) {
  float dt = 0.1f;
  uint32_t N = 128;
  endAndBegin();

  get_from_UI_C(N, storages.d, storages.u, storages.v, frameData);
  endAndBegin();
  vel_step_c(N, storages.u, storages.v, storages.u0, storages.v0, storages.d, storages.s, 0, dt);
  
}

void renderNavierStroke(const mg::RenderContext &renderContext, const Storages &storages) {
  renderFluid(renderContext, storages.d);
}

} // namespace mg