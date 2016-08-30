#include "pipelines.h"
#include <unordered_map>
#include "vulkanContext.h"
#include "cassert"
#include "shaders.h"
#include "vkDefs.h"
#include <string>
namespace Pipelines {
	std::unordered_map<std::string, Pipeline> pipelines;
	void init() {

		createShaders({ "drawText", "solid", "text", "mrt",	"deferred", "ssao", "ssaoBlur", "depth" });

		createPipeline(newPipeline("polygon"), PIPELINES::POLYGON, getShader("solid"), basicInputState, vkContext.deferredRenderPass, vkContext.pipelineLayout);
		createPipeline(newPipeline("text"), PIPELINES::NO_DEPTH_TEST, getShader("text"), positionInputState, vkContext.deferredRenderPass, vkContext.pipelineLayout);
		createPipeline(newPipeline("depth"), PIPELINES::NO_DEPTH_TEST, getShader("depth"), positionInputState, vkContext.deferredRenderPass, vkContext.pipelineLayout);
		createPipeline(newPipeline("mrt"), PIPELINES::MRT, getShader("mrt"), basicInputState, vkContext.deferredRenderPass, vkContext.pipelineLayout);
		createPipeline(newPipeline("deferred"), PIPELINES::MULTI_TEXTURE, getShader("deferred"), positionInputState, vkContext.deferredRenderPass, vkContext.pipelineLayoutMultiTexture);
		createPipeline(newPipeline("ssao"), PIPELINES::MULTI_TEXTURE, getShader("ssao"), positionInputState, vkContext.deferredRenderPass, vkContext.pipelineLayoutMultiTexture);
		createPipeline(newPipeline("ssaoBlur"), PIPELINES::MULTI_TEXTURE, getShader("ssaoBlur"), positionInputState, vkContext.deferredRenderPass, vkContext.pipelineLayoutMultiTexture);

		createPipeline(newPipeline("drawText"), PIPELINES::NO_DEPTH_TEST, getShader("drawText"), positionInputState, vkContext.deferredRenderPass, vkContext.pipelineLayout);


		deleteShaders();
	}

	void deletePipeline(Pipeline pipeline) {
		vkDestroyPipeline(vkContext.device, pipeline.pipeline, nullptr);
	}
	void destroy() {
		for (auto &pipeline : pipelines) {
			deletePipeline(pipeline.second);
		}
		pipelines.clear();
	}

	Pipeline *findPipeline(const char* name) {
		auto res = pipelines.find(name);
		assert(res != std::end(pipelines));
		return &res->second;
	}
	Pipeline* newPipeline(const char *name) {
		uint32_t strLenght = (uint32_t)strlen(name);
		assert(strLenght < MAX_PIPELINE_NAME_SIZE);
		Pipeline pipeline = {};
		strcpy_s(pipeline.name, strLenght + 1, name);
		pipelines.emplace(name, pipeline);
		return &pipelines.find(pipeline.name)->second;
	}
	void freePipeline(const char* name) {
		auto res = pipelines.find(name);
		assert(res != std::end(pipelines));
		deletePipeline(res->second);
		pipelines.erase(res);
	}
}