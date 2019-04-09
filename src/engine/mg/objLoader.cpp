#include "meshLoader.h"

#include "mg/meshContainer.h"
#include "mg/mgSystem.h"
#include <assimp/Importer.hpp>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <fstream>

namespace mg {

struct Outputs {
  std::ofstream binary, sizes, properties;
};

struct BasicVertex {
  glm::vec3 position;
  glm::vec3 normals;
};

static std::tuple<mg::MeshId, mg::MeshProperties> createMesh(uint32_t index, const aiMesh &paiMesh, const aiScene &pScene, Outputs *outputs) {
  aiColor3D diffuseColor, ambiet, specular;
  std::string name;
  pScene.mMaterials[paiMesh.mMaterialIndex]->Get(AI_MATKEY_NAME, name);
  pScene.mMaterials[paiMesh.mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
  pScene.mMaterials[paiMesh.mMaterialIndex]->Get(AI_MATKEY_COLOR_AMBIENT, ambiet);
  pScene.mMaterials[paiMesh.mMaterialIndex]->Get(AI_MATKEY_COLOR_SPECULAR, specular);

  mg::MeshProperties meshProperties= {};
  meshProperties.diffuse = {diffuseColor.r, diffuseColor.g, diffuseColor.b, 1.0};
  //meshProperties.specular = {specular.r, specular.g, specular.b, 1.0};

  const uint32_t numberOfVertices = paiMesh.mNumVertices;
  assert(sizeof(glm::vec3) == sizeof(aiVector3D));
  std::vector<BasicVertex> vertices(numberOfVertices);

  aiVector3D *pPos = &paiMesh.mVertices[0];
  aiVector3D *pNormal = &paiMesh.mNormals[0];
  aiVector3D *pTexture = &paiMesh.mTextureCoords[0][0];
  for (uint32_t i = 0; i < numberOfVertices; i++) {
    vertices[i].position = {pPos[i].x, pPos[i].y, pPos[i].z};
    vertices[i].normals = {pNormal[i].x, pNormal[i].y, pNormal[i].z};
    // vertices[i].textCoordinate = { pTexture[i].x, pTexture[i].y, pTexture[i].z};
  }

  std::vector<uint32_t> indices;
  for (unsigned int i = 0; i < paiMesh.mNumFaces; i++) {
    const aiFace &Face = paiMesh.mFaces[i];
    if (Face.mNumIndices != 3)
      continue;
    indices.push_back(Face.mIndices[0]);
    indices.push_back(Face.mIndices[1]);
    indices.push_back(Face.mIndices[2]);
  }

  if (outputs->binary.is_open()) {
    outputs->binary.write((char *)vertices.data(), mg::sizeofContainerInBytes(vertices));

    outputs->sizes << mg::sizeofContainerInBytes(vertices) << '/';

    outputs->binary.write((char *)indices.data(), mg::sizeofContainerInBytes(indices));
    outputs->sizes << mg::sizeofContainerInBytes(indices) << '/';
  }

  mg::CreateMeshInfo createMeshInfo = {};
  createMeshInfo.id = "mesh" + std::to_string(index);
  createMeshInfo.vertices = (uint8_t *)vertices.data();
  createMeshInfo.indices = (uint8_t *)indices.data();
  createMeshInfo.verticesSizeInBytes = mg::sizeofContainerInBytes(vertices);
  createMeshInfo.indicesSizeInBytes = mg::sizeofContainerInBytes(indices);
  createMeshInfo.nrOfIndices = indices.size();
  const auto meshId = mg::mgSystem.meshContainer.createMesh(createMeshInfo);

  return {meshId, meshProperties};
}

static ObjMeshes parseScene(const aiScene *pScene, const std::string &fileName, bool saveSceneToFile) {
  ObjMeshes objMeshes = {};
  Outputs outputs = {};
  if (saveSceneToFile) {
    outputs.binary.open(fileName + ".bin", std::fstream::binary);
    outputs.sizes.open(fileName + ".sizes.txt");
    outputs.properties.open(fileName + ".mat", std::fstream::binary);
  }

  for (uint32_t i = 0; i < pScene->mNumMeshes; i++) {
    const aiMesh *paiMesh = pScene->mMeshes[i];
    auto [meshId, meshProperties] = createMesh(i, *paiMesh, *pScene, &outputs);
    objMeshes.ids.push_back(meshId);
    objMeshes.meshProperties.emplace_back(std::move(meshProperties));
  }

  if (saveSceneToFile)
    outputs.properties.write((char *)objMeshes.meshProperties.data(), mg::sizeofContainerInBytes(objMeshes.meshProperties));
  return objMeshes;
}
ObjMeshes loadMeshesFromFile(const std::string path, const std::string &fileName, bool saveToFile) {
  Assimp::Importer Importer = {};
  int flags = aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;
  const aiScene *scene = Importer.ReadFile(path + fileName, flags);
  mgAssert(scene != nullptr);
  return parseScene(scene, fileName, saveToFile);
}

mg::ObjMeshes loadMeshesFromBinary(const std::string path, const std::string &fileName) {
  mg::ObjMeshes objMeshes = {};

  const auto fullPath = path + fileName;

  const auto binary = mg::readBinaryFromDisc(fullPath + ".bin");
  const auto materials = mg::readBinaryFromDisc(fullPath + ".mat");

  objMeshes.meshProperties.resize(materials.size() / sizeof(MeshProperties));
  objMeshes.ids.reserve(objMeshes.meshProperties.size());
  std::memcpy(objMeshes.meshProperties.data(), materials.data(), mg::sizeofContainerInBytes(materials));

  std::ifstream offsets(fullPath + ".sizes.txt");

  const auto nrOfMeshes = materials.size() / sizeof(MeshProperties);
  uint32_t currentOffset = 0;
  for (uint32_t i = 0; i < nrOfMeshes; i++) {
    uint32_t vertexsize, indicesSize;
    char delimiter;
    offsets >> vertexsize >> delimiter >> indicesSize >> delimiter;
    mgAssert((currentOffset + vertexsize + indicesSize)  <= binary.size());
    mg::CreateMeshInfo createMeshInfo = {};
    createMeshInfo.id = "mesh" + std::to_string(i);
    createMeshInfo.vertices = (unsigned char *)&binary[currentOffset];
    createMeshInfo.indices = (unsigned char *)&binary[currentOffset + vertexsize];
    createMeshInfo.verticesSizeInBytes = vertexsize;
    createMeshInfo.indicesSizeInBytes = indicesSize;
    createMeshInfo.nrOfIndices = indicesSize / sizeof(uint32_t);

    currentOffset += vertexsize + indicesSize;

    const auto meshId = mg::mgSystem.meshContainer.createMesh(createMeshInfo);
    objMeshes.ids.push_back(meshId);
  }
  return objMeshes;
}

} // namespace mg