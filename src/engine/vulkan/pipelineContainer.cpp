#include "pipelineContainer.h"

#include "../mg/mgSystem.h"
#include "shaderPipelineInput.h"
#include "shaders.h"
#include "singleRenderpass.h"
#include "vkContext.h"
#include "vkUtils.h"
#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

// Fowler-Noll-Vo_hash_function
// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
static const uint64_t FNV_ffset_basis = 14695981039346656037ULL;
static const uint64_t FNV_prime = 1099511628211ULL;

static uint64_t hashBytes(const unsigned char *first, const unsigned char *last) {
  mgAssertDesc(sizeof(size_t) == 8, "size_t is not 8 bytes, build for 64 bits");
  mgAssert(uint64_t(last - first) % 8 == 0);
  uint64_t hash = FNV_ffset_basis;
  for (; first != last; ++first) {
    hash = hash ^ static_cast<uint64_t>(*first);
    hash = hash * FNV_prime;
  }
  return hash;
}

namespace mg {

struct _PipelineDesc {
  _PipelineDesc();
  _PipelineDesc(const _PipelineDesc &other);
  _PipelineDesc &operator=(const _PipelineDesc &other);

  PipelineStateDesc state;
  char shaderName[30];
  mg::shaders::VertexInputState vertexInputState[10];
  uint32_t vertexInputStateCount;
};

_PipelineDesc::_PipelineDesc() {
  memset(this, 0, sizeof(_PipelineDesc));
}
_PipelineDesc::_PipelineDesc(const _PipelineDesc &other) {
  memcpy(this, &other, sizeof(_PipelineDesc));
}
_PipelineDesc &_PipelineDesc::operator=(const _PipelineDesc &other) {
  memcpy(this, &other, sizeof(_PipelineDesc));
  return *this;
}

PipelineStateDesc::PipelineStateDesc() {
  memset(this, 0, sizeof(PipelineStateDesc));
  rasterization.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  rasterization.rasterization.polygonMode = VK_POLYGON_MODE_FILL;
  rasterization.rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterization.rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  rasterization.blend.blendEnable = VK_TRUE;
  rasterization.blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  rasterization.blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  rasterization.blend.colorBlendOp = VK_BLEND_OP_ADD;
  rasterization.blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  rasterization.blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  rasterization.blend.alphaBlendOp = VK_BLEND_OP_ADD;

  rasterization.depth.TestEnable = VK_TRUE;
  rasterization.depth.writeEnable = VK_TRUE;
  rasterization.depth.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

  rasterization.graphics.subpass = 0;
  rasterization.graphics.nrOfColorAttachments = 1;

  for (uint32_t i = 0; i < rayTracing.MAX_GROUPS; i++) {
    rayTracing.groups[i].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
    rayTracing.groups[i].generalShader = VK_SHADER_UNUSED_NV;
    rayTracing.groups[i].closestHitShader = VK_SHADER_UNUSED_NV;
    rayTracing.groups[i].anyHitShader = VK_SHADER_UNUSED_NV;
    rayTracing.groups[i].intersectionShader = VK_SHADER_UNUSED_NV;
  }
}

PipelineStateDesc::PipelineStateDesc(const PipelineStateDesc &other) {
  memcpy(this, &other, sizeof(PipelineStateDesc));
}
PipelineStateDesc &PipelineStateDesc::operator=(const PipelineStateDesc &other) {
  memcpy(this, &other, sizeof(PipelineStateDesc));
  return *this;
}

static Pipeline _createPipeline(const _PipelineDesc &pipelineDesc) {
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
  VkPipelineRasterizationStateCreateInfo rasterizationState = {};
  std::array<VkPipelineColorBlendAttachmentState, 5> blendAttachmentState = {};
  VkPipelineColorBlendStateCreateInfo colorBlendState = {};
  VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
  VkPipelineViewportStateCreateInfo viewportState = {};
  VkPipelineMultisampleStateCreateInfo multisampleState = {};
  VkPipelineDynamicStateCreateInfo dynamicState = {};

  inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyState.topology = pipelineDesc.state.rasterization.inputAssemblyState.topology;
  inputAssemblyState.primitiveRestartEnable = VK_FALSE;

  rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizationState.depthClampEnable = VK_FALSE;
  rasterizationState.rasterizerDiscardEnable = VK_FALSE;
  rasterizationState.polygonMode = pipelineDesc.state.rasterization.rasterization.polygonMode;
  rasterizationState.cullMode = pipelineDesc.state.rasterization.rasterization.cullMode;
  rasterizationState.frontFace = pipelineDesc.state.rasterization.rasterization.frontFace;
  rasterizationState.depthBiasEnable = VK_FALSE;
  rasterizationState.depthBiasConstantFactor = 0.0f;
  rasterizationState.depthBiasClamp = 0.0f;
  rasterizationState.depthBiasSlopeFactor = 0.0f;
  rasterizationState.lineWidth = 1.0f;

  for (uint32_t i = 0; i < pipelineDesc.state.rasterization.graphics.nrOfColorAttachments; i++) {
    blendAttachmentState[i].blendEnable = pipelineDesc.state.rasterization.blend.blendEnable;
    blendAttachmentState[i].srcColorBlendFactor = pipelineDesc.state.rasterization.blend.srcColorBlendFactor;
    blendAttachmentState[i].dstColorBlendFactor = pipelineDesc.state.rasterization.blend.dstColorBlendFactor;
    blendAttachmentState[i].colorBlendOp = pipelineDesc.state.rasterization.blend.colorBlendOp;
    blendAttachmentState[i].srcAlphaBlendFactor = pipelineDesc.state.rasterization.blend.srcAlphaBlendFactor;
    blendAttachmentState[i].dstAlphaBlendFactor = pipelineDesc.state.rasterization.blend.dstAlphaBlendFactor;
    blendAttachmentState[i].alphaBlendOp = pipelineDesc.state.rasterization.blend.alphaBlendOp;
    blendAttachmentState[i].colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  }

  colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendState.logicOpEnable = VK_FALSE;
  colorBlendState.logicOp = VK_LOGIC_OP_COPY;
  colorBlendState.attachmentCount = pipelineDesc.state.rasterization.graphics.nrOfColorAttachments;
  colorBlendState.pAttachments = blendAttachmentState.data();

  depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilState.depthTestEnable = pipelineDesc.state.rasterization.depth.TestEnable;
  depthStencilState.depthWriteEnable = pipelineDesc.state.rasterization.depth.writeEnable;
  depthStencilState.depthCompareOp = pipelineDesc.state.rasterization.depth.DepthCompareOp;
  depthStencilState.stencilTestEnable = VK_FALSE;
  depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
  depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
  depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
  depthStencilState.front = depthStencilState.back;

  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampleState.minSampleShading = 1.0f;
  multisampleState.sampleShadingEnable = VK_FALSE;

  const uint32_t nrOfDynamicStates = 2;
  VkDynamicState dynamicStateEnables[nrOfDynamicStates] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.pDynamicStates = dynamicStateEnables;
  dynamicState.dynamicStateCount = nrOfDynamicStates;

  VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.layout = pipelineDesc.state.rasterization.vkPipelineLayout;
  pipelineCreateInfo.renderPass = pipelineDesc.state.rasterization.vkRenderPass;
  pipelineCreateInfo.subpass = pipelineDesc.state.rasterization.graphics.subpass;

  const auto shader = mg::getShader(pipelineDesc.shaderName);

  VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
  vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  constexpr uint32_t maxNrOfInputVertexAttribute = 10;
  mgAssert(pipelineDesc.vertexInputStateCount <= maxNrOfInputVertexAttribute);
  VkVertexInputAttributeDescription vkVertexInputAttributeDescription[maxNrOfInputVertexAttribute] = {};

  uint32_t strideInterleaved = 0;
  uint32_t strideInstancesInterleaved = 0;
  for (uint32_t i = 0; i < pipelineDesc.vertexInputStateCount; i++) {
    vkVertexInputAttributeDescription[i].binding = pipelineDesc.vertexInputState[i].binding;
    vkVertexInputAttributeDescription[i].format = pipelineDesc.vertexInputState[i].format;
    vkVertexInputAttributeDescription[i].offset = pipelineDesc.vertexInputState[i].offset;
    vkVertexInputAttributeDescription[i].location = pipelineDesc.vertexInputState[i].location;
    if (pipelineDesc.vertexInputState[i].binding == 0)
      strideInterleaved += pipelineDesc.vertexInputState[i].size;
    else
      strideInstancesInterleaved += pipelineDesc.vertexInputState[i].size;
  }

  // clang-format off
  VkVertexInputBindingDescription vkVertexInputBindingDescription[2] ={ {
    .binding = 0,
    .stride = strideInterleaved,
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  },{
    .binding = 1,
    .stride = strideInstancesInterleaved,
    .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
  }};
  // clang-format on

  uint32_t count = uint32_t(strideInterleaved > 0) + uint32_t(strideInstancesInterleaved > 0);
  vertexInputStateCreateInfo.pVertexBindingDescriptions = vkVertexInputBindingDescription;
  vertexInputStateCreateInfo.vertexBindingDescriptionCount = count;
  vertexInputStateCreateInfo.pVertexAttributeDescriptions = vkVertexInputAttributeDescription;
  vertexInputStateCreateInfo.vertexAttributeDescriptionCount = pipelineDesc.vertexInputStateCount;

  pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
  pipelineCreateInfo.pRasterizationState = &rasterizationState;
  pipelineCreateInfo.pColorBlendState = &colorBlendState;
  pipelineCreateInfo.pMultisampleState = &multisampleState;
  pipelineCreateInfo.pViewportState = &viewportState;
  pipelineCreateInfo.pDepthStencilState = &depthStencilState;
  pipelineCreateInfo.pDynamicState = &dynamicState;

  pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
  pipelineCreateInfo.stageCount = shader.count;
  pipelineCreateInfo.pStages = shader.stageCreateInfo;

  Pipeline pipeline = {};
  pipeline.layout = pipelineDesc.state.rasterization.vkPipelineLayout;

  checkResult(vkCreateGraphicsPipelines(mg::vkContext.device, mg::vkContext.pipelineCache, 1, &pipelineCreateInfo,
                                        nullptr, &pipeline.pipeline));
  return pipeline;
}

void PipelineContainer::createPipelineContainer() {
  mg::createShaders();
}

PipelineContainer::~PipelineContainer() {
  mgAssert(_idToPipeline.empty());
}

void PipelineContainer::destroyPipelineContainer() {
  waitForDeviceIdle();
  for (auto pipelineIt : _idToPipeline) {
    vkDestroyPipeline(mg::vkContext.device, pipelineIt.second.pipeline, nullptr);
  }
  _idToPipeline.clear();
  mg::deleteShaders();
}

void PipelineContainer::resetPipelineContainer() {
  waitForDeviceIdle();
  destroyPipelineContainer();
  createPipelineContainer();
}

Pipeline PipelineContainer::createPipeline(const PipelineStateDesc &pipelineDesc,
                                           const CreatePipelineInfo &createPipelineInfo) {
  _PipelineDesc _pipelineDesc = {};
  _pipelineDesc.state = pipelineDesc;
  mgAssert(createPipelineInfo.shaderName.size() + 1 < mg::countof(_pipelineDesc.shaderName));
  mgAssert(createPipelineInfo.vertexInputStateCount < mg::countof(_pipelineDesc.vertexInputState));

  const auto vertexInputState = createPipelineInfo.vertexInputState;

  strncpy(_pipelineDesc.shaderName, createPipelineInfo.shaderName.c_str(), sizeof(_pipelineDesc.shaderName));
  _pipelineDesc.vertexInputStateCount = createPipelineInfo.vertexInputStateCount;
  for (uint32_t i = 0; i < createPipelineInfo.vertexInputStateCount; i++) {
    _pipelineDesc.vertexInputState[i].binding = vertexInputState[i].binding;
    _pipelineDesc.vertexInputState[i].format = vertexInputState[i].format;
    _pipelineDesc.vertexInputState[i].location = vertexInputState[i].location;
    _pipelineDesc.vertexInputState[i].offset = vertexInputState[i].offset;
    _pipelineDesc.vertexInputState[i].size = vertexInputState[i].size;
  }

  auto startAdress = (const unsigned char *)((&_pipelineDesc));
  auto hashValue = hashBytes(startAdress, startAdress + sizeof(_pipelineDesc));

  auto it = _idToPipeline.find(hashValue);
  if (it != std::end(_idToPipeline)) {
    return it->second;
  }
  auto pipeline = _createPipeline(_pipelineDesc);
  _idToPipeline.emplace(hashValue, pipeline);
  return pipeline;
}

Pipeline PipelineContainer::createComputePipeline(const PipelineStateDesc &pipelineDesc,
                                                  const CreateComputePipelineInfo &createComputePipelineInfo) {

  _PipelineDesc _pipelineDesc = {};
  _pipelineDesc.state = pipelineDesc;
  mgAssert(createComputePipelineInfo.shaderName.size() + 1 < mg::countof(_pipelineDesc.shaderName));
  strncpy(_pipelineDesc.shaderName, createComputePipelineInfo.shaderName.c_str(), sizeof(_pipelineDesc.shaderName));

  auto startAdress = (const unsigned char *)((&_pipelineDesc));
  auto hashValue = hashBytes(startAdress, startAdress + sizeof(_pipelineDesc));

  auto it = _idToPipeline.find(hashValue);
  if (it != std::end(_idToPipeline)) {
    return it->second;
  }

  // create new pipeline
  const auto shader = mg::getShader(createComputePipelineInfo.shaderName);

  VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
  shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  mgAssert(shader.count == 1);
  shaderStageCreateInfo.module = shader.stageCreateInfo[0].module;
  shaderStageCreateInfo.pName = "main";

  VkComputePipelineCreateInfo pipelineCreateInfo = {};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.stage = shaderStageCreateInfo;
  pipelineCreateInfo.layout = pipelineDesc.compute.pipelineLayout;

  Pipeline pipeline = {};
  pipeline.layout = pipelineDesc.compute.pipelineLayout;
  checkResult(
      vkCreateComputePipelines(vkContext.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline.pipeline));

  _idToPipeline.emplace(hashValue, pipeline);
  return pipeline;
}

Pipeline PipelineContainer::createRayTracingPipeline(const PipelineStateDesc &pipelineDesc,
                                                     const CreateRayTracingPipelineInfo &createRayTracingPipelineInfo) {
  _PipelineDesc _pipelineDesc = {};
  _pipelineDesc.state = pipelineDesc;
  mgAssert(createRayTracingPipelineInfo.shaderName.size() + 1 < mg::countof(_pipelineDesc.shaderName));
  strncpy(_pipelineDesc.shaderName, createRayTracingPipelineInfo.shaderName.c_str(), sizeof(_pipelineDesc.shaderName));

  auto startAdress = (const unsigned char *)((&_pipelineDesc));
  auto hashValue = hashBytes(startAdress, startAdress + sizeof(_pipelineDesc));

  auto it = _idToPipeline.find(hashValue);
  if (it != std::end(_idToPipeline)) {
    return it->second;
  }

  const auto &shader = mg::getShader(createRayTracingPipelineInfo.shaderName);
  const auto &fToI = shader.fileNameToIndex;
  const auto &rayTracing = pipelineDesc.rayTracing;
  enum { MAX_SHADER_STAGES = 10 };
  VkPipelineShaderStageCreateInfo shaderStageCreateInfo[MAX_SHADER_STAGES];
  mgAssert(MAX_SHADER_STAGES >= rayTracing.shaderCount);
  for (uint32_t i = 0; i < rayTracing.shaderCount; i++) {
    const auto shaderIndex = fToI.at(rayTracing.shaders[i]);
    mgAssert(shaderIndex < shader.count);
    shaderStageCreateInfo[i] = shader.stageCreateInfo[shaderIndex];
  }

  VkRayTracingPipelineCreateInfoNV rayPipelineInfo{};
  rayPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
  rayPipelineInfo.stageCount = pipelineDesc.rayTracing.shaderCount;
  rayPipelineInfo.pStages = shaderStageCreateInfo;
  rayPipelineInfo.pGroups = pipelineDesc.rayTracing.groups;
  rayPipelineInfo.groupCount = pipelineDesc.rayTracing.groupCount;
  rayPipelineInfo.layout = pipelineDesc.rayTracing.pipelineLayout;
  rayPipelineInfo.maxRecursionDepth = 1;

  Pipeline pipeline = {};
  pipeline.layout = pipelineDesc.rayTracing.pipelineLayout;
  checkResult(nv::vkCreateRayTracingPipelinesNV(mg::vkContext.device, VK_NULL_HANDLE, 1, &rayPipelineInfo, nullptr,
                                                &pipeline.pipeline));

  _idToPipeline.emplace(hashValue, pipeline);
  return pipeline;
} // namespace mg

} // namespace mg