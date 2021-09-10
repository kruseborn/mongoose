#pragma once

// This file is auto generated, don�t touch

#include <glm/glm.hpp>
#include "vkContext.h"

namespace mg {
namespace shaders {

enum class Resources { UBO, COMBINED_IMAGE_SAMPLER, SSBO };
constexpr uint32_t VERTEX_BINDING_ID = 0;
constexpr uint32_t INSTANCE_BINDING_ID = 1;
struct VertexInputState {
  VkFormat format;
  uint32_t location, offset, binding, size;
};

namespace addSource {
struct Ubo {
  glm::vec2 delta;
  int32_t i;
  int32_t j;
  uint32_t N;
};
struct D {
  float* x = nullptr;
};
struct U {
  float* x = nullptr;
};
struct V {
  float* x = nullptr;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet u;
    VkDescriptorSet v;
    VkDescriptorSet d;
  };
  VkDescriptorSet values[4];
};
constexpr struct {
  const char *addSource_comp = "addSource.comp.spv";
} files = {};
constexpr const char *shader = "addSource";
} //addSource

namespace advec {
struct Ubo {
  uint32_t N;
  uint32_t b;
  float dt;
};
struct D {
  float* x = nullptr;
};
struct D0 {
  float* x = nullptr;
};
struct U {
  float* x = nullptr;
};
struct V {
  float* x = nullptr;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet u;
    VkDescriptorSet v;
    VkDescriptorSet d;
    VkDescriptorSet d0;
  };
  VkDescriptorSet values[5];
};
constexpr struct {
  const char *advec_comp = "advec.comp.spv";
} files = {};
constexpr const char *shader = "advec";
} //advec

namespace cubes {
struct Ubo {
  glm::mat4 mvp;
  glm::vec4 color;
};
namespace InputAssembler {
  static VertexInputState vertexInputState[2] = {
    { VK_FORMAT_R32G32B32_SFLOAT, 0, 0, 0, 12 },
    { VK_FORMAT_R32G32B32_SFLOAT, 1, 0, 1, 12 },
  };
  struct VertexInputData {
    glm::vec3 in_position;
  };
  struct InstanceInputData {
    glm::vec3 instancing_translate;
  };
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
  };
  VkDescriptorSet values[1];
};
constexpr struct {
  const char *cubes_frag = "cubes.frag.spv";
  const char *cubes_vert = "cubes.vert.spv";
} files = {};
constexpr const char *shader = "cubes";
} //cubes

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

namespace density {
struct Ubo {
  glm::vec4 corner;
  glm::uvec4 N;
  float cellSize;
};
struct Density {
  float* x = nullptr;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet density;
  };
  VkDescriptorSet values[2];
};
constexpr struct {
  const char *density_comp = "density.comp.spv";
} files = {};
constexpr const char *shader = "density";
} //density

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
  struct InstanceInputData {
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

namespace diffuse {
struct Ubo {
  uint32_t N;
  uint32_t b;
  float dt;
  float diff;
};
struct X {
  float* x = nullptr;
};
struct X0 {
  float* x = nullptr;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet x;
    VkDescriptorSet x0;
  };
  VkDescriptorSet values[3];
};
constexpr struct {
  const char *diffuse_comp = "diffuse.comp.spv";
} files = {};
constexpr const char *shader = "diffuse";
} //diffuse

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

