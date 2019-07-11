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
struct UBO {
  glm::mat4 mvp;
  glm::vec4 color;
};
namespace shaderResource {
  static bool hasPushConstant = false;
  static Resources resources[2] = {
    Resources::UBO,
    Resources::COMBINED_IMAGE_SAMPLER,
  };
  union DescriptorSets {
    struct {
      VkDescriptorSet ubo;
      VkDescriptorSet imageSampler;
    };
    VkDescriptorSet values[2];
  };
  } // shaderResource
} //denoise

namespace depth {
struct UBO {
  glm::mat4 mvp;
  glm::vec2 nearAndFar;
};
namespace InputAssembler {
  static VertexInputState vertexInputState[1] = {
    { VK_FORMAT_R32G32B32A32_SFLOAT, 0, 0, 0, 16 },
  };
  struct VertexInputData {
    glm::vec4 positions;
  };
};
namespace shaderResource {
  static bool hasPushConstant = false;
  static Resources resources[2] = {
    Resources::UBO,
    Resources::COMBINED_IMAGE_SAMPLER,
  };
  union DescriptorSets {
    struct {
      VkDescriptorSet ubo;
      VkDescriptorSet samplerDepth;
    };
    VkDescriptorSet values[2];
  };
  } // shaderResource
} //depth

namespace final {
struct UBO {
  struct Light {
    glm::vec4 position;
    glm::vec4 color;
  };
  glm::mat4 projection;
  glm::mat4 view;
  Light lights[32];
};
namespace shaderResource {
  static bool hasPushConstant = false;
  static Resources resources[5] = {
    Resources::UBO,
    Resources::COMBINED_IMAGE_SAMPLER,
    Resources::COMBINED_IMAGE_SAMPLER,
    Resources::COMBINED_IMAGE_SAMPLER,
    Resources::COMBINED_IMAGE_SAMPLER,
  };
  union DescriptorSets {
    struct {
      VkDescriptorSet ubo;
      VkDescriptorSet samplerNormal;
      VkDescriptorSet samplerDiffuse;
      VkDescriptorSet samplerSSAOBlured;
      VkDescriptorSet samplerWorldViewPostion;
    };
    VkDescriptorSet values[5];
  };
  } // shaderResource
} //final

