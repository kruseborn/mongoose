#include "navier_stroke.h"

#include "mg/mgSystem.h"
#include "mg/window.h"
#include "rendering/rendering.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

// incrompressible fluid: water
// compressible fluid: air

namespace mg {

  struct Compute {
  VkFence fence;
  VkCommandBuffer commandBuffer;
};

static Compute createCompute() {
  Compute compute = {};

  // create and begin command buffer recording
  VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
  commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  commandBufferAllocateInfo.commandPool = vkContext.commandPool;
  commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  commandBufferAllocateInfo.commandBufferCount = 1;

  checkResult(vkAllocateCommandBuffers(vkContext.device, &commandBufferAllocateInfo, &compute.commandBuffer));

  VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {};
  vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  checkResult(vkBeginCommandBuffer(compute.commandBuffer, &vkCommandBufferBeginInfo));

  // create and reset fence
  VkFenceCreateInfo vkFenceCreateInfo = {};
  vkFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  vkFenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  checkResult(vkCreateFence(vkContext.device, &vkFenceCreateInfo, nullptr, &compute.fence));
  checkResult(vkResetFences(vkContext.device, 1, &compute.fence));

  return compute;
}

static void destroyCompute(Compute *compute) {
  vkFreeCommandBuffers(vkContext.device, vkContext.commandPool, 1, &compute->commandBuffer);
  vkDestroyFence(vkContext.device, compute->fence, nullptr);
}

FluidTimes fluidTimes = {};

static constexpr uint32_t size = (128 + 2) * (128 + 2);
static constexpr float force = 5.0f;
static constexpr float source = 100.0f;
static constexpr float visc = 0.0f;
static constexpr float diff = 0.0f;

static float u[size] = {}, v[size] = {}, u_prev[size] = {}, v_prev[size] = {};
static float dens[size] = {}, dens_prev[size] = {};
static float s[size] = {};

#define IX(i, j) ((i) + (N + 2) * (j))

void set_bnd(int N, int b, float *x) {
  int i;
  for (i = 1; i <= N; i++) {
    x[IX(0, i)] = b == 1 ? -x[IX(1, i)] : x[IX(1, i)];
    x[IX(N + 1, i)] = b == 1 ? -x[IX(N, i)] : x[IX(N, i)];
    x[IX(i, 0)] = b == 2 ? -x[IX(i, 1)] : x[IX(i, 1)];
    x[IX(i, N + 1)] = b == 2 ? -x[IX(i, N)] : x[IX(i, N)];
  }
  x[IX(0, 0)] = 0.5f * (x[IX(1, 0)] + x[IX(0, 1)]);
  x[IX(0, N + 1)] = 0.5f * (x[IX(1, N + 1)] + x[IX(0, N)]);
  x[IX(N + 1, 0)] = 0.5f * (x[IX(N, 0)] + x[IX(N + 1, 1)]);
  x[IX(N + 1, N + 1)] = 0.5f * (x[IX(N, N + 1)] + x[IX(N + 1, N)]);
}

void diffuseCompute(int N, int b, float *x, float *x0, float diff, float dt) {
  auto compute = createCompute();
  using namespace mg::shaders::diffuse;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.compute.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutStorage;

  const auto pipeline = mg::mgSystem.pipelineContainer.createComputePipeline(pipelineStateDesc, {.shaderName = shader});

  uint32_t sizeInBytes = sizeof(float) * size;

  auto su = mg::mgSystem.storageContainer.createStorage(x, sizeInBytes);
  auto sv = mg::mgSystem.storageContainer.createStorage(x0, sizeInBytes);

  mg::waitForDeviceIdle();

  auto ssu = mg::mgSystem.storageContainer.getStorage(su);
  auto ssv = mg::mgSystem.storageContainer.getStorage(sv);

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->dt = dt;
  ubo->N = N;
  ubo->diff = 0;
  ubo->b = b;

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.x = ssu.descriptorSet;
  descriptorSets.x0 = ssv.descriptorSet;

  vkCmdBindPipeline(compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  vkCmdDispatch(compute.commandBuffer, size, 1, 1);

  // setup a memory barrier, device memory has to be finished with writing when reading occurs on
  // the host
  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

  vkCmdPipelineBarrier(compute.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                       0, &memoryBarrier, 0, nullptr, 0, nullptr);

  StorageData storageDatas[] = {ssu, ssv };
  float *datas[] = {x, x0};
  VkDeviceMemory memories[2];
  VkBuffer buffers[2];

  for (uint32_t i = 0; i < 2; i++) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeInBytes;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffer is exclusive to a single queue family at a time.
    checkResult(vkCreateBuffer(mg::vkContext.device, &bufferInfo, nullptr, &buffers[i]));

    VkMemoryRequirements vkMemoryRequirements = {};
    vkGetBufferMemoryRequirements(mg::vkContext.device, buffers[i], &vkMemoryRequirements);

    const auto memoryTypeIndex =
        findMemoryTypeIndex(mg::vkContext.physicalDeviceMemoryProperties, vkMemoryRequirements.memoryTypeBits,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 0);

    VkMemoryAllocateInfo vkMemoryAllocateInfo = {};
    vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkMemoryAllocateInfo.allocationSize = vkMemoryRequirements.size;
    vkMemoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

    checkResult(vkAllocateMemory(mg::vkContext.device, &vkMemoryAllocateInfo, nullptr, &memories[i]));
    checkResult(vkBindBufferMemory(mg::vkContext.device, buffers[i], memories[i], 0));

    VkBufferCopy region = {};
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = sizeInBytes;
    vkCmdCopyBuffer(compute.commandBuffer, storageDatas[i].buffer, buffers[i], 1, &region);

    VkMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;

    vkCmdPipelineBarrier(compute.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0,
                         &memoryBarrier, 0, nullptr, 0, nullptr);
  }

  vkEndCommandBuffer(compute.commandBuffer);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &compute.commandBuffer;

  // fences are GPU to CPU syncs, CPU waits for GPU to finish it's current work on the queue for a
  // specific command buffer
  checkResult(vkQueueSubmit(vkContext.queue, 1, &submitInfo, compute.fence));
  checkResult(vkWaitForFences(vkContext.device, 1, &compute.fence, VK_TRUE, UINT64_MAX));

  for (uint32_t i = 0; i < 2; i++) {
    void *data = nullptr;
    checkResult(vkMapMemory(mg::vkContext.device, memories[i], 0, VK_WHOLE_SIZE, 0, &data));
    memcpy(datas[i], data, sizeInBytes);
    vkUnmapMemory(mg::vkContext.device, memories[i]);

    vkFreeMemory(mg::vkContext.device, memories[i], nullptr);
    vkDestroyBuffer(mg::vkContext.device, buffers[i], nullptr);
  }

  destroyCompute(&compute);

  mg::waitForDeviceIdle();
  mg::mgSystem.storageContainer.removeStorage(su);
  mg::mgSystem.storageContainer.removeStorage(sv);

  mg::waitForDeviceIdle();

}

void diffuse(int N, int b, float *x, float *x0, float diff, float dt) {
  float a = dt * diff * N * N;
  for (size_t iterations = 0; iterations < 5; iterations++) {
    for (size_t j = 0; j < N; j++) {
      for (size_t i = 0; i < N; i++) {
        x[IX(i, j)] =
            (x0[IX(i, j)] + a * (x[IX(i - 1, j)] + x[IX(i + 1, j)] + x[IX(i, j - 1)] + x[IX(i, j + 1)])) / (1 + 4 * a);
      }
    }
  }
  set_bnd(N, b, x);
} // namespace mg



void advectCompute(int N, int b, float *d, float *d0, float *u, float *v, float dt) {
  auto compute = createCompute();
  using namespace mg::shaders::advec;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.compute.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutStorage;

  const auto pipeline = mg::mgSystem.pipelineContainer.createComputePipeline(pipelineStateDesc, {.shaderName = shader});

  uint32_t sizeInBytes = sizeof(float) * size;

  auto su = mg::mgSystem.storageContainer.createStorage(u, sizeInBytes);
  auto sv = mg::mgSystem.storageContainer.createStorage(v, sizeInBytes);
  auto sd = mg::mgSystem.storageContainer.createStorage(d, sizeInBytes);
  auto sd0 = mg::mgSystem.storageContainer.createStorage(d0, sizeInBytes);

  mg::waitForDeviceIdle();

  auto ssu = mg::mgSystem.storageContainer.getStorage(su);
  auto ssv = mg::mgSystem.storageContainer.getStorage(sv);
  auto ssd = mg::mgSystem.storageContainer.getStorage(sd);
  auto ssd0 = mg::mgSystem.storageContainer.getStorage(sd0);

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->dt = dt;
  ubo->N = N;

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.uu = ssu.descriptorSet;
  descriptorSets.vv = ssv.descriptorSet;
  descriptorSets.d = ssd.descriptorSet;
  descriptorSets.d0 = ssd0.descriptorSet;
  

  vkCmdBindPipeline(compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  vkCmdDispatch(compute.commandBuffer, size, 1, 1);

  // setup a memory barrier, device memory has to be finished with writing when reading occurs on
  // the host
  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

  vkCmdPipelineBarrier(compute.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                       0, &memoryBarrier, 0, nullptr, 0, nullptr);

  StorageData storageDatas[] = {ssu, ssv, ssd, ssd0};
  float *datas[] = {u, v, d, d0};
  VkDeviceMemory memories[4];
  VkBuffer buffers[4];


  for (uint32_t i = 0; i < 4; i++) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeInBytes;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffer is exclusive to a single queue family at a time.
    checkResult(vkCreateBuffer(mg::vkContext.device, &bufferInfo, nullptr, &buffers[i]));

    VkMemoryRequirements vkMemoryRequirements = {};
    vkGetBufferMemoryRequirements(mg::vkContext.device, buffers[i], &vkMemoryRequirements);

    const auto memoryTypeIndex =
        findMemoryTypeIndex(mg::vkContext.physicalDeviceMemoryProperties, vkMemoryRequirements.memoryTypeBits,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 0);

    VkMemoryAllocateInfo vkMemoryAllocateInfo = {};
    vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkMemoryAllocateInfo.allocationSize = vkMemoryRequirements.size;
    vkMemoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

    checkResult(vkAllocateMemory(mg::vkContext.device, &vkMemoryAllocateInfo, nullptr, &memories[i]));
    checkResult(vkBindBufferMemory(mg::vkContext.device, buffers[i], memories[i], 0));

    VkBufferCopy region = {};
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = sizeInBytes;
    vkCmdCopyBuffer(compute.commandBuffer, storageDatas[i].buffer, buffers[i], 1, &region);

    VkMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;

    vkCmdPipelineBarrier(compute.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0,
                         &memoryBarrier, 0, nullptr, 0, nullptr);
  }

  vkEndCommandBuffer(compute.commandBuffer);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &compute.commandBuffer;

  // fences are GPU to CPU syncs, CPU waits for GPU to finish it's current work on the queue for a
  // specific command buffer
  checkResult(vkQueueSubmit(vkContext.queue, 1, &submitInfo, compute.fence));
  checkResult(vkWaitForFences(vkContext.device, 1, &compute.fence, VK_TRUE, UINT64_MAX));

  for (uint32_t i = 0; i < 4; i++) {
    void *data = nullptr;
    checkResult(vkMapMemory(mg::vkContext.device, memories[i], 0, VK_WHOLE_SIZE, 0, &data));
    memcpy(datas[i], data, sizeInBytes);
    vkUnmapMemory(mg::vkContext.device, memories[i]);

    vkFreeMemory(mg::vkContext.device, memories[i], nullptr);
    vkDestroyBuffer(mg::vkContext.device, buffers[i], nullptr);
  }

  destroyCompute(&compute);

  mg::waitForDeviceIdle();
  mg::mgSystem.storageContainer.removeStorage(su);
  mg::mgSystem.storageContainer.removeStorage(sv);
  mg::mgSystem.storageContainer.removeStorage(sd);
  mg::mgSystem.storageContainer.removeStorage(sd0);

  mg::waitForDeviceIdle();

}

void advect(int N, int b, float *d, float *d0, float *u, float *v, float dt) {
  int i, j, i0, j0, i1, j1;
  float s0, t0, s1, t1, dt0;
  dt0 = dt * N;
  for (i = 1; i <= N; i++) {
    for (j = 1; j <= N; j++) {
      float prevX = i - dt0 * u[IX(i, j)];
      float prevY = j - dt0 * v[IX(i, j)];
      prevX = std::max(prevX, 0.5f);
      prevX = std::min(prevX, N + 0.5f);
      prevY = std::max(prevY, 0.5f);
      prevY = std::min(prevY, N + 0.5f);
      i0 = (int)prevX;
      i1 = i0 + 1;
      j0 = (int)prevY;
      j1 = j0 + 1;

      s1 = prevX - i0;
      s0 = 1 - s1;
      t1 = prevY - j0;
      t0 = 1 - t1;
      d[IX(i, j)] = s0 * (t0 * d0[IX(i0, j0)] + t1 * d0[IX(i0, j1)]) + s1 * (t0 * d0[IX(i1, j0)] + t1 * d0[IX(i1, j1)]);
    }
  }
  set_bnd(N, b, d);
}

void preProjectCompute(int N, float *u, float *v, float *p, float *div) {

  auto compute = createCompute();
  using namespace mg::shaders::preProject;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.compute.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutStorage;

  const auto pipeline = mg::mgSystem.pipelineContainer.createComputePipeline(pipelineStateDesc, {.shaderName = shader});

  uint32_t sizeInBytes = sizeof(float) * size;

  auto su = mg::mgSystem.storageContainer.createStorage(u, sizeInBytes);
  auto sv = mg::mgSystem.storageContainer.createStorage(v, sizeInBytes);
  auto sd = mg::mgSystem.storageContainer.createStorage(p, sizeInBytes);
  auto sd0 = mg::mgSystem.storageContainer.createStorage(div, sizeInBytes);

  mg::waitForDeviceIdle();

  auto ssu = mg::mgSystem.storageContainer.getStorage(su);
  auto ssv = mg::mgSystem.storageContainer.getStorage(sv);
  auto ssd = mg::mgSystem.storageContainer.getStorage(sd);
  auto ssd0 = mg::mgSystem.storageContainer.getStorage(sd0);

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->N = N;

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.u = ssu.descriptorSet;
  descriptorSets.v = ssv.descriptorSet;
  descriptorSets.p = ssd.descriptorSet;
  descriptorSets.div = ssd0.descriptorSet;

  vkCmdBindPipeline(compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  vkCmdDispatch(compute.commandBuffer, size, 1, 1);

  // setup a memory barrier, device memory has to be finished with writing when reading occurs on
  // the host
  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

  vkCmdPipelineBarrier(compute.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                       0, &memoryBarrier, 0, nullptr, 0, nullptr);

  StorageData storageDatas[] = {ssu, ssv, ssd, ssd0};
  float *datas[] = {u, v, p, div};
  VkDeviceMemory memories[4];
  VkBuffer buffers[4];

  for (uint32_t i = 0; i < 4; i++) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeInBytes;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffer is exclusive to a single queue family at a time.
    checkResult(vkCreateBuffer(mg::vkContext.device, &bufferInfo, nullptr, &buffers[i]));

    VkMemoryRequirements vkMemoryRequirements = {};
    vkGetBufferMemoryRequirements(mg::vkContext.device, buffers[i], &vkMemoryRequirements);

    const auto memoryTypeIndex =
        findMemoryTypeIndex(mg::vkContext.physicalDeviceMemoryProperties, vkMemoryRequirements.memoryTypeBits,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 0);

    VkMemoryAllocateInfo vkMemoryAllocateInfo = {};
    vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkMemoryAllocateInfo.allocationSize = vkMemoryRequirements.size;
    vkMemoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

    checkResult(vkAllocateMemory(mg::vkContext.device, &vkMemoryAllocateInfo, nullptr, &memories[i]));
    checkResult(vkBindBufferMemory(mg::vkContext.device, buffers[i], memories[i], 0));

    VkBufferCopy region = {};
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = sizeInBytes;
    vkCmdCopyBuffer(compute.commandBuffer, storageDatas[i].buffer, buffers[i], 1, &region);

    VkMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;

    vkCmdPipelineBarrier(compute.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0,
                         &memoryBarrier, 0, nullptr, 0, nullptr);
  }

  vkEndCommandBuffer(compute.commandBuffer);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &compute.commandBuffer;

  // fences are GPU to CPU syncs, CPU waits for GPU to finish it's current work on the queue for a
  // specific command buffer
  checkResult(vkQueueSubmit(vkContext.queue, 1, &submitInfo, compute.fence));
  checkResult(vkWaitForFences(vkContext.device, 1, &compute.fence, VK_TRUE, UINT64_MAX));

  for (uint32_t i = 0; i < 4; i++) {
    void *data = nullptr;
    checkResult(vkMapMemory(mg::vkContext.device, memories[i], 0, VK_WHOLE_SIZE, 0, &data));
    memcpy(datas[i], data, sizeInBytes);
    vkUnmapMemory(mg::vkContext.device, memories[i]);

    vkFreeMemory(mg::vkContext.device, memories[i], nullptr);
    vkDestroyBuffer(mg::vkContext.device, buffers[i], nullptr);
  }

  destroyCompute(&compute);

  mg::waitForDeviceIdle();
  mg::mgSystem.storageContainer.removeStorage(su);
  mg::mgSystem.storageContainer.removeStorage(sv);
  mg::mgSystem.storageContainer.removeStorage(sd);
  mg::mgSystem.storageContainer.removeStorage(sd0);

  mg::waitForDeviceIdle();
}
void projectCompute(int N, float *u, float *v, float *p, float *div) {

  auto compute = createCompute();
  using namespace mg::shaders::project;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.compute.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutStorage;

  const auto pipeline = mg::mgSystem.pipelineContainer.createComputePipeline(pipelineStateDesc, {.shaderName = shader});

  uint32_t sizeInBytes = sizeof(float) * size;

  auto su = mg::mgSystem.storageContainer.createStorage(u, sizeInBytes);
  auto sv = mg::mgSystem.storageContainer.createStorage(v, sizeInBytes);
  auto sd = mg::mgSystem.storageContainer.createStorage(p, sizeInBytes);
  auto sd0 = mg::mgSystem.storageContainer.createStorage(div, sizeInBytes);

  mg::waitForDeviceIdle();

  auto ssu = mg::mgSystem.storageContainer.getStorage(su);
  auto ssv = mg::mgSystem.storageContainer.getStorage(sv);
  auto ssd = mg::mgSystem.storageContainer.getStorage(sd);
  auto ssd0 = mg::mgSystem.storageContainer.getStorage(sd0);

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->N = N;

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.u = ssu.descriptorSet;
  descriptorSets.v = ssv.descriptorSet;
  descriptorSets.p = ssd.descriptorSet;
  descriptorSets.div = ssd0.descriptorSet;

  vkCmdBindPipeline(compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  vkCmdDispatch(compute.commandBuffer, size, 1, 1);

  // setup a memory barrier, device memory has to be finished with writing when reading occurs on
  // the host
  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

  vkCmdPipelineBarrier(compute.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                       0, &memoryBarrier, 0, nullptr, 0, nullptr);

  StorageData storageDatas[] = {ssu, ssv, ssd, ssd0};
  float *datas[] = {u, v, p, div};
  VkDeviceMemory memories[4];
  VkBuffer buffers[4];

  for (uint32_t i = 0; i < 4; i++) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeInBytes;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffer is exclusive to a single queue family at a time.
    checkResult(vkCreateBuffer(mg::vkContext.device, &bufferInfo, nullptr, &buffers[i]));

    VkMemoryRequirements vkMemoryRequirements = {};
    vkGetBufferMemoryRequirements(mg::vkContext.device, buffers[i], &vkMemoryRequirements);

    const auto memoryTypeIndex =
        findMemoryTypeIndex(mg::vkContext.physicalDeviceMemoryProperties, vkMemoryRequirements.memoryTypeBits,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 0);

    VkMemoryAllocateInfo vkMemoryAllocateInfo = {};
    vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkMemoryAllocateInfo.allocationSize = vkMemoryRequirements.size;
    vkMemoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

    checkResult(vkAllocateMemory(mg::vkContext.device, &vkMemoryAllocateInfo, nullptr, &memories[i]));
    checkResult(vkBindBufferMemory(mg::vkContext.device, buffers[i], memories[i], 0));

    VkBufferCopy region = {};
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = sizeInBytes;
    vkCmdCopyBuffer(compute.commandBuffer, storageDatas[i].buffer, buffers[i], 1, &region);

    VkMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;

    vkCmdPipelineBarrier(compute.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0,
                         &memoryBarrier, 0, nullptr, 0, nullptr);
  }

  vkEndCommandBuffer(compute.commandBuffer);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &compute.commandBuffer;

  // fences are GPU to CPU syncs, CPU waits for GPU to finish it's current work on the queue for a
  // specific command buffer
  checkResult(vkQueueSubmit(vkContext.queue, 1, &submitInfo, compute.fence));
  checkResult(vkWaitForFences(vkContext.device, 1, &compute.fence, VK_TRUE, UINT64_MAX));

  for (uint32_t i = 0; i < 4; i++) {
    void *data = nullptr;
    checkResult(vkMapMemory(mg::vkContext.device, memories[i], 0, VK_WHOLE_SIZE, 0, &data));
    memcpy(datas[i], data, sizeInBytes);
    vkUnmapMemory(mg::vkContext.device, memories[i]);

    vkFreeMemory(mg::vkContext.device, memories[i], nullptr);
    vkDestroyBuffer(mg::vkContext.device, buffers[i], nullptr);
  }

  destroyCompute(&compute);

  mg::waitForDeviceIdle();
  mg::mgSystem.storageContainer.removeStorage(su);
  mg::mgSystem.storageContainer.removeStorage(sv);
  mg::mgSystem.storageContainer.removeStorage(sd);
  mg::mgSystem.storageContainer.removeStorage(sd0);

  mg::waitForDeviceIdle();
}

void postProjectCompute(int N, float *u, float *v, float *p, float *div) {

  auto compute = createCompute();
  using namespace mg::shaders::postProject;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.compute.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutStorage;

  const auto pipeline = mg::mgSystem.pipelineContainer.createComputePipeline(pipelineStateDesc, {.shaderName = shader});

  uint32_t sizeInBytes = sizeof(float) * size;

  auto su = mg::mgSystem.storageContainer.createStorage(u, sizeInBytes);
  auto sv = mg::mgSystem.storageContainer.createStorage(v, sizeInBytes);
  auto sd = mg::mgSystem.storageContainer.createStorage(p, sizeInBytes);
  auto sd0 = mg::mgSystem.storageContainer.createStorage(div, sizeInBytes);

  mg::waitForDeviceIdle();

  auto ssu = mg::mgSystem.storageContainer.getStorage(su);
  auto ssv = mg::mgSystem.storageContainer.getStorage(sv);
  auto ssd = mg::mgSystem.storageContainer.getStorage(sd);
  auto ssd0 = mg::mgSystem.storageContainer.getStorage(sd0);

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->N = N;

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.u = ssu.descriptorSet;
  descriptorSets.v = ssv.descriptorSet;
  descriptorSets.p = ssd.descriptorSet;
  descriptorSets.div = ssd0.descriptorSet;

  vkCmdBindPipeline(compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  vkCmdDispatch(compute.commandBuffer, size, 1, 1);

  // setup a memory barrier, device memory has to be finished with writing when reading occurs on
  // the host
  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

  vkCmdPipelineBarrier(compute.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                       0, &memoryBarrier, 0, nullptr, 0, nullptr);

  StorageData storageDatas[] = {ssu, ssv, ssd, ssd0};
  float *datas[] = {u, v, p, div};
  VkDeviceMemory memories[4];
  VkBuffer buffers[4];

  for (uint32_t i = 0; i < 4; i++) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeInBytes;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffer is exclusive to a single queue family at a time.
    checkResult(vkCreateBuffer(mg::vkContext.device, &bufferInfo, nullptr, &buffers[i]));

    VkMemoryRequirements vkMemoryRequirements = {};
    vkGetBufferMemoryRequirements(mg::vkContext.device, buffers[i], &vkMemoryRequirements);

    const auto memoryTypeIndex =
        findMemoryTypeIndex(mg::vkContext.physicalDeviceMemoryProperties, vkMemoryRequirements.memoryTypeBits,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 0);

    VkMemoryAllocateInfo vkMemoryAllocateInfo = {};
    vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkMemoryAllocateInfo.allocationSize = vkMemoryRequirements.size;
    vkMemoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

    checkResult(vkAllocateMemory(mg::vkContext.device, &vkMemoryAllocateInfo, nullptr, &memories[i]));
    checkResult(vkBindBufferMemory(mg::vkContext.device, buffers[i], memories[i], 0));

    VkBufferCopy region = {};
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = sizeInBytes;
    vkCmdCopyBuffer(compute.commandBuffer, storageDatas[i].buffer, buffers[i], 1, &region);

    VkMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;

    vkCmdPipelineBarrier(compute.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0,
                         &memoryBarrier, 0, nullptr, 0, nullptr);
  }

  vkEndCommandBuffer(compute.commandBuffer);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &compute.commandBuffer;

  // fences are GPU to CPU syncs, CPU waits for GPU to finish it's current work on the queue for a
  // specific command buffer
  checkResult(vkQueueSubmit(vkContext.queue, 1, &submitInfo, compute.fence));
  checkResult(vkWaitForFences(vkContext.device, 1, &compute.fence, VK_TRUE, UINT64_MAX));

  for (uint32_t i = 0; i < 4; i++) {
    void *data = nullptr;
    checkResult(vkMapMemory(mg::vkContext.device, memories[i], 0, VK_WHOLE_SIZE, 0, &data));
    memcpy(datas[i], data, sizeInBytes);
    vkUnmapMemory(mg::vkContext.device, memories[i]);

    vkFreeMemory(mg::vkContext.device, memories[i], nullptr);
    vkDestroyBuffer(mg::vkContext.device, buffers[i], nullptr);
  }

  destroyCompute(&compute);

  mg::waitForDeviceIdle();
  mg::mgSystem.storageContainer.removeStorage(su);
  mg::mgSystem.storageContainer.removeStorage(sv);
  mg::mgSystem.storageContainer.removeStorage(sd);
  mg::mgSystem.storageContainer.removeStorage(sd0);

  mg::waitForDeviceIdle();
}

void project_C(int N, float *u, float *v, float *p, float *div) {
  // for (i = 1; i <= N; i++) {
  //  for (j = 1; j <= N; j++) {
  //    div[IX(i, j)] = -0.5f * h * (u[IX(i + 1, j)] - u[IX(i - 1, j)] + v[IX(i, j + 1)] - v[IX(i, j - 1)]);
  //    p[IX(i, j)] = 0;
  //  }
  //}
  // set_bnd(N, 0, div);
  // set_bnd(N, 0, p);
  preProjectCompute(N, u, v, p, div);
  // mg::waitForDeviceIdle();

  projectCompute(N, u, v, p, div);
  //for (k = 0; k < 20; k++) {
  //  for (i = 1; i <= N; i++) {
  //    for (j = 1; j <= N; j++) {
  //      p[IX(i, j)] = (div[IX(i, j)] + p[IX(i - 1, j)] + p[IX(i + 1, j)] + p[IX(i, j - 1)] + p[IX(i, j + 1)]) / 4;
  //    }
  //  }
  //  set_bnd(N, 0, p);
  //}
  postProjectCompute(N, u, v, p, div);
}

void project(int N, float *u, float *v, float *p, float *div) {
  int i, j, k;
  float h;
  h = 1.0f / N;
  for (i = 1; i <= N; i++) {
    for (j = 1; j <= N; j++) {
      div[IX(i, j)] = -0.5f * h * (u[IX(i + 1, j)] - u[IX(i - 1, j)] + v[IX(i, j + 1)] - v[IX(i, j - 1)]);
      p[IX(i, j)] = 0;
    }
  }
  set_bnd(N, 0, div);
  set_bnd(N, 0, p);

  for (k = 0; k < 20; k++) {
    for (i = 1; i <= N; i++) {
      for (j = 1; j <= N; j++) {
        p[IX(i, j)] = (div[IX(i, j)] + p[IX(i - 1, j)] + p[IX(i + 1, j)] + p[IX(i, j - 1)] + p[IX(i, j + 1)]) / 4;
      }
    }
    set_bnd(N, 0, p);
  }
  for (i = 1; i <= N; i++) {
    for (j = 1; j <= N; j++) {
      u[IX(i, j)] -= 0.5f * (p[IX(i + 1, j)] - p[IX(i - 1, j)]) / h;
      v[IX(i, j)] -= 0.5f * (p[IX(i, j + 1)] - p[IX(i, j - 1)]) / h;
    }
  }
  set_bnd(N, 1, u);
  set_bnd(N, 2, v);
}

void vel_step(int N, float *u, float *v, float *u0, float *v0, float visc, float dt) {
  {
    auto start = mg::timer::now();
    diffuseCompute(N, 1, u0, u, visc, dt);
    diffuseCompute(N, 2, v0, v, visc, dt);
    auto end = mg::timer::now();
    fluidTimes.diffuseSum += uint32_t(mg::timer::durationInMs(start, end));
  }

  {
    auto start = mg::timer::now();
    //project(N, u0, v0, u, v);
    project_C(N, u0, v0, u, v);
    auto end = mg::timer::now();
    fluidTimes.projectSum += uint32_t(mg::timer::durationInMs(start, end));
  }

  {
    auto start = mg::timer::now();

    advectCompute(N, 1, u, u0, u0, v0, dt);

    advectCompute(N, 2, v, v0, u0, v0, dt);
    auto end = mg::timer::now();
    fluidTimes.advecSum += uint32_t(mg::timer::durationInMs(start, end));
  }

  project_C(N, u, v, u0, v0);

  // density step
  diffuseCompute(N, 0, s, dens, diff, dt);
  advectCompute(N, 0, dens, s, u, v, dt);
}

static void get_from_UI(int N, float *d, float *u, float *v, const mg::FrameData &frameData) {

  if (frameData.mouse.left) {
    int i, j;
    i = int(frameData.mouse.xy.x * N);
    j = int((1.0f - frameData.mouse.xy.y) * N);
    auto delta = frameData.mouse.xy - frameData.mouse.prevXY;

    for (int y = j - 4; y <= j + 4; y++) {
      for (int x = i - 4; x <= i + 4; x++) {
        u[IX(x, y)] += 50.0f * delta.x;
        v[IX(x, y)] += 50.0f * (-1.0f * delta.y);
        d[IX(x, y)] += 0.5;
      }
    }
  }
}

void simulateNavierStroke(const mg::FrameData &frameData) {
  float dt = 0.1f;
  uint32_t N = 128;
  get_from_UI(N, dens, u, v, frameData);
  vel_step(N, u, v, u_prev, v_prev, visc, dt);
  for (size_t i = 0; i < size; i++) {
    dens[i] *= 0.95f;
  }
}

void renderNavierStroke(const mg::RenderContext &renderContext) { renderFluid(renderContext, dens, size); }

} // namespace mg