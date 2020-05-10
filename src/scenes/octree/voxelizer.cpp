#include "voxelizer.h"

#include "chunkedAllocator.h"
#include "empty_render_pass.h"
#include "mg/mgSystem.h"
#include "rendering/rendering.h"
#include <iostream>
#include <stack>

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

std::vector<glm::uvec4> createOctreeFromMesh(const RenderContext &renderContext, const EmptyRenderPass emptyRenderPass,
                                             const MeshId &id) {
  using namespace mg::shaders::voxelizer;

  auto compute = createCompute();
  beginEmptyRenderPass(emptyRenderPass, float(mg::vkContext.screen.width), compute.commandBuffer);

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.rasterization.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.rasterization.vkPipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  pipelineStateDesc.rasterization.graphics.subpass = renderContext.subpass;
  pipelineStateDesc.rasterization.rasterization.cullMode = VK_CULL_MODE_NONE;
  pipelineStateDesc.rasterization.depth.TestEnable = VK_FALSE;
  pipelineStateDesc.rasterization.depth.writeEnable = VK_FALSE;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = shader;
  createPipelineInfo.vertexInputState = InputAssembler::vertexInputState;
  createPipelineInfo.vertexInputStateCount = mg::countof(InputAssembler::vertexInputState);

  const auto pipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);

  ubo->resolution = 8.0f;

  VkBuffer storageBuffer;
  uint32_t storageOffset;
  VkDescriptorSet storageSet;
  Voxels *voxels = (Voxels *)mg::mgSystem.linearHeapAllocator.allocateStorage(sizeof(Voxels), &storageBuffer,
                                                                              &storageOffset, &storageSet);
  memset(voxels, 0, sizeof(voxels));

  DescriptorSets descriptorSets = {.ubo = uboSet};

  uint32_t dynamicOffsets[] = {uniformOffset, storageOffset};
  vkCmdBindDescriptorSets(compute.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  const auto mesh = mg::getMesh(id);
  vkCmdBindPipeline(compute.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(compute.commandBuffer, 0, 1, &mesh.buffer, &offset);
  // vkCmdBindIndexBuffer(compute.commandBuffer, mesh.buffer, mesh.indicesOffset, VK_INDEX_TYPE_UINT32);
  vkCmdDraw(compute.commandBuffer, mesh.indexCount, 1, 0, 0);

  endRenderEmptyRenderPass(compute.commandBuffer);

  vkEndCommandBuffer(compute.commandBuffer);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &compute.commandBuffer;

  // fences are GPU to CPU syncs, CPU waits for GPU to finish it's current work on the queue for a
  // specific command buffer
  checkResult(vkQueueSubmit(vkContext.queue, 1, &submitInfo, compute.fence));
  checkResult(vkWaitForFences(vkContext.device, 1, &compute.fence, VK_TRUE, UINT64_MAX));

  Voxels cpuVoxels;
  memcpy(&cpuVoxels, voxels, sizeof(Voxels));
  std::vector<glm::uvec4> res;
  for (size_t i = 0; i < cpuVoxels.count.x; i++) {
    res.push_back(cpuVoxels.values[i]);
  }

  destroyCompute(&compute);
  return res;
}

bool isInside(glm::vec4 pos, uint32_t voxelSize, glm::vec3 corner, uint32_t size) {
  // clang-format off
  return 
    pos.x >= corner.x && 
    pos.y >= corner.y && 
    pos.z >= corner.z && 
    pos.x + voxelSize < corner.x + size && 
    pos.y + voxelSize < corner.y + size && 
    pos.z + voxelSize < corner.z + size;
  // clang-format on
}

uint64_t _createOctree(ChunkedAllocator<uint32_t> &allocator, const std::vector<glm::uvec4> &vec, glm::uvec3 pos,
                       uint32_t size, uint64_t descriptorIndex);
void traverseTree(std::vector<uint32_t> vecs);

std::vector<uint32_t> buildOctree(const std::vector<glm::uvec4> &vec, glm::uvec3 pos, uint32_t size) {
  ChunkedAllocator<uint32_t> allocator = {};
  allocator.push_back(0);
  _createOctree(allocator, vec, pos, size, 0);
  allocator[0] |= 1 << 18;
  auto res = allocator.finalize();
  traverseTree(res);
  return res;
}

std::string bits(uint32_t nr, uint32_t n) {
  std::string bits;
  while (n--) {
    bits += nr & 1 ? '1' : '0';
    nr >>= 1;
  }
  std::reverse(bits.begin(), bits.end());
  return bits;
}

void traverseTree(std::vector<uint32_t> vecs) {
  std::stack<uint32_t> queue;
  queue.push(vecs.front());
  while (queue.size()) {
    auto n = queue.top();
    queue.pop();
    auto leaf = 0xFF & n;
    auto children = (n >> 8) & 0xFF;
    auto farB = (n & 0x10000) >> 16;
    auto child = n >> 18;
    std::cout << "n " << bits(n, 32) << std::endl;
    std::cout << "leafs " << bits(leaf, 8) << std::endl;
    std::cout << "children " << bits(children, 8) << std::endl;
    std::cout << "child " << child << std::endl;

    if (child && children)
      queue.push(vecs[child]);
  }
}

uint64_t _createOctree(ChunkedAllocator<uint32_t> &allocator, const std::vector<glm::uvec4> &vec, glm::uvec3 pos,
                       uint32_t size, uint64_t descriptorIndex) {

  uint32_t halfSize = size >> 1;

  // clang-format off
  uint32_t posX[] = {pos.x + halfSize, pos.x, pos.x + halfSize, pos.x, pos.x + halfSize, pos.x, pos.x + halfSize, pos.x};
  uint32_t posY[] = {pos.y + halfSize, pos.y + halfSize, pos.y, pos.y, pos.y + halfSize, pos.y + halfSize, pos.y, pos.y};
  uint32_t posZ[] = {pos.z + halfSize, pos.z + halfSize, pos.z + halfSize, pos.z + halfSize, pos.z, pos.z, pos.z, pos.z};
  // clang-format on

  uint32_t childMask = 0;
  uint32_t childIndices[8];
  uint32_t childCount = 0;
  for (uint32_t i = 0; i < 8; i++) {
    for (uint32_t j = 0; j < uint32_t(vec.size()); j++) {
      if (isInside(vec[j], 1, pos, halfSize)) {
        childMask |= 0x80 >> i;
        childIndices[childCount++] = i;
        break;
      }
    }
  }

  uint64_t childOffset = uint64_t(allocator.size()) - descriptorIndex;
  bool hasLargeChildren = false;
  uint32_t leafMask;
  if (halfSize == 0) {
    leafMask = 0;

    for (uint32_t i = 0; i < childCount; i++) {
      uint32_t idx = childIndices[childCount - 1 - i];
      allocator.push_back(0);
    }
  } else {
    leafMask = childMask;
    for (uint32_t i = 0; i < childCount; i++) {
      allocator.push_back(0);
    }
    uint64_t grandChildOffsets[8];
    uint64_t delta = 0;
    uint64_t insertionCound = allocator.insertionCount();
    for (uint32_t i = 0; i < childCount; i++) {
      uint32_t idx = childIndices[childCount - 1 - i];
      grandChildOffsets[i] = delta + _createOctree(allocator, vec, glm::uvec3{posX[idx], posY[idx], posZ[idx]},
                                                   halfSize, descriptorIndex + childOffset + i);
      delta += allocator.insertionCount() - insertionCound;
      insertionCound = allocator.insertionCount();
      if (grandChildOffsets[i] > 0x3FFF) {
        hasLargeChildren = true;
      }
    }
    for (uint32_t i = 0; i < childCount; i++) {
      uint64_t childIndex = descriptorIndex + childOffset + i;
      uint64_t offset = grandChildOffsets[i];
      if (hasLargeChildren) {
        offset += childCount - i;
        allocator.insert(childIndex + 1, uint32_t(offset));
        allocator[childIndex] |= 0x20000;
        offset >>= 32;
      }
      allocator[childIndex] |= uint32_t(offset << 18);
    }
  }

  allocator[descriptorIndex] = (childMask << 8) | leafMask;
  if (hasLargeChildren)
    allocator[descriptorIndex] |= 0x10000;
  return childOffset;
}
//     32 - 18  |  17 |    16-9    |   8 - 1
// childPointer | far | valid mask | leaf mask
//     15          1        8           8
//  0xFFFE0000  0x10000   0xFF00       0xFF
// 0x20000 = bit 18
//  0x3FFF 14 bits

void renderCubes(const RenderContext &renderContext, std::vector<glm::uvec4> vec, mg::MeshId meshid) {
  const auto mesh = mg::getMesh(meshid);

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
      vec.size() * sizeof(ia::InstanceInputData), &instanceBuffer, &instanceBufferOffset);

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

  for (uint32_t i = 0; i < vec.size(); i++) {
    glm::vec3 position = {};
    position.x = vec[i].x * 2.0f;
    position.y = vec[i].y * 2.0f;
    position.z = vec[i].z * 2.0f;
    instance[i].instancing_translate = position;
  }

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
  vkCmdBindVertexBuffers(mg::vkContext.commandBuffer, mg::shaders::INSTANCE_BINDING_ID, 1, &instanceBuffer,
                         &instanceBufferOffset);
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(mg::vkContext.commandBuffer, mg::shaders::VERTEX_BINDING_ID, 1, &mesh.buffer, &offset);
  vkCmdBindIndexBuffer(mg::vkContext.commandBuffer, mesh.buffer, mesh.indicesOffset, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(mg::vkContext.commandBuffer, mesh.indexCount, uint32_t(vec.size()), 0, 0, 0);
}

} // namespace mg
