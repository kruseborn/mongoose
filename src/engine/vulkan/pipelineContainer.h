#pragma once
#include "../mg/mgUtils.h"
#include "shaderPipelineInput.h"
#include "vkContext.h"

#include "mg/mgAssert.h"
#include <unordered_map>

namespace mg {

struct PipelineStateDesc {
  PipelineStateDesc();
  PipelineStateDesc(const PipelineStateDesc &other);
  PipelineStateDesc &operator=(const PipelineStateDesc &other);
  struct {
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
  } rasterization;

  struct {
    VkPipelineLayout pipelineLayout;
  } compute;

  struct {
    enum { MAX_GROUPS = 10 };
    enum { MAX_SHADERS = 10 };
    uint32_t depth;
    VkPipelineLayout pipelineLayout;
    uint32_t groupCount;
    uint32_t shaderCount;
    VkRayTracingShaderGroupCreateInfoNV groups[MAX_GROUPS];
    char shaders[MAX_SHADERS][70];
  } rayTracing;
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

struct CreateComputePipelineInfo {
  std::string shaderName;
};

struct CreateRayTracingPipelineInfo {
  std::string shaderName;
};

class PipelineContainer : mg::nonCopyable {
public:
  void createPipelineContainer();
  void destroyPipelineContainer();

  void resetPipelineContainer();

  Pipeline createPipeline(const PipelineStateDesc &pipelineDesc, const CreatePipelineInfo &createPipelineInfo);
  Pipeline createComputePipeline(const PipelineStateDesc &pipelineDesc,
                                 const CreateComputePipelineInfo &createComputePipelineInfo);
  Pipeline createRayTracingPipeline(const PipelineStateDesc &pipelineDesc,
                                    const CreateRayTracingPipelineInfo &CreateRayTracingPipelineInfo);
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

} // namespace mg
