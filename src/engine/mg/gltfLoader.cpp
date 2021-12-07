#include "meshLoader.h"

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION // optional. disable exception handling.

#include "mg/mgAssert.h"
#include "vulkan/shaderPipelineInput.h"
#include <array>
#include <glm/glm.hpp>
#include <optional>
#include <string_view>
#include <tiny_gltf.h>
#include <tuple>

struct VertexData {
  std::vector<uint8_t> buffer;
  VkFormat format;
  uint32_t size;
};

struct IndicesData {
  VkIndexType indexType;
  std::vector<uint8_t> indices;
};

struct VertexDatas {
  VertexData positions, normals, textCoords, tangents;
};

struct Primitive {
  int32_t materialIndex;
  int32_t textureIndex;
  VertexDatas vertexDatas;
  IndicesData indicesData;
};

struct InternalMesh {
  std::vector<Primitive> primitives;
};

struct Material {
  std::string name;
  uint32_t textureIndex;
  glm::vec4 colorFactor;
  float factor;
};
struct TextureData {
  std::string name;
  uint32_t source;
};

struct _GltfMesh {
  std::string id;
  std::vector<std::vector<unsigned char>> buffers;
  std::vector<InternalMesh> internalMeshes;
};

template <class T> static std::optional<Material> getMaterial(const T &values, const std::string &name) {
  const auto it = values.find(name);
  if (it == std::end(values))
    return std::nullopt;
  Material material = {};
  material.name = name;
  material.textureIndex = it->second.TextureIndex();
  if (it->second.number_array.size()) {
    const auto &cf = it->second.ColorFactor();
    material.colorFactor = glm::vec4{float(cf[0]), float(cf[1]), float(cf[2]), float(cf[3])};
  }
  if (it->second.has_number_value)
    material.factor = float(it->second.Factor());
  return material;
}

template <class T> static int32_t getAttributeIndex(const T &values, const std::string &name) {
  const auto it = values.find(name);
  if (it == std::end(values))
    return -1;
  return it->second;
}

static std::array<std::string, 11> materialNames = {
    "baseColorTexture", "metallicRoughnessTexture", "roughnessFactor",  "metallicFactor", "baseColorFactor",
    "normalTexture",    "emissiveTexture",          "occlusionTexture", "alphaMode",      "alphaCutoff",
    "emissiveFactor"};
static std::array<std::string, 4> extensionNames = {"specularGlossinessTexture", "diffuseTexture", "diffuseFactor",
                                                    "specularFactor"};

static std::vector<Material> parseMaterials(const tinygltf::Model &model) {
  std::vector<Material> materials;
  for (const auto &modelMaterial : model.materials) {
    for (const auto &materialName : materialNames) {
      const auto material = getMaterial(modelMaterial.values, materialName);
      if (material)
        materials.push_back(material.value());
    }
    for (const auto &extensionName : extensionNames) {
      const auto material = getMaterial(modelMaterial.values, extensionName);
      if (material)
        materials.push_back(material.value());
    }
  }
  return materials;
}

static std::vector<TextureData> parseTextures(const tinygltf::Model &model) {
  std::vector<TextureData> textures;
  for (const auto &modelTexture : model.textures) {
    TextureData texture = {};
    texture.name = modelTexture.name;
    texture.source = modelTexture.source;
  }
  return textures;
}

static std::vector<mg::ImageData> parseImages(const tinygltf::Model &model) {
  std::vector<mg::ImageData> imageDatas;
  for (const auto &modelImage : model.images) {
    mg::ImageData image = {};
    image.name = modelImage.uri;
    image.width = modelImage.width;
    image.height = modelImage.height;
    image.components = modelImage.component;
    imageDatas.push_back(image);
  }
  return imageDatas;
}

