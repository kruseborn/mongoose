#include "meshes.h"

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <vector>
#include <glm/glm.hpp>
#include "meshes.h"
#include "vulkanContext.h"
#include "vkDefs.h"
#include <cstdlib>
#include "utils.h"
#include <sstream>
#include <algorithm>
Assimp::Importer Importer;
#pragma warning( disable : 4996)

static void InitMesh(unsigned int index, const aiMesh* paiMesh, const aiScene* pScene) {
	aiColor3D diffuseColor, ambiet, specular;
	std::string name;
	pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_NAME, name);
	pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
	pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_AMBIENT, ambiet);
	pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_SPECULAR, specular);

	MeshProperties properties;
	properties.diffuse = { diffuseColor.r, diffuseColor.g, diffuseColor.b, 1.0 };
//	properties.ambient = { ambiet.r, ambiet.g, ambiet.b, 1.0 };
	properties.specular = { specular.r, specular.g, specular.b, 1.0 };
//	properties.opacity = 1.0;

	const uint32_t numberOfVertices = paiMesh->mNumVertices;
	assert(sizeof(glm::vec3) == sizeof(aiVector3D));
	std::vector<BasicVertex> vertices(numberOfVertices);

	aiVector3D *pPos = &paiMesh->mVertices[0];
	aiVector3D *pNormal = &paiMesh->mNormals[0];
	aiVector3D *pTexture = &paiMesh->mTextureCoords[0][0];
	for (uint32_t i = 0; i < numberOfVertices; i++) {
		vertices[i].position = { pPos[i].x, pPos[i].y, pPos[i].z };
		vertices[i].normals = { pNormal[i].x, pNormal[i].y, pNormal[i].z };
		//vertices[i].textures = { pTexture[i].x, pTexture[i].y, pTexture[i].z };
	}

	std::vector<uint32_t> indices;
	for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
		const aiFace& Face = paiMesh->mFaces[i];
		if (Face.mNumIndices != 3)
			continue;
		indices.push_back(Face.mIndices[0]);
		indices.push_back(Face.mIndices[1]);
		indices.push_back(Face.mIndices[2]);
	}
	
	Meshes::insert(vertices.data(), numberOfVertices * sizeof(BasicVertex), indices.data(), uint32_t(indices.size()), properties);
}
static bool InitFromScene(const aiScene* pScene) {
	for (uint32_t i = 0; i < pScene->mNumMeshes; i++) {
		const aiMesh* paiMesh = pScene->mMeshes[i];
		InitMesh(i, paiMesh, pScene);
	}
	return true;
}
void loadMeshesFromFile(const char *fileName) {
	const aiScene* pScene = nullptr;
	int flags = aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;
	pScene = Importer.ReadFile(fileName, flags);
	InitFromScene(pScene);
	//storeDataToDisc();
}
void loadBinaryMeshFromFile(const char *fileName, const char *offsets, const char *materialFileName) {
	FILE* file = fopen(offsets, "r");
	assert(file);
	char line[256];
	std::vector<uint32_t> bufferOffset, sizeIndices, nrOfIndices;

	while (fgets(line, sizeof(line), file)) {
		std::stringstream stream(line);
		uint32_t size, indicesSize, indices;
		stream >> size >> indicesSize >> indices;
		bufferOffset.push_back(size);
		sizeIndices.push_back(indicesSize);
		nrOfIndices.push_back(indices);
	}

	void *content = readBinaryDataFromFile(fileName);
	char *dataPtr = (char*)content;
	void *material = readBinaryDataFromFile(materialFileName);
	MeshProperties *materialPtr = (MeshProperties*)material;

	uint32_t currentOffset = 0;
	for (uint32_t i = 0; i < bufferOffset.size(); i++) {
		
		void *pp = dataPtr + currentOffset;
		BasicVertex* bs = (BasicVertex*)pp;
		uint32_t size = (bufferOffset[i] - sizeIndices[i]) / sizeof(BasicVertex);
		Meshes::insert(dataPtr + currentOffset, bufferOffset[i] - sizeIndices[i], sizeIndices[i], nrOfIndices[i], *materialPtr);
		currentOffset += bufferOffset[i];
		materialPtr++;
	}


	free(content);
	free(material);
}
