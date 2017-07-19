#pragma once
#include "vulkan/vulkan.h"
#include "glm/glm.hpp"
namespace glm {

}
struct VertexInputState {
	VkVertexInputBindingDescription *bindings;
	uint32_t nrOfBindings;
	VkVertexInputAttributeDescription *attributes;
	uint32_t nrOfAttributes;
};

struct PositionVertex {
	glm::vec4 position;
};
struct BasicVertex {
	glm::vec3 position;
	glm::vec3 normals;
	glm::vec2 textCoordinate;
};

extern VertexInputState basicInputState;
extern VertexInputState positionInputState;
extern float realtime, deltaTime;
extern uint32_t framecount;

