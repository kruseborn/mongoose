#pragma once
#include "mg/mgUtils.h"
#include "vulkan/deviceAllocator.h"
#include "vulkan/vkContext.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace mg {

enum class TEXTURE_TYPE { TEXTURE_1D, TEXTURE_2D, TEXTURE_3D, ATTACHMENT, DEPTH };
enum class TEXTURE_SAMPLER {
  POINT_CLAMP_TO_BORDER,
  LINEAR_CLAMP_TO_BORDER,
  LINEAR__ANISOTROPIC,
  LINEAR_CLAMP_TO_EDGE,
  LINEAR_REPEAT,
  NONE
};

struct _TextureData {
  enum { MAX_DESCRIPTOR_SET = 5 };

  uint32_t nrOfDescriptorSets;
  VkDescriptorSet descriptorSets[MAX_DESCRIPTOR_SET];
  mg::TEXTURE_SAMPLER textureSamplers[MAX_DESCRIPTOR_SET];
  VkImage image;
  VkImageView imageView;
  mg::DeviceHeapAllocation heapAllocation;
  VkFormat format;
};

struct CreateTextureInfo {
  std::string id;
  TEXTURE_TYPE type;
  VkFormat format;
  VkExtent3D size;
  uint32_t sizeInBytes;
  void *data;

  std::vector<TEXTURE_SAMPLER> textureSamplers;
};

struct Texture {
  VkDescriptorSet descriptorSet;
  VkImageView imageView;
  VkFormat format;
  std::string id;
};

class TextureContainer : mg::nonCopyable {
public:
  void createTextureContainer() {};
  void createTexture(const CreateTextureInfo &textureInfo) ;
  Texture getTexture(const std::string &id, TEXTURE_SAMPLER sampler = TEXTURE_SAMPLER::NONE);
  void removeTexture(const std::string &id) ;
  void destroyTextureContainer();

  ~TextureContainer();

private:
  void destroyTexture(const std::string &id);

  std::unordered_map<std::string, mg::_TextureData> _idToTexture;
};

} // namespace mg
