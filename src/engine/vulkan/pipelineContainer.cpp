#include "pipelineContainer.h"

#include "../mg/mgSystem.h"
#include "shaderPipelineInput.h"
#include "shaders.h"
#include "singleRenderpass.h"
#include "vkContext.h"
#include "vkUtils.h"
#include <cassert>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>


static mg::Shaders shaderNames() {
  mg::Shaders shaders = {};
  shaders.graphics.reserve(20);
  shaders.computes.reserve(20);

  namespace fs = std::filesystem;
  const auto shaderPath = mg::getShaderPath() + "../";
  for (auto &file : fs::directory_iterator(shaderPath)) {
    const auto path = fs::path(file);
    if (path.extension().generic_string() == ".comp") {
      shaders.computes.push_back(path.filename().generic_string());
    } else if (path.extension().generic_string() == ".glsl") {
        std::string str = path.filename().generic_string();
        str.resize(str.size() - 5); // remove extension and .
        shaders.graphics.push_back(str);
    } 
  }
  return shaders;
}

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

_PipelineDesc::_PipelineDesc() { memset(this, 0, sizeof(_PipelineDesc)); }
_PipelineDesc::_PipelineDesc(const _PipelineDesc &other) { memcpy(this, &other, sizeof(_PipelineDesc)); }
_PipelineDesc &_PipelineDesc::operator=(const _PipelineDesc &other) {
  memcpy(this, &other, sizeof(_PipelineDesc));
  return *this;
}

PipelineStateDesc::PipelineStateDesc() {
  memset(this, 0, sizeof(PipelineStateDesc));
  inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  rasterization.polygonMode = VK_POLYGON_MODE_FILL;
  rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  blend.blendEnable = VK_TRUE;
  blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  blend.colorBlendOp = VK_BLEND_OP_ADD;
  blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  blend.alphaBlendOp = VK_BLEND_OP_ADD;

  depth.TestEnable = VK_TRUE;
  depth.writeEnable = VK_TRUE;
  depth.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

  graphics.subpass = 0;
  graphics.nrOfColorAttachments = 1;
}

PipelineStateDesc::PipelineStateDesc(const PipelineStateDesc &other) { memcpy(this, &other, sizeof(PipelineStateDesc)); }
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
  inputAssemblyState.topology = pipelineDesc.state.inputAssemblyState.topology;
  inputAssemblyState.primitiveRestartEnable = VK_FALSE;

  rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizationState.depthClampEnable = VK_FALSE;
  rasterizationState.rasterizerDiscardEnable = VK_FALSE;
  rasterizationState.polygonMode = pipelineDesc.state.rasterization.polygonMode;
  rasterizationState.cullMode = pipelineDesc.state.rasterization.cullMode;
  rasterizationState.frontFace = pipelineDesc.state.rasterization.frontFace;
  rasterizationState.depthBiasEnable = VK_FALSE;
  rasterizationState.depthBiasConstantFactor = 0.0f;
  rasterizationState.depthBiasClamp = 0.0f;
  rasterizationState.depthBiasSlopeFactor = 0.0f;
  rasterizationState.lineWidth = 1.0f;

  for (uint32_t i = 0; i < pipelineDesc.state.graphics.nrOfColorAttachments; i++) {
    blendAttachmentState[i].blendEnable = pipelineDesc.state.blend.blendEnable;
    blendAttachmentState[i].srcColorBlendFactor = pipelineDesc.state.blend.srcColorBlendFactor;
    blendAttachmentState[i].dstColorBlendFactor = pipelineDesc.state.blend.dstColorBlendFactor;
    blendAttachmentState[i].colorBlendOp = pipelineDesc.state.blend.colorBlendOp;
    blendAttachmentState[i].srcAlphaBlendFactor = pipelineDesc.state.blend.srcAlphaBlendFactor;
    blendAttachmentState[i].dstAlphaBlendFactor = pipelineDesc.state.blend.dstAlphaBlendFactor;
    blendAttachmentState[i].alphaBlendOp = pipelineDesc.state.blend.alphaBlendOp;
    blendAttachmentState[i].colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  }

  colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendState.logicOpEnable = VK_FALSE;
  colorBlendState.logicOp = VK_LOGIC_OP_COPY;
  colorBlendState.attachmentCount = pipelineDesc.state.graphics.nrOfColorAttachments;
  colorBlendState.pAttachments = blendAttachmentState.data();

  depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilState.depthTestEnable = pipelineDesc.state.depth.TestEnable;
  depthStencilState.depthWriteEnable = pipelineDesc.state.depth.writeEnable;
  depthStencilState.depthCompareOp = pipelineDesc.state.depth.DepthCompareOp;
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
  pipelineCreateInfo.layout = pipelineDesc.state.vkPipelineLayout;
  pipelineCreateInfo.renderPass = pipelineDesc.state.vkRenderPass;
  pipelineCreateInfo.subpass = pipelineDesc.state.graphics.subpass;

  const auto shader = mg::getShader(pipelineDesc.shaderName);
  VkPipelineShaderStageCreateInfo shaders[] = {shader.vertex.shaderStage, shader.fragment.shaderStage};

  VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
  vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  constexpr uint32_t maxNrOfInputVertexAttribute = 10;
  mgAssert(pipelineDesc.vertexInputStateCount <= maxNrOfInputVertexAttribute);
  VkVertexInputAttributeDescription vkVertexInputAttributeDescription[maxNrOfInputVertexAttribute] = {};

  uint32_t strideInterleaved = 0;
  for (uint32_t i = 0; i < pipelineDesc.vertexInputStateCount; i++) {
    vkVertexInputAttributeDescription[i].binding = pipelineDesc.vertexInputState[i].binding;
    vkVertexInputAttributeDescription[i].format = pipelineDesc.vertexInputState[i].format;
    vkVertexInputAttributeDescription[i].offset = pipelineDesc.vertexInputState[i].offset;
    vkVertexInputAttributeDescription[i].location = pipelineDesc.vertexInputState[i].location;
    strideInterleaved += pipelineDesc.vertexInputState[i].size;
  }

  VkVertexInputBindingDescription vkVertexInputBindingDescription = {};
  vkVertexInputBindingDescription.binding = 0;
  vkVertexInputBindingDescription.stride = strideInterleaved;
  vkVertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  vertexInputStateCreateInfo.pVertexBindingDescriptions = &vkVertexInputBindingDescription;
  vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
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
  pipelineCreateInfo.stageCount = mg::countof(shaders);
  pipelineCreateInfo.pStages = shaders;

  Pipeline pipeline = {};
  pipeline.layout = pipelineDesc.state.vkPipelineLayout;

  checkResult(vkCreateGraphicsPipelines(mg::vkContext.device, mg::vkContext.pipelineCache, 1, &pipelineCreateInfo, nullptr,
                                        &pipeline.pipeline));
  return pipeline;
}