static IndicesData parseIndices(const tinygltf::Model &model, const tinygltf::Primitive &primitive) {
  IndicesData indicesData = {};
  const auto &accessor = model.accessors[primitive.indices];
  auto bufferView = model.bufferViews[accessor.bufferView];
  uint32_t sizeofIndexType = 0;
  switch (accessor.componentType) {
  case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
    sizeofIndexType = sizeof(uint32_t);
    indicesData.indexType = VK_INDEX_TYPE_UINT32;
    break;
  }
  case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
    sizeofIndexType = sizeof(uint16_t);
    indicesData.indexType = VK_INDEX_TYPE_UINT16;
    break;
  }
  default:
    mgAssertDesc(false, "not supported");
  }
  indicesData.indices.resize(accessor.count * sizeofIndexType);
  memcpy(indicesData.indices.data(),
         (const char *)(&model.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]),
         accessor.count * sizeofIndexType);
  return indicesData;
}

static VertexData getVertexData(const tinygltf::Model &model, const tinygltf::Accessor &accessor,
                                const tinygltf::BufferView &bufferView) {
  VertexData vertexData = {};
  vertexData.buffer.resize(bufferView.byteLength);
  memcpy(vertexData.buffer.data(),
         (const char *)(&model.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]),
         bufferView.byteLength);

  switch (accessor.type) {
  case TINYGLTF_TYPE_VEC2:
    vertexData.format = VK_FORMAT_R32G32_SFLOAT;
    vertexData.size = sizeof(float) * 2;
    break;
  case TINYGLTF_TYPE_VEC3:
    vertexData.format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexData.size = sizeof(float) * 3;
    break;
  case TINYGLTF_TYPE_VEC4:
    vertexData.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertexData.size = sizeof(float) * 4;
    break;
  default:
    mgAssertDesc(false, "not supported");
  }
  return vertexData;
}

static VertexDatas parseVertexDatas(const tinygltf::Model &model, const tinygltf::Primitive &tinyPrimitive) {
  VertexDatas vertexDatas = {};
  for (const auto &attribute : tinyPrimitive.attributes) {
    const auto attributeName = attribute.first;
    const auto accessorIndex = attribute.second;
    const auto &accessor = model.accessors[accessorIndex];
    const auto bufferView = model.bufferViews[accessor.bufferView];

    auto vertexData = getVertexData(model, accessor, bufferView);

    if (attributeName == "POSITION")
      vertexDatas.positions = std::move(vertexData);
    else if (attributeName == "NORMAL")
      vertexDatas.normals = std::move(vertexData);
    else if (attributeName == "TEXCOORD_0")
      vertexDatas.textCoords = std::move(vertexData);
    else if (attributeName == "TANGENT")
      vertexDatas.tangents = std::move(vertexData);
    else
      mgAssert(false);
  }
  return vertexDatas;
}

static void parseGltFTree(std::vector<InternalMesh> &internalMeshes, const tinygltf::Model &model,
                          const tinygltf::Node &node) {
  InternalMesh mesh = {};
  if (node.mesh != -1) {
    const auto modelMesh = model.meshes[node.mesh];
    int32_t binding = -1;
    for (const auto &modelPrimitive : modelMesh.primitives) {
      binding++;
      Primitive primitive = {};
      primitive.materialIndex = modelPrimitive.material;

      primitive.vertexDatas = parseVertexDatas(model, modelPrimitive);
      primitive.indicesData = parseIndices(model, modelPrimitive);

      mesh.primitives.push_back(primitive);
    }
    internalMeshes.push_back(mesh);
  }
  for (const auto &childNodeIndex : node.children) {
    parseGltFTree(internalMeshes, model, model.nodes[childNodeIndex]);
  }
}

static std::tuple<std::vector<float>, uint32_t> makeMeshInterleaved(const _GltfMesh &gltfMesh) {
  const auto &internalMesh = gltfMesh.internalMeshes.front();

  const auto &primitive = internalMesh.primitives.front();
  const auto &vertexData = primitive.vertexDatas;
  const uint8_t *ptr = primitive.indicesData.indices.data();
  const uint8_t *end = primitive.indicesData.indices.data() + primitive.indicesData.indices.size();
  const auto type = primitive.indicesData.indexType;
  const uint32_t step = type == VK_INDEX_TYPE_UINT32 ? sizeof(uint32_t) : sizeof(uint16_t);

  uint32_t totalSize = 0;
  for (; ptr < end; ptr += step) {
    totalSize +=
        vertexData.positions.size + vertexData.normals.size + vertexData.tangents.size + vertexData.textCoords.size;
  }

  std::vector<float> vertices(totalSize / sizeof(float));
  uint32_t count = 0;

  uint32_t offset = 0;
  ptr = primitive.indicesData.indices.data();
  for (; ptr < end; ptr += step) {
    count++;
    const auto index = type == VK_INDEX_TYPE_UINT32 ? *(uint32_t *)(ptr) : *(uint16_t *)(ptr);
    memcpy(vertices.data() + offset, vertexData.positions.buffer.data() + vertexData.positions.size * index,
           vertexData.positions.size);
    offset += vertexData.positions.size / sizeof(float);
    memcpy(vertices.data() + offset, vertexData.normals.buffer.data() + vertexData.normals.size * index,
           vertexData.normals.size);
    offset += vertexData.normals.size / sizeof(float);
    memcpy(vertices.data() + offset, vertexData.tangents.buffer.data() + vertexData.tangents.size * index,
           vertexData.tangents.size);
    offset += vertexData.tangents.size / sizeof(float);
    memcpy(vertices.data() + offset, vertexData.textCoords.buffer.data() + vertexData.textCoords.size * index,
           vertexData.textCoords.size);
    offset += vertexData.textCoords.size / sizeof(float);
  }
  return {vertices, count};
}

namespace mg {

GltfMeshes parseGltf(const std::string &id, const std::string &path, const std::string &name) {
  tinygltf::Model model = {};
  tinygltf::TinyGLTF loader = {};
  std::string err, warn;
  

  loader.LoadASCIIFromFile(&model, &err, &warn, path + name);
  if (!err.empty()) {
    printf("Err: %s\n", err.c_str());
    exit(1);
  }

  _GltfMesh mesh = {};
  mesh.id = id;

  auto materials = parseMaterials(model);
  auto textures = parseTextures(model);
  auto images = parseImages(model);
  for (auto &image : images) {
    image.path = path;
  }

  mesh.internalMeshes.reserve(model.nodes.size());

  const auto &defualtScene = model.scenes[model.defaultScene == -1 ? 0 : model.defaultScene];
  for (const auto &nodeIndex : defualtScene.nodes)
    parseGltFTree(mesh.internalMeshes, model, model.nodes[nodeIndex]);

  auto meshInterleaved = makeMeshInterleaved(mesh);

  GltfMeshes gltfMeshes = {};
  gltfMeshes.id = id;
  gltfMeshes.images = images;
  GltfMesh gltfMesh = {};

  auto [vertices, count] = std::move(meshInterleaved);
  gltfMesh.vertices = std::move(vertices);
  gltfMesh.count = count;

  for (const auto &internalMesh : mesh.internalMeshes) {
    for (const auto &primitive : internalMesh.primitives) {
      gltfMesh.materialIndex = primitive.materialIndex;
      gltfMesh.textureIndex = primitive.textureIndex;

      if (primitive.vertexDatas.positions.size)
        gltfMesh.attributes.push_back({"POSITION", primitive.vertexDatas.positions.format});
      if (primitive.vertexDatas.normals.size)
        gltfMesh.attributes.push_back({"NORMALS", primitive.vertexDatas.normals.format});
      if (primitive.vertexDatas.tangents.size)
        gltfMesh.attributes.push_back({"TANGENT", primitive.vertexDatas.tangents.format});
      if (primitive.vertexDatas.textCoords.size)
        gltfMesh.attributes.push_back({"TEXCOORD", primitive.vertexDatas.textCoords.format});
    }
  }
  gltfMeshes.meshes.push_back(std::move(gltfMesh));
  return gltfMeshes;
}

} // namespace mg