namespace fluid {
struct Ubo {
  glm::uvec4 screenSize;
};
struct D {
  float* x = nullptr;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet d;
  };
  VkDescriptorSet values[2];
};
constexpr struct {
  const char *fluid_frag = "fluid.frag.spv";
  const char *fluid_vert = "fluid.vert.spv";
} files = {};
constexpr const char *shader = "fluid";
} //fluid

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
  struct InstanceInputData {
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
  struct InstanceInputData {
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
  struct InstanceInputData {
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
  struct InstanceInputData {
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

namespace marching_cubes {
struct Ubo {
  glm::vec4 corner;
  glm::vec4 attributes;
  glm::uvec4 N;
  float cellSize;
};
struct A2iTriangleConnectionTable {
  int32_t x[16];
};
struct AiCubeEdgeFlags {
  int32_t x[256];
};
struct Density {
  float* x = nullptr;
};
struct DrawIndirectCommand {
  uint32_t vertexCount;
  uint32_t instanceCount;
  uint32_t firstVertex;
  uint32_t firstInstance;
};
struct Mesh {
  struct Triangle {
    glm::vec4 v[6];
  };
  Triangle triangles[100000];
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet density;
    VkDescriptorSet a2iTriangleConnectionTable;
    VkDescriptorSet aiCubeEdgeFlags;
    VkDescriptorSet mesh;
  };
  VkDescriptorSet values[5];
};
constexpr struct {
  const char *marching_cubes_comp = "marching_cubes.comp.spv";
} files = {};
constexpr const char *shader = "marching_cubes";
} //marching_cubes

namespace meshNormals {
struct Ubo {
  glm::mat4 mvp;
  glm::vec4 color;
};
namespace InputAssembler {
  static VertexInputState vertexInputState[2] = {
    { VK_FORMAT_R32G32B32_SFLOAT, 0, 0, 0, 12 },
    { VK_FORMAT_R32G32B32_SFLOAT, 1, 12, 0, 12 },
  };
  struct VertexInputData {
    glm::vec3 in_position;
    glm::vec3 in_normal;
  };
  struct InstanceInputData {
  };
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
  };
  VkDescriptorSet values[1];
};
constexpr struct {
  const char *meshNormals_frag = "meshNormals.frag.spv";
  const char *meshNormals_vert = "meshNormals.vert.spv";
} files = {};
constexpr const char *shader = "meshNormals";
} //meshNormals

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
  struct InstanceInputData {
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

namespace octreeAlloc {
struct Ubo {
  glm::uvec4 attrib;
};
struct uuBuildInfo {
  glm::uvec4 info1;
};
struct uuOctree {
  uint32_t* values = nullptr;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet uOctree;
    VkDescriptorSet uBuildInfo;
  };
  VkDescriptorSet values[3];
};
constexpr struct {
  const char *octreeAlloc_comp = "octreeAlloc.comp.spv";
} files = {};
constexpr const char *shader = "octreeAlloc";
} //octreeAlloc

namespace octreeModify {
struct Ubo {
  glm::uvec4 attrib;
};
struct DispatchIndirectCommand {
  uint32_t x;
  uint32_t y;
  uint32_t z;
};
struct uuBuildInfo {
  glm::uvec4 info1;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet uBuildInfo;
  };
  VkDescriptorSet values[2];
};
constexpr struct {
  const char *octreeModify_comp = "octreeModify.comp.spv";
} files = {};
constexpr const char *shader = "octreeModify";
} //octreeModify

namespace octreeTag {
struct Ubo {
  glm::uvec4 attrib;
};
struct uuFragmentList {
  glm::uvec2* values = nullptr;
};
struct uuOctree {
  uint32_t* values = nullptr;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet uFragmentList;
    VkDescriptorSet uOctree;
  };
  VkDescriptorSet values[3];
};
constexpr struct {
  const char *octreeTag_comp = "octreeTag.comp.spv";
} files = {};
constexpr const char *shader = "octreeTag";
} //octreeTag

namespace octreeTracer {
struct Ubo {
  glm::mat4 uProjection;
  glm::mat4 uView;
  glm::vec4 uPosition;
  glm::ivec4 screenSize;
};
struct uuOctree {
  uint32_t* values = nullptr;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet textures;
    VkDescriptorSet volumeTexture;
    VkDescriptorSet uOctree;
  };
  VkDescriptorSet values[4];
};
constexpr struct {
  const char *octreeTracer_frag = "octreeTracer.frag.spv";
  const char *octreeTracer_vert = "octreeTracer.vert.spv";
} files = {};
constexpr const char *shader = "octreeTracer";
} //octreeTracer

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
  struct InstanceInputData {
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

namespace postProject {
struct Ubo {
  uint32_t N;
};
struct Div {
  float* x = nullptr;
};
struct P {
  float* x = nullptr;
};
struct U {
  float* x = nullptr;
};
struct V {
  float* x = nullptr;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet u;
    VkDescriptorSet v;
    VkDescriptorSet p;
    VkDescriptorSet div;
  };
  VkDescriptorSet values[5];
};
constexpr struct {
  const char *postProject_comp = "postProject.comp.spv";
} files = {};
constexpr const char *shader = "postProject";
} //postProject

namespace preProject {
struct Ubo {
  uint32_t N;
};
struct Div {
  float* x = nullptr;
};
struct P {
  float* x = nullptr;
};
struct U {
  float* x = nullptr;
};
struct V {
  float* x = nullptr;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet u;
    VkDescriptorSet v;
    VkDescriptorSet p;
    VkDescriptorSet div;
  };
  VkDescriptorSet values[5];
};
constexpr struct {
  const char *preProject_comp = "preProject.comp.spv";
} files = {};
constexpr const char *shader = "preProject";
} //preProject

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