// hard coded for now, this should be created with a hashmap or something similar
VkPipelineLayout getPipelineLayout(mg::shaders::Resources *resources, uint32_t resourcesCount) {
  mgAssert(resourcesCount > 0);
  for (uint32_t i = 0; i < resourcesCount; i++) {
    if (resources[i] == mg::shaders::Resources::SSBO)
      return mg::vkContext.pipelineLayouts.pipelineStorageLayout;
  }
  if (resourcesCount > 3)
    return mg::vkContext.pipelineLayouts.pipelineLayoutMultiTexture;
  else
    return mg::vkContext.pipelineLayouts.pipelineLayout;
}

void PipelineContainer::createPipelineContainer() { 
  const auto shaders = shaderNames();
  mg::createShaders(shaders.graphics); 
  mg::createComputeShaders(shaders.computes);
}

PipelineContainer::~PipelineContainer() { mgAssert(_idToPipeline.empty()); }

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

Pipeline PipelineContainer::createPipeline(const PipelineStateDesc &pipelineDesc, const CreatePipelineInfo &createPipelineInfo) {
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

Pipeline PipelineContainer::createComputePipeline(const ComputePipelineStateDesc &computePipelineStateDesc) {
  _PipelineDesc _pipelineDesc = {};
  mgAssert(computePipelineStateDesc.shaderName.size() + 1 < mg::countof(_pipelineDesc.shaderName));
  strncpy(_pipelineDesc.shaderName, computePipelineStateDesc.shaderName.c_str(), sizeof(_pipelineDesc.shaderName));

  auto startAdress = (const unsigned char *)((&_pipelineDesc));
  auto hashValue = hashBytes(startAdress, startAdress + sizeof(_pipelineDesc));

  auto it = _idToPipeline.find(hashValue);
  if (it != std::end(_idToPipeline)) {
    return it->second;
  }
  
  // create new pipeline
  const auto shader = mg::getShader(computePipelineStateDesc.shaderName);

  VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
  shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageCreateInfo.module = shader.compute.shaderStage.module;
  shaderStageCreateInfo.pName = "main";

  VkComputePipelineCreateInfo pipelineCreateInfo = {};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.stage = shaderStageCreateInfo;
  pipelineCreateInfo.layout = computePipelineStateDesc.pipelineLayout;

  Pipeline pipeline = {};
  pipeline.layout = computePipelineStateDesc.pipelineLayout;
  checkResult(vkCreateComputePipelines(vkContext.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline.pipeline));

  _idToPipeline.emplace(hashValue, pipeline);
  return pipeline;
}
} // namespace mg