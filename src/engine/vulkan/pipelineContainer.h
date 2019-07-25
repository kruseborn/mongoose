#pragma once
#include "vkContext.h"
#include "shaderPipelineInput.h"
#include "../mg/mgUtils.h"

#include "mg/mgAssert.h"
#include <unordered_map>

namespace mg {

struct PipelineStateDesc {
  PipelineStateDesc();
  PipelineStateDesc(const PipelineStateDesc &other);
  PipelineStateDesc & operator=(const PipelineStateDesc &other);

  VkRenderPass vkRenderPass;
  VkPipelineLayout vkPipelineLayout;
  struct {
    VkPrimitiveTopology topology;
  } inputAssemblyState;
  struct {
    VkPolygonMode polygonMode;
    VkCullModeFlags cullMode;
    VkFrontFace frontFace;
  } rasterization;
  struct {
    VkBool32 blendEnable;
    VkBlendFactor srcColorBlendFactor;
    VkBlendFactor dstColorBlendFactor;
    VkBlendOp colorBlendOp;
    VkBlendFactor srcAlphaBlendFactor;
    VkBlendFactor dstAlphaBlendFactor;
    VkBlendOp alphaBlendOp;
  } blend;
  struct {
    VkBool32 TestEnable;
    VkBool32 writeEnable;
    VkCompareOp DepthCompareOp;
  } depth;
  struct {
    uint32_t subpass;
    uint32_t nrOfColorAttachments;
  } graphics;
};

struct ComputePipelineStateDesc {
  std::string shaderName;
  VkPipelineLayout pipelineLayout;
};

struct Pipeline {
  VkPipeline pipeline;
  VkPipelineLayout layout;
};

struct CreatePipelineInfo {
  std::string shaderName;
  mg::shaders::VertexInputState *vertexInputState;
  uint32_t vertexInputStateCount;
};

class PipelineContainer : mg::nonCopyable {
public:
  void createPipelineContainer();
  void destroyPipelineContainer();

  void resetPipelineContainer();

  Pipeline createPipeline(const PipelineStateDesc &pipelineDesc, const CreatePipelineInfo &createPipelineInfo);
  Pipeline createComputePipeline(const ComputePipelineStateDesc &computePipelineStateDesc);
  ~PipelineContainer();

private:
  std::unordered_map<uint64_t, Pipeline> _idToPipeline;
};

struct Pipelines {
  struct {
    Pipeline frontAndBack;
    Pipeline volume;
    Pipeline volumePostProcess;
    Pipeline font;
    Pipeline rayMan;
  } volumeRendering;

  struct {
    Pipeline reflection;
    Pipeline rayMan;
    Pipeline font;
    Pipeline solidTriangle;
    Pipeline solidLinelist;
    Pipeline smoothLineList;
    Pipeline measurementLineList;
    Pipeline colorTable;
  } ui;
  Pipeline pois2D;
  Pipeline pois3D;
  Pipeline phong;
  Pipeline patientPlane;
  Pipeline patientPlaneRGBA;
  Pipeline patientPlaneDose;
};

} // namespace

