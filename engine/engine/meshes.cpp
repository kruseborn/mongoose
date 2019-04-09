#include "meshes.h"
#include "vulkanContext.h"
#include "vkUtils.h"
#include <cstdlib>
#include <cstring>
#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

#pragma warning( disable : 4996)


static void *vnData = malloc(800000000);
static void *propertiesData = malloc(100000000);
static uint32_t vnSize = 0;
static uint32_t pSize = 0;

#include <stdio.h>

void adx_store_data(const char *filepath, const char *data)
{
	FILE *fp = fopen(filepath, "ab");
	if (fp != NULL)
	{
		fputs(data, fp);
		fclose(fp);
	}
}
std::string str;

std::fstream stream("test.txt");
std::fstream stream2("material.txt");


static void addData(void *source, uint32_t size, uint32_t nrOfIndices) {
	memcpy((char*)vnData + vnSize, source, size);
	vnSize += size;
	stream << size << " " << sizeof(uint32_t) * nrOfIndices << " " << nrOfIndices << std::endl;
}
static void addProperties(void *source, uint32_t size) {
	memcpy((char*)propertiesData + pSize, source, size);
	pSize += size;
	stream2 << pSize << std::endl;
}
void storeDataToDisc() {
	FILE *write_ptr;
	write_ptr = fopen("test.bin", "wb");  // w for write, b for binary
	fwrite(vnData, vnSize, 1, write_ptr);
	fclose(write_ptr);

	stream.close();
}


namespace Meshes {
	std::vector<Mesh> _meshes;


	void init() {
		//loadBinaryMeshFromFile("test.bin", "test.txt", "material.bin");
		loadMeshesFromFile("data/rungholt.obj");
	}
	void destroy() {
		for (uint32_t i = 0; i < _meshes.size(); i++) {
			vkDestroyBuffer(vkContext.device, _meshes[i].buffer, nullptr);
			vkFreeMemory(vkContext.device, _meshes[i].memory, nullptr);
		}
		_meshes.clear();
	}

	void insert(void *data, uint32_t verticesSize, uint32_t indicesSize, uint32_t nrOfIndices, MeshProperties &properties) {
		VkBuffer buffer;
		VkDeviceMemory deviceMemory;
		createDeviceVisibleBuffer(data, verticesSize + indicesSize, buffer, deviceMemory, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		Mesh meshBuffer;
		meshBuffer.properties = properties;
		meshBuffer.buffer = buffer;
		meshBuffer.memory = deviceMemory;
		meshBuffer.verticesOffset = 0;
		meshBuffer.indicesOffset = verticesSize;
		meshBuffer.indexCount = nrOfIndices;

		_meshes.push_back(meshBuffer);
	}


	void insert(void *vertices, uint32_t verticesSize, uint32_t *indices, uint32_t nrOfIndices, MeshProperties &properties) {
		uint32_t totalSize = verticesSize + sizeof(uint32_t) * nrOfIndices;
		void *data = malloc(totalSize);
		memcpy(data, vertices, verticesSize);
		memcpy((char*)data + verticesSize, indices, sizeof(uint32_t)*nrOfIndices);
		insert(data, verticesSize, sizeof(uint32_t) * nrOfIndices, nrOfIndices, properties);
		free(data);
	}
	void getMeshes(Mesh **meshes, uint32_t *size) {
		*meshes = _meshes.data();
		*size = uint32_t(_meshes.size());
	}
}