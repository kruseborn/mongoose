#include "pipelines.h"
#include "vulkanContext.h"
#include "shaders.h"
#include "vkUtils.h"
#include "vkDefs.h"
#include <vector>

namespace {
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
	VkPipelineRasterizationStateCreateInfo rasterizationState = {};
	VkPipelineColorBlendAttachmentState blendAttachmentState = {};
	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	VkPipelineViewportStateCreateInfo viewportState = {};
	VkPipelineMultisampleStateCreateInfo multisampleState = {};
#define nrOfDynamicStates 2
	VkDynamicState dynamicStateEnables[nrOfDynamicStates] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState = {};
}

static void initDefaultAttributes() {
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;

	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.depthClampEnable = VK_TRUE;
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.depthBiasEnable = VK_FALSE;
	rasterizationState.depthBiasConstantFactor = 0.000;
	rasterizationState.depthBiasClamp = 0.0f;
	rasterizationState.depthBiasSlopeFactor = 0.0f;
	rasterizationState.lineWidth = 1.0f;

	blendAttachmentState.blendEnable = VK_TRUE;
	blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.logicOpEnable = VK_FALSE;
	colorBlendState.logicOp = VK_LOGIC_OP_COPY;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &blendAttachmentState;

	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.front = depthStencilState.back;
	depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;

	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleState.minSampleShading = 0.0f;

	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pDynamicStates = dynamicStateEnables;
	dynamicState.dynamicStateCount = nrOfDynamicStates;

}

static void createNoDepthTestPipeline(const Shader &shader, VertexInputState &vertexInputState, VkPipeline &pipeline, VkRenderPass renderPass, VkPipelineLayout layout) {
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = layout;
	pipelineCreateInfo.renderPass = renderPass;

	initDefaultAttributes();

	VkPipelineShaderStageCreateInfo shaders[2] = { shader.vertex.shaderStage, shader.fragment.shaderStage };
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = vertexInputState.bindings;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = vertexInputState.nrOfBindings;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputState.attributes;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexInputState.nrOfAttributes;

	depthStencilState.depthTestEnable = VK_FALSE;
	depthStencilState.depthWriteEnable = VK_FALSE;

	blendAttachmentState.blendEnable = false;
	/*blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	*/

	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicState;


	pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaders;

	checkResult(vkCreateGraphicsPipelines(vkContext.device, vkContext.pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
}

static void createPolygonPipeline(const Shader &shader, VertexInputState &vertexInputState, VkPipeline &pipeline, VkRenderPass renderPass, VkPipelineLayout layout) {
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = layout;
	pipelineCreateInfo.renderPass = renderPass;

	initDefaultAttributes();

	VkPipelineShaderStageCreateInfo shaders[2] = { shader.vertex.shaderStage, shader.fragment.shaderStage };
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = vertexInputState.bindings;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = vertexInputState.nrOfBindings;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputState.attributes;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexInputState.nrOfAttributes;

	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicState;


	pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaders;

	checkResult(vkCreateGraphicsPipelines(vkContext.device, vkContext.pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
}

static void createMRTPipeline(const Shader &shader, VertexInputState &vertexInputState, VkPipeline &pipeline) {
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = vkContext.pipelineLayout;
	pipelineCreateInfo.renderPass = vkContext.deferredRenderPass;

	initDefaultAttributes();

	VkPipelineShaderStageCreateInfo shaders[2] = { shader.vertex.shaderStage, shader.fragment.shaderStage };
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = vertexInputState.bindings;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = vertexInputState.nrOfBindings;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputState.attributes;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexInputState.nrOfAttributes;


	VkPipelineColorBlendAttachmentState blendAttachmentStates[3] = {};
	for (uint32_t i = 0; i < 3; i++) {
		blendAttachmentStates[i].blendEnable = VK_FALSE;
		blendAttachmentStates[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentStates[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentStates[i].colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentStates[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentStates[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentStates[i].alphaBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentStates[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}
	colorBlendState.attachmentCount = 3;
	colorBlendState.pAttachments = blendAttachmentStates;

	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicState;


	pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaders;

	checkResult(vkCreateGraphicsPipelines(vkContext.device, vkContext.pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
}

static void createMutltiTexturePipeline(const Shader &shader, VertexInputState &vertexInputState, VkPipeline &pipeline) {
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = vkContext.pipelineLayoutMultiTexture;
	pipelineCreateInfo.renderPass = vkContext.deferredRenderPass;

	initDefaultAttributes();

	VkPipelineShaderStageCreateInfo shaders[2] = { shader.vertex.shaderStage, shader.fragment.shaderStage };
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = vertexInputState.bindings;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = vertexInputState.nrOfBindings;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputState.attributes;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexInputState.nrOfAttributes;

	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicState;

	pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaders;

	checkResult(vkCreateGraphicsPipelines(vkContext.device, vkContext.pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
}

void createPipeline(Pipeline *pipeline, PIPELINES pipelineType, const Shader &shader, VertexInputState &vertexInputState, VkRenderPass renderPass, VkPipelineLayout layout) {
	pipeline->layout = layout;

	switch (pipelineType) {
	case PIPELINES::NO_DEPTH_TEST:
		createNoDepthTestPipeline(shader, vertexInputState, pipeline->pipeline, renderPass, layout);
		break;
	case PIPELINES::POLYGON:
		createPolygonPipeline(shader, vertexInputState, pipeline->pipeline, renderPass, layout);
		break;
	case PIPELINES::MRT:
		createMRTPipeline(shader, vertexInputState, pipeline->pipeline);
		break;
	case PIPELINES::MULTI_TEXTURE:
		createMutltiTexturePipeline(shader, vertexInputState, pipeline->pipeline);
		break;
	default:
		assert(false);
	}
}
