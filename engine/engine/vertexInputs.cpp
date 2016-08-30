#include "vkDefs.h"


struct VertexPositionInfo {
	static const uint32_t nrBindings = 1;
	VkVertexInputBindingDescription bindings[nrBindings] = {
		{ 0u, sizeof(PositionVertex), VK_VERTEX_INPUT_RATE_VERTEX }
	};
	static const uint32_t nrAttributes = 1;
	VkVertexInputAttributeDescription attributes[nrAttributes] = {
		{ 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 },
	};
};
struct BasicVertexInfo {
	static const uint32_t nrBindings = 1;
	VkVertexInputBindingDescription bindings[nrBindings] = {
		{ 0u, sizeof(BasicVertex), VK_VERTEX_INPUT_RATE_VERTEX }
	};
	static const uint32_t nrAttributes = 3;
	VkVertexInputAttributeDescription attributes[nrAttributes] = {
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
		{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(glm::vec3) },
		{ 2, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec3) + sizeof(glm::vec3) }
	};
};

static BasicVertexInfo basicVertexInfo;
static VertexPositionInfo positionVertexInfo;

VertexInputState positionInputState = {
	positionVertexInfo.bindings,
	positionVertexInfo.nrBindings,
	positionVertexInfo.attributes,
	positionVertexInfo.nrAttributes,
};
VertexInputState basicInputState = {
	basicVertexInfo.bindings,
	basicVertexInfo.nrBindings,
	basicVertexInfo.attributes,
	basicVertexInfo.nrAttributes,
};