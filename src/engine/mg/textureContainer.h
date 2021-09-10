#pragma once
#include "mg/mgUtils.h"
#include "vulkan/deviceAllocator.h"
#include "vulkan/vkContext.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace mg {

struct TextureId {
  uint32_t index;
  uint32_t generation;
};

enum class TEXTURE_TYPE { TEXTURE_1D, TEXTURE_2D, TEXTURE_3D, ATTACHMENT, IMAGE_STORAGE, DEPTH };

struct _TextureData {
  VkImage image;
  VkImageView imageView;
  mg::DeviceHeapAllocation heapAllocation;
  VkFormat format;
  TEXTURE_TYPE type;
};

struct CreateTextureInfo {
  std::string id;
  TEXTURE_TYPE type;
  VkFormat format;
  VkExtent3D size;
  uint32_t sizeInBytes;
  void *data;
};

struct Texture {
  VkImageView imageView;
  VkFormat format;
  std::string id;
};

class TextureContainer : mg::nonCopyable {
public:
  void createTextureContainer();
  TextureId createTexture(const CreateTextureInfo &textureInfo);
  uint32_t getTexture2DDescriptorIndex(TextureId textureId);
  uint32_t getTexture3DDescriptorIndex(TextureId textureId);


  Texture getTexture(TextureId textureId);
  void removeTexture(TextureId textureId);

  VkDescriptorSet getDescriptorSet(); 
  VkDescriptorSet getDescriptorSet3D(); 

  void setupDescriptorSets();

  void destroyTextureContainer();

  ~TextureContainer();

private:
  VkDescriptorSet _descriptorSet;
  VkDescriptorSet _descriptorSet3D;
  
  std::vector<_TextureData> _idToTexture;
  std::vector<uint32_t> _freeIndices;
  std::vector<uint32_t> _generations;
  std::vector<bool> _isAlive;
  std::vector<uint32_t> _idToDescriptorIndex2D;
  std::vector<uint32_t> _idToDescriptorIndex3D;


};

} // namespace mg
