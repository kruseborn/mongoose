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

struct TinyObjMesh {
  mg::MeshId id;
  uint32_t numTriangles;
  uint32_t material_id;
};

struct TinyObjMeshes {
  std::vector<tinyobj::material_t> materials;
  std::vector<TinyObjMesh> meshes;
};

GltfMeshes parseGltf(const std::string &id, const std::string &path, const std::string &name);
TinyObjMeshes loadObjFromFile(const std::string &fileName);

} // namespace mg
