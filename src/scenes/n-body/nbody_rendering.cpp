#include "nbody_rendering.h"
#include "mg/mgSystem.h"
#include "nbody_utils.h"
#include "rendering/rendering.h"

void computeMandelbrot(const ComputeData &computeData) {
  using namespace mg::shaders::nbody;

  mg::ComputePipelineStateDesc computePipelineStateDesc = {};
  computePipelineStateDesc.pipelineLayout = mg::vkContext.pipelineLayouts.pipelineComputeLayout;
  computePipelineStateDesc.shaderName = "nbody.comp";

  const auto pipeline = mg::mgSystem.pipelineContainer.createComputePipeline(computePipelineStateDesc);

  using DescriptorSets = shaderResource::DescriptorSets;
  auto storage = mg::mgSystem.storageContainer.getStorage(computeData.storageId);

  DescriptorSets descriptorSets = {};
  descriptorSets.storage = storage.descriptorSet;

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, 0, nullptr);

  vkCmdDispatch(mg::vkContext.commandBuffer, (uint32_t)ceil(computeData.width / float(computeData.workgroupSize)),
                (uint32_t)ceil(computeData.height / float(computeData.workgroupSize)), 1);

  VkBufferMemoryBarrier vkBufferMemoryBarrier = {};
  vkBufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  	// Add memory barrier to ensure that compute shader has finished writing to the buffer
  vkBufferMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT; // Compute shader has finished writes to the buffer
  vkBufferMemoryBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
  vkBufferMemoryBarrier.buffer = storage.buffer;
  vkBufferMemoryBarrier.size = storage.size;
  // No ownership transfer necessary
  vkBufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  vkBufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  vkCmdPipelineBarrier(mg::vkContext.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0,
                       0, nullptr, 1, &vkBufferMemoryBarrier, 0, nullptr);
}
