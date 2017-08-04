#pragma once
#include "vulkan/vulkan.h"

#define MAX_PIPELINE_NAME_SIZE 16
struct Pipeline {
	char name[MAX_PIPELINE_NAME_SIZE];
	VkPipeline pipeline;
	VkPipelineLayout layout;
};

struct Pipeline;
namespace Pipelines {
	void init();
	void destroy();

	Pipeline* findPipeline(const char *name);
	Pipeline* newPipeline(const char *name);
	void freePipeline(const char *name);
}

enum PIPELINES { POLYGON, NO_DEPTH_TEST, MRT, MULTI_TEXTURE };
struct Shader;
struct VertexInputState;
void createPipeline(Pipeline *pipeline, PIPELINES pipelineType, const Shader &shader, VertexInputState &vertexInputState, VkRenderPass renderPass, VkPipelineLayout layout);
