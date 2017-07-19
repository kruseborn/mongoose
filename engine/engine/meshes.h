#pragma once
#include <cstdint>
#include "vulkan/vulkan.h"
#include <vector>
#include "glm/glm.hpp"

struct MeshProperties {
	glm::vec4 diffuse, specular;
};
struct Mesh {
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDeviceSize verticesOffset;
	VkDeviceSize indicesOffset;
	uint32_t indexCount;

	MeshProperties properties;
};

namespace Meshes {
	void init();
	void destroy();

	void insert(void *data, uint32_t verticesSize, uint32_t indicesSize, uint32_t nrOfIndices, MeshProperties &properties);
	void insert(void *vertices, uint32_t vertexSize, uint32_t *indices, uint32_t nrOfIndices, MeshProperties &properties);
	void getMeshes(Mesh **meshes, uint32_t *size);
}
void storeDataToDisc();

void loadMeshesFromFile(const char *fileName);
void loadBinaryMeshFromFile(const char *fileName, const char *offset, const char* material);


