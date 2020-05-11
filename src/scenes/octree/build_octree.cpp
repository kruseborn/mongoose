#include "build_octree.h"

#include "mg/mgSystem.h"
#include "mg/window.h"
#include "rendering/rendering.h"
#include <vector>
namespace mg {

void initOctreeStorages(OctreeStorages *storages, const std::vector<glm::uvec2> &voxels) {
  std::vector<glm::uvec4> uBuildInfo(1);
  std::vector<uint32_t> octree(1000000);

  storages->octree = mg::mgSystem.storageContainer.createStorage(octree.data(), mg::sizeofContainerInBytes(octree));
  storages->fragmentList =
      mg::mgSystem.storageContainer.createStorage((void *)voxels.data(), mg::sizeofContainerInBytes(voxels));
  storages->buildInfo =
      mg::mgSystem.storageContainer.createStorage(uBuildInfo.data(), mg::sizeofContainerInBytes(uBuildInfo));

  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

void destroyOctreeStorages(OctreeStorages *storages) {
  mg::mgSystem.storageContainer.removeStorage(storages->octree);
  mg::mgSystem.storageContainer.removeStorage(storages->fragmentList);
  mg::mgSystem.storageContainer.removeStorage(storages->buildInfo);
  *storages = {};
}

static uint32_t group_x_64(unsigned x) {
  return (x >> 6u) + ((x & 0x3fu) ? 1u : 0u);
}

static void tagNodes(const OctreeStorages &storages, const FrameData &frameData, uint32_t octreeLevel,
                     uint32_t voxels) {
  using namespace mg::shaders::octreeTag;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.compute.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutStorage;

  const auto pipeline = mg::mgSystem.pipelineContainer.createComputePipeline(pipelineStateDesc, {.shaderName = shader});

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);

  ubo->attrib = {voxels, 1 << octreeLevel, 0, 0};

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.uOctree = mg::mgSystem.storageContainer.getStorage(storages.octree).descriptorSet;
  descriptorSets.uFragmentList = mg::mgSystem.storageContainer.getStorage(storages.fragmentList).descriptorSet;

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  uint32_t frag_list_group_x = group_x_64(voxels);
  vkCmdDispatch(mg::vkContext.commandBuffer, frag_list_group_x, 1, 1);
}

static void modifyNodes(const OctreeStorages &storages, const FrameData &frameData, VkBuffer *dispatchIndirectBuffer,
                        uint32_t *dispatchIndirectBufferOffset, uint32_t octreeLevel, uint32_t voxels) {
  using namespace mg::shaders::octreeModify;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.compute.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutStorage;

  const auto pipeline = mg::mgSystem.pipelineContainer.createComputePipeline(pipelineStateDesc, {.shaderName = shader});

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->attrib = {voxels, 1 << octreeLevel, 0, 0};

  VkDescriptorSet storageSet;
  DispatchIndirectCommand *drawIndirectCommand =
      (DispatchIndirectCommand *)mg::mgSystem.linearHeapAllocator.allocateStorage(
          sizeof(DispatchIndirectCommand), dispatchIndirectBuffer, dispatchIndirectBufferOffset, &storageSet);

  VkBufferMemoryBarrier bufferMemoryBarrier2 = {};
  bufferMemoryBarrier2.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bufferMemoryBarrier2.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  bufferMemoryBarrier2.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
  bufferMemoryBarrier2.buffer = *dispatchIndirectBuffer;
  bufferMemoryBarrier2.offset = *dispatchIndirectBufferOffset;
  bufferMemoryBarrier2.size = sizeof(DispatchIndirectCommand);
  bufferMemoryBarrier2.srcQueueFamilyIndex = vkContext.queueFamilyIndex;
  bufferMemoryBarrier2.dstQueueFamilyIndex = vkContext.queueFamilyIndex;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier2, 0, nullptr);

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.uBuildInfo = mg::mgSystem.storageContainer.getStorage(storages.buildInfo).descriptorSet;

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

  uint32_t dynamicOffsets[] = {uniformOffset, *dispatchIndirectBufferOffset};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  vkCmdDispatch(mg::vkContext.commandBuffer, 1, 1, 1);
}

static void allocNodes(const OctreeStorages &storages, const FrameData &frameData, VkBuffer *dispatchIndirectBuffer,
                       uint32_t *dispatchIndirectBufferOffset, uint32_t octreeLevel, uint32_t voxels) {
  using namespace mg::shaders::octreeAlloc;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.compute.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayoutStorage;

  const auto pipeline = mg::mgSystem.pipelineContainer.createComputePipeline(pipelineStateDesc, {.shaderName = shader});

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *ubo =
      (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  ubo->attrib = {voxels, 1 << octreeLevel, 0, 0};

  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.uOctree = mg::mgSystem.storageContainer.getStorage(storages.octree).descriptorSet;
  descriptorSets.uBuildInfo = mg::mgSystem.storageContainer.getStorage(storages.buildInfo).descriptorSet;

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  vkCmdDispatchIndirect(mg::vkContext.commandBuffer, *dispatchIndirectBuffer, *dispatchIndirectBufferOffset);
}

static void computeMemoryBarrier() {
  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

void clearBuffers(const OctreeStorages &storages) {

  vkCmdFillBuffer(mg::vkContext.commandBuffer, mg::mgSystem.storageContainer.getStorage(storages.octree).buffer, 0,
                  VK_WHOLE_SIZE, 0);
  vkCmdFillBuffer(mg::vkContext.commandBuffer, mg::mgSystem.storageContainer.getStorage(storages.buildInfo).buffer, 0,
                  VK_WHOLE_SIZE, 0);

  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}
void buildOctree(const OctreeStorages &storages, uint32_t octreeLevel, const mg::FrameData &frameData,
                 uint32_t voxels) {

  clearBuffers(storages);

  VkBuffer dispatchIndirectBuffer;
  uint32_t dispatchIndirectBufferOffset;

  for (uint32_t cur = 0; cur <= octreeLevel; cur++) {
    tagNodes(storages, frameData, octreeLevel, voxels);
    computeMemoryBarrier();

    if (cur != octreeLevel) {
      modifyNodes(storages, frameData, &dispatchIndirectBuffer, &dispatchIndirectBufferOffset, octreeLevel, voxels);
      computeMemoryBarrier();
      allocNodes(storages, frameData, &dispatchIndirectBuffer, &dispatchIndirectBufferOffset, octreeLevel, voxels);
      computeMemoryBarrier();
    }
  }
  VkMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}


} // namespace mg