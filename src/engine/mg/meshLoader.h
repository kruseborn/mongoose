#pragma once
#include "meshContainer.h"
#include "vulkan/shaderPipelineInput.h"
#include <vector>

namespace mg {
struct Attribute {
  std::string name;
  VkFormat format;
};

struct GltfMesh {
  std::string id;
  std::vector<float> vertices;
  std::vector<Attribute> attributes;
  uint32_t count;
  uint32_t materialIndex;
  uint32_t textureIndex;
};

struct ImageData {
  uint32_t width, height;
  uint32_t components;
  std::string name;
  std::string path;
};

struct GltfMeshes {
  std::string id;
  std::vector<GltfMesh> meshes;
  std::vector<ImageData> images;
};

struct MeshProperties {
  glm::vec4 diffuse;
};

struct ObjMeshes {
  std::vector<MeshProperties> meshProperties;
  std::vector<mg::MeshId> ids;
};

GltfMeshes parseGltf(const std::string &id, const std::string &path, const std::string &name);
ObjMeshes loadMeshesFromFile(const std::string path, const std::string &fileName, bool saveToFile);
ObjMeshes loadMeshesFromBinary(const std::string path, const std::string &fileName);

} // namespace mg