namespace fontRendering {
struct UBO {
  glm::mat4 projection;
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
namespace shaderResource {
  static bool hasPushConstant = false;
  static Resources resources[2] = {
    Resources::UBO,
    Resources::COMBINED_IMAGE_SAMPLER,
  };
  union DescriptorSets {
    struct {
      VkDescriptorSet ubo;
      VkDescriptorSet glyphTexture;
    };
    VkDescriptorSet values[2];
  };
  } // shaderResource
} //fontRendering

namespace frontAndBack {
struct UBO {
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
namespace shaderResource {
  static bool hasPushConstant = false;
  static Resources resources[1] = {
    Resources::UBO,
  };
  union DescriptorSets {
    struct {
      VkDescriptorSet ubo;
    };
    VkDescriptorSet values[1];
  };
  } // shaderResource
} //frontAndBack

namespace gltf {
struct UBO {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
  glm::vec4 cameraPosition;
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
namespace shaderResource {
  static bool hasPushConstant = false;
  static Resources resources[5] = {
    Resources::UBO,
    Resources::COMBINED_IMAGE_SAMPLER,
    Resources::COMBINED_IMAGE_SAMPLER,
    Resources::COMBINED_IMAGE_SAMPLER,
    Resources::COMBINED_IMAGE_SAMPLER,
  };
  union DescriptorSets {
    struct {
      VkDescriptorSet ubo;
      VkDescriptorSet baseColorTexture;
      VkDescriptorSet normalTexture;
      VkDescriptorSet roughnessMetallicTexture;
      VkDescriptorSet emissiveTexture;
    };
    VkDescriptorSet values[5];
  };
  } // shaderResource
} //gltf

namespace imgui {
struct UBO {
  glm::vec2 uScale;
  glm::vec2 uTranslate;
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
namespace shaderResource {
  static bool hasPushConstant = false;
  static Resources resources[2] = {
    Resources::UBO,
    Resources::COMBINED_IMAGE_SAMPLER,
  };
  union DescriptorSets {
    struct {
      VkDescriptorSet ubo;
      VkDescriptorSet sTexture;
    };
    VkDescriptorSet values[2];
  };
  } // shaderResource
} //imgui

namespace mrt {
struct UBO {
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 model;
  glm::mat4 mNormal;
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
namespace shaderResource {
  static bool hasPushConstant = true;
  static Resources resources[1] = {
    Resources::UBO,
  };
  union DescriptorSets {
    struct {
      VkDescriptorSet ubo;
    };
    VkDescriptorSet values[1];
  };
  } // shaderResource
} //mrt

namespace nbody {
struct Storage {
  struct ImageData {
    glm::vec4 value;
  };
  ImageData* imageData = nullptr;
};
namespace shaderResource {
  static bool hasPushConstant = false;
  static Resources resources[1] = {
    Resources::SSBO,
  };
  union DescriptorSets {
    struct {
      VkDescriptorSet storage;
    };
    VkDescriptorSet values[1];
  };
  } // shaderResource
} //nbody

namespace solid {
struct UBO {
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
namespace shaderResource {
  static bool hasPushConstant = false;
  static Resources resources[2] = {
    Resources::UBO,
    Resources::SSBO,
  };
  union DescriptorSets {
    struct {
      VkDescriptorSet ubo;
      VkDescriptorSet storage;
    };
    VkDescriptorSet values[2];
  };
  } // shaderResource
} //solid

namespace ssao {
struct UBO {
  glm::mat4 projection;
  glm::vec4 kernel[64];
  glm::vec2 noiseScale;
};
namespace shaderResource {
  static bool hasPushConstant = false;
  static Resources resources[4] = {
    Resources::UBO,
    Resources::COMBINED_IMAGE_SAMPLER,
    Resources::COMBINED_IMAGE_SAMPLER,
    Resources::COMBINED_IMAGE_SAMPLER,
  };
  union DescriptorSets {
    struct {
      VkDescriptorSet ubo;
      VkDescriptorSet samplerNormal;
      VkDescriptorSet samplerNoise;
      VkDescriptorSet samplerWordViewPosition;
    };
    VkDescriptorSet values[4];
  };
  } // shaderResource
} //ssao

namespace ssaoBlur {
struct UBO {
  glm::vec2 size;
};
namespace shaderResource {
  static bool hasPushConstant = false;
  static Resources resources[2] = {
    Resources::UBO,
    Resources::COMBINED_IMAGE_SAMPLER,
  };
  union DescriptorSets {
    struct {
      VkDescriptorSet ubo;
      VkDescriptorSet samplerSSAO;
    };
    VkDescriptorSet values[2];
  };
  } // shaderResource
} //ssaoBlur

namespace textureRendering {
struct UBO {
  glm::mat4 mvp;
};
namespace InputAssembler {
  static VertexInputState vertexInputState[1] = {
    { VK_FORMAT_R32G32B32A32_SFLOAT, 0, 0, 0, 16 },
  };
  struct VertexInputData {
    glm::vec4 positions;
  };
};
namespace shaderResource {
  static bool hasPushConstant = false;
  static Resources resources[2] = {
    Resources::UBO,
    Resources::COMBINED_IMAGE_SAMPLER,
  };
  union DescriptorSets {
    struct {
      VkDescriptorSet ubo;
      VkDescriptorSet samplerImage;
    };
    VkDescriptorSet values[2];
  };
  } // shaderResource
} //textureRendering

namespace volume {
struct UBO {
  glm::mat4 boxToWorld;
  glm::mat4 worldToBox;
  glm::mat4 mv;
  glm::vec4 cameraPosition;
  glm::vec4 color;
  glm::vec4 minMaxIsoValue;
  glm::vec4 nrOfVoxels;
};
namespace shaderResource {
  static bool hasPushConstant = false;
  static Resources resources[4] = {
    Resources::UBO,
    Resources::COMBINED_IMAGE_SAMPLER,
    Resources::COMBINED_IMAGE_SAMPLER,
    Resources::COMBINED_IMAGE_SAMPLER,
  };
  union DescriptorSets {
    struct {
      VkDescriptorSet ubo;
      VkDescriptorSet samplerFront;
      VkDescriptorSet samplerBack;
      VkDescriptorSet samplerVolume;
    };
    VkDescriptorSet values[4];
  };
  } // shaderResource
} //volume

} // shaders
} // mg
