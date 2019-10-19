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
constexpr struct {
  const char *denoise_frag = "denoise.frag.spv";
  const char *denoise_vert = "denoise.vert.spv";
} files = {};
constexpr const char *shader = "denoise";
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
constexpr struct {
  const char *depth_frag = "depth.frag.spv";
  const char *depth_vert = "depth.vert.spv";
} files = {};
constexpr const char *shader = "depth";
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
constexpr struct {
  const char *final_frag = "final.frag.spv";
  const char *final_vert = "final.vert.spv";
} files = {};
constexpr const char *shader = "final";
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
constexpr struct {
  const char *fontRendering_frag = "fontRendering.frag.spv";
  const char *fontRendering_vert = "fontRendering.vert.spv";
} files = {};
constexpr const char *shader = "fontRendering";
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
constexpr struct {
  const char *frontAndBack_frag = "frontAndBack.frag.spv";
  const char *frontAndBack_vert = "frontAndBack.vert.spv";
} files = {};
constexpr const char *shader = "frontAndBack";
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
constexpr struct {
  const char *gltf_frag = "gltf.frag.spv";
  const char *gltf_vert = "gltf.vert.spv";
} files = {};
constexpr const char *shader = "gltf";
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
constexpr struct {
  const char *imageStorage_frag = "imageStorage.frag.spv";
  const char *imageStorage_vert = "imageStorage.vert.spv";
} files = {};
constexpr const char *shader = "imageStorage";
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
constexpr struct {
  const char *imgui_frag = "imgui.frag.spv";
  const char *imgui_vert = "imgui.vert.spv";
} files = {};
constexpr const char *shader = "imgui";
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
constexpr struct {
  const char *mrt_frag = "mrt.frag.spv";
  const char *mrt_vert = "mrt.vert.spv";
} files = {};
constexpr const char *shader = "mrt";
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
constexpr struct {
  const char *particle_frag = "particle.frag.spv";
  const char *particle_vert = "particle.vert.spv";
} files = {};
constexpr const char *shader = "particle";
} //particle

namespace procedural {
struct Ubo {
  glm::mat4 viewInverse;
  glm::mat4 projInverse;
  glm::vec4 cameraPosition;
  glm::vec4 cameraLookat;
  glm::vec4 attrib;
  glm::vec4 sobolId;
};
struct Storage {
  struct StorageData {
    glm::vec4 positions[500];
    glm::vec4 albedos[500];
  };
  StorageData storageData;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet image;
    VkDescriptorSet topLevelAS;
    VkDescriptorSet textures;
    VkDescriptorSet accumulationImage;
  };
  VkDescriptorSet values[5];
};
constexpr struct {
  const char *procedural_dielectrics_proc_rchit = "procedural.dielectrics.proc.rchit.spv";
  const char *procedural_lambert_proc_rchit = "procedural.lambert.proc.rchit.spv";
  const char *procedural_metal_proc_rchit = "procedural.metal.proc.rchit.spv";
  const char *procedural_proc_rint = "procedural.proc.rint.spv";
  const char *procedural_rgen = "procedural.rgen.spv";
  const char *procedural_rmiss = "procedural.rmiss.spv";
} files = {};
constexpr const char *shader = "procedural";
} //procedural

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
constexpr struct {
  const char *simulate_positions_comp = "simulate_positions.comp.spv";
} files = {};
constexpr const char *shader = "simulate_positions";
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
constexpr struct {
  const char *simulate_velocities_comp = "simulate_velocities.comp.spv";
} files = {};
constexpr const char *shader = "simulate_velocities";
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
constexpr struct {
  const char *solid_frag = "solid.frag.spv";
  const char *solid_vert = "solid.vert.spv";
} files = {};
constexpr const char *shader = "solid";
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
constexpr struct {
  const char *ssao_frag = "ssao.frag.spv";
  const char *ssao_vert = "ssao.vert.spv";
} files = {};
constexpr const char *shader = "ssao";
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
constexpr struct {
  const char *ssaoBlur_frag = "ssaoBlur.frag.spv";
  const char *ssaoBlur_vert = "ssaoBlur.vert.spv";
} files = {};
constexpr const char *shader = "ssaoBlur";
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
constexpr struct {
  const char *textureRendering_frag = "textureRendering.frag.spv";
  const char *textureRendering_vert = "textureRendering.vert.spv";
} files = {};
constexpr const char *shader = "textureRendering";
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
constexpr struct {
  const char *toneMapping_frag = "toneMapping.frag.spv";
  const char *toneMapping_vert = "toneMapping.vert.spv";
} files = {};
constexpr const char *shader = "toneMapping";
} //toneMapping

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
constexpr struct {
  const char *volume_frag = "volume.frag.spv";
  const char *volume_vert = "volume.vert.spv";
} files = {};
constexpr const char *shader = "volume";
} //volume

} // shaders
} // mg