namespace project {
struct Ubo {
  uint32_t N;
};
struct Div {
  float* x = nullptr;
};
struct P {
  float* x = nullptr;
};
struct U {
  float* x = nullptr;
};
struct V {
  float* x = nullptr;
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
    VkDescriptorSet u;
    VkDescriptorSet v;
    VkDescriptorSet p;
    VkDescriptorSet div;
  };
  VkDescriptorSet values[5];
};
constexpr struct {
  const char *project_comp = "project.comp.spv";
} files = {};
constexpr const char *shader = "project";
} //project

namespace sdf {
struct Ubo {
  glm::mat4 worldToBox;
  glm::mat4 worldToBox2;
  glm::vec4 info;
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
  const char *sdf_frag = "sdf.frag.spv";
  const char *sdf_vert = "sdf.vert.spv";
} files = {};
constexpr const char *shader = "sdf";
} //sdf

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
  struct InstanceInputData {
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

namespace solidColor {
struct Ubo {
  glm::mat4 mvp;
  glm::vec4 color;
};
namespace InputAssembler {
  static VertexInputState vertexInputState[1] = {
    { VK_FORMAT_R32G32B32_SFLOAT, 0, 0, 0, 12 },
  };
  struct VertexInputData {
    glm::vec3 in_position;
  };
  struct InstanceInputData {
  };
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
  };
  VkDescriptorSet values[1];
};
constexpr struct {
  const char *solidColor_frag = "solidColor.frag.spv";
  const char *solidColor_vert = "solidColor.vert.spv";
} files = {};
constexpr const char *shader = "solidColor";
} //solidColor

namespace solidColorAndNormal {
struct Ubo {
  glm::mat4 mvp;
  glm::vec4 color;
};
namespace InputAssembler {
  static VertexInputState vertexInputState[2] = {
    { VK_FORMAT_R32G32B32A32_SFLOAT, 0, 0, 0, 16 },
    { VK_FORMAT_R32G32B32A32_SFLOAT, 1, 16, 0, 16 },
  };
  struct VertexInputData {
    glm::vec4 in_position;
    glm::vec4 in_normal;
  };
  struct InstanceInputData {
  };
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
  };
  VkDescriptorSet values[1];
};
constexpr struct {
  const char *solidColorAndNormal_frag = "solidColorAndNormal.frag.spv";
  const char *solidColorAndNormal_vert = "solidColorAndNormal.vert.spv";
} files = {};
constexpr const char *shader = "solidColorAndNormal";
} //solidColorAndNormal

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

namespace terrain {
struct Ubo {
  glm::mat4 mvp;
  glm::vec4 color;
};
namespace InputAssembler {
  static VertexInputState vertexInputState[2] = {
    { VK_FORMAT_R32G32B32A32_SFLOAT, 0, 0, 0, 16 },
    { VK_FORMAT_R32G32B32A32_SFLOAT, 1, 16, 0, 16 },
  };
  struct VertexInputData {
    glm::vec4 in_position;
    glm::vec4 in_normal;
  };
  struct InstanceInputData {
  };
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
  };
  VkDescriptorSet values[1];
};
constexpr struct {
  const char *terrain_frag = "terrain.frag.spv";
  const char *terrain_vert = "terrain.vert.spv";
} files = {};
constexpr const char *shader = "terrain";
} //terrain

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
  struct InstanceInputData {
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

namespace voxelizer {
struct Ubo {
  uint32_t resolution;
};
struct Voxels {
  glm::uvec2 values[1000000];
};
namespace InputAssembler {
  static VertexInputState vertexInputState[3] = {
    { VK_FORMAT_R32G32B32_SFLOAT, 0, 0, 0, 12 },
    { VK_FORMAT_R32G32B32_SFLOAT, 1, 12, 0, 12 },
    { VK_FORMAT_R32G32_SFLOAT, 2, 24, 0, 8 },
  };
  struct VertexInputData {
    glm::vec3 in_position;
    glm::vec3 in_normal;
    glm::vec2 in_tex;
  };
  struct InstanceInputData {
  };
};
union DescriptorSets {
  struct {
    VkDescriptorSet ubo;
  };
  VkDescriptorSet values[1];
};
constexpr struct {
  const char *voxelizer_frag = "voxelizer.frag.spv";
  const char *voxelizer_geom = "voxelizer.geom.spv";
  const char *voxelizer_vert = "voxelizer.vert.spv";
} files = {};
constexpr const char *shader = "voxelizer";
} //voxelizer

} // shaders
} // mg
