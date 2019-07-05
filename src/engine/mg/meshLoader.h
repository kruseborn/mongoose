#pragma once
#include "meshContainer.h"
#include "vulkan/shaderPipelineInput.h"
#include <vector>
#include <glm/glm.hpp>

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

struct ObjMesh {
  mg::MeshId id;
  uint32_t materialId;
};

struct ObjMaterial {
  glm::vec4 diffuse;
};

struct ObjMeshes {
  std::vector<ObjMaterial> materials;
  std::vector<ObjMesh> meshes;
};

GltfMeshes parseGltf(const std::string &id, const std::string &path, const std::string &name);
ObjMeshes loadObjFromFile(const std::string &filename);

} // namespace mg
