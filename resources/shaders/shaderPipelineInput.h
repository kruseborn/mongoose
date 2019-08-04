#pragma once

// This file is auto generated, don�t touch

#include <glm/glm.hpp>
#include "vkContext.h"

namespace mg {
namespace shaders {

enum class Resources { UBO, COMBINED_IMAGE_SAMPLER, SSBO };
struct VertexInputState {
  VkFormat format;
  uint32_t location, offset, binding, size;
};

namespace denoise {
struct Ubo {
  glm::mat4 mvp;
  glm::vec4 color;
};
struct TextureIndices {
  int32_t textureIndex;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet textures;
  };
  VkDescriptorSet values[2];
};
} //denoise

namespace depth {
struct Ubo {
  glm::mat4 mvp;
  glm::vec2 nearAndFar;
};
struct TextureIndices {
  int32_t textureIndex;
};
namespace InputAssembler {
  static VertexInputState vertexInputState[1] = {
    { VK_FORMAT_R32G32B32A32_SFLOAT, 0, 0, 0, 16 },
  };
  struct VertexInputData {
    glm::vec4 positions;
  };
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet textures;
  };
  VkDescriptorSet values[2];
};
} //depth

namespace final {
struct Ubo {
  struct Light {
    glm::vec4 position;
    glm::vec4 color;
  };
  glm::mat4 projection;
  glm::mat4 view;
  Light lights[32];
};
struct TextureIndices {
  int32_t normalIndex;
  int32_t diffuseIndex;
  int32_t ssaoBluredIndex;
  int32_t worldViewPostionIndex;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet textures;
  };
  VkDescriptorSet values[2];
};
} //final

namespace fontRendering {
struct Ubo {
  glm::mat4 projection;
};
struct TextureIndices {
  int32_t textureIndex;
};
namespace InputAssembler {
  static VertexInputState vertexInputState[2] = {
    { VK_FORMAT_R32G32B32A32_SFLOAT, 0, 0, 0, 16 },
    { VK_FORMAT_R32G32B32A32_SFLOAT, 1, 16, 0, 16 },
  };
  struct VertexInputData {
    glm::vec4 inPosition;
    glm::vec4 inColor;
  };
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet textures;
  };
  VkDescriptorSet values[2];
};
} //fontRendering

namespace frontAndBack {
struct Ubo {
  glm::mat4 mvp;
  glm::mat4 worldToBox;
};
namespace InputAssembler {
  static VertexInputState vertexInputState[1] = {
    { VK_FORMAT_R32G32B32_SFLOAT, 0, 0, 0, 12 },
  };
  struct VertexInputData {
    glm::vec3 positions;
  };
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
  };
  VkDescriptorSet values[1];
};
} //frontAndBack

namespace gltf {
struct Ubo {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
  glm::vec4 cameraPosition;
};
struct TextureIndices {
  int32_t baseColorIndex;
  int32_t normalIndex;
  int32_t roughnessMetallicIndex;
  int32_t emissiveIndex;
};
namespace InputAssembler {
  static VertexInputState vertexInputState[4] = {
    { VK_FORMAT_R32G32B32_SFLOAT, 0, 0, 0, 12 },
    { VK_FORMAT_R32G32B32_SFLOAT, 1, 12, 0, 12 },
    { VK_FORMAT_R32G32B32A32_SFLOAT, 2, 24, 0, 16 },
    { VK_FORMAT_R32G32_SFLOAT, 3, 40, 0, 8 },
  };
  struct VertexInputData {
    glm::vec3 in_position;
    glm::vec3 in_normal;
    glm::vec4 in_tangent;
    glm::vec2 in_texCoord;
  };
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet textures;
  };
  VkDescriptorSet values[2];
};
} //gltf

namespace imageStorage {
struct Ubo {
  glm::vec4 temp;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet image;
  };
  VkDescriptorSet values[2];
};
} //imageStorage

namespace imgui {
struct Ubo {
  glm::vec2 uScale;
  glm::vec2 uTranslate;
};
struct TextureIndices {
  int32_t textureIndex;
};
namespace InputAssembler {
  static VertexInputState vertexInputState[3] = {
    { VK_FORMAT_R32G32_SFLOAT, 0, 0, 0, 8 },
    { VK_FORMAT_R32G32_SFLOAT, 1, 8, 0, 8 },
    { VK_FORMAT_R32G32B32A32_SFLOAT, 2, 16, 0, 16 },
  };
  struct VertexInputData {
    glm::vec2 inPos;
    glm::vec2 inUV;
    glm::vec4 inColor;
  };
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet textures;
  };
  VkDescriptorSet values[2];
};
} //imgui

namespace mrt {
struct Ubo {
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 model;
  glm::mat4 mNormal;
};
struct Material {
  glm::vec4 diffuse;
};
namespace InputAssembler {
  static VertexInputState vertexInputState[3] = {
    { VK_FORMAT_R32G32B32_SFLOAT, 0, 0, 0, 12 },
    { VK_FORMAT_R32G32B32_SFLOAT, 1, 12, 0, 12 },
    { VK_FORMAT_R32G32_SFLOAT, 2, 24, 0, 8 },
  };
  struct VertexInputData {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
  };
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
  };
  VkDescriptorSet values[1];
};
} //mrt

namespace particle {
struct Ubo {
  glm::mat4 projection;
  glm::mat4 modelview;
  glm::vec2 screendim;
};
struct TextureIndices {
  int32_t textureIndex;
};
namespace InputAssembler {
  static VertexInputState vertexInputState[2] = {
    { VK_FORMAT_R32G32B32A32_SFLOAT, 0, 0, 0, 16 },
    { VK_FORMAT_R32G32B32A32_SFLOAT, 1, 16, 0, 16 },
  };
  struct VertexInputData {
    glm::vec4 in_position;
    glm::vec4 in_velocity;
  };
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet textures;
  };
  VkDescriptorSet values[2];
};
} //particle

namespace simulate_positions {
struct Ubo {
  float dt;
};
struct Storage {
  struct Particle {
    glm::vec4 position;
    glm::vec4 velocity;
  };
  Particle* particles = nullptr;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet storage;
  };
  VkDescriptorSet values[2];
};
} //simulate_positions

namespace simulate_velocities {
struct Ubo {
  float dt;
};
struct Storage {
  struct Particle {
    glm::vec4 position;
    glm::vec4 velocity;
  };
  Particle* particles = nullptr;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet storage;
  };
  VkDescriptorSet values[2];
};
} //simulate_velocities

namespace solid {
struct Ubo {
  glm::mat4 mvp;
};
struct Storage {
  struct StorageData {
    glm::vec4 color;
    glm::vec4 position;
  };
  StorageData* storageData = nullptr;
};
namespace InputAssembler {
  static VertexInputState vertexInputState[1] = {
    { VK_FORMAT_R32G32B32_SFLOAT, 0, 0, 0, 12 },
  };
  struct VertexInputData {
    glm::vec3 in_position;
  };
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
  };
  VkDescriptorSet values[1];
};
} //solid

namespace ssao {
struct Ubo {
  glm::mat4 projection;
  glm::vec4 kernel[64];
  glm::vec2 noiseScale;
};
struct TextureIndices {
  int32_t normalIndex;
  int32_t wordViewPositionIndex;
  int32_t noiseIndex;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet textures;
  };
  VkDescriptorSet values[2];
};
} //ssao

namespace ssaoBlur {
struct Ubo {
  glm::vec2 size;
};
struct TextureIndices {
  int32_t ssaoIndex;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet textures;
  };
  VkDescriptorSet values[2];
};
} //ssaoBlur

namespace textureRendering {
struct Ubo {
  glm::mat4 mvp;
};
struct TextureIndices {
  int32_t textureIndex;
};
namespace InputAssembler {
  static VertexInputState vertexInputState[1] = {
    { VK_FORMAT_R32G32B32A32_SFLOAT, 0, 0, 0, 16 },
  };
  struct VertexInputData {
    glm::vec4 positions;
  };
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet textures;
  };
  VkDescriptorSet values[2];
};
} //textureRendering

namespace toneMapping {
struct Ubo {
  glm::vec4 color;
};
struct TextureIndices {
  int32_t textureIndex;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet textures;
  };
  VkDescriptorSet values[2];
};
} //toneMapping

namespace tracing {
struct Ubo {
  glm::mat4 viewInverse;
  glm::mat4 projInverse;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet image;
    VkDescriptorSet topLevelAS;
  };
  VkDescriptorSet values[3];
};
} //tracing

namespace volume {
struct Ubo {
  glm::mat4 boxToWorld;
  glm::mat4 worldToBox;
  glm::mat4 mv;
  glm::vec4 cameraPosition;
  glm::vec4 color;
  glm::vec4 minMaxIsoValue;
};
struct TextureIndices {
  int32_t frontIndex;
  int32_t backIndex;
  int32_t volumeIndex;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet textures;
    VkDescriptorSet volumeTexture;
  };
  VkDescriptorSet values[3];
};
} //volume

} // shaders
} // mg
