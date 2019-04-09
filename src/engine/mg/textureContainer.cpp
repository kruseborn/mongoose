#include "textureContainer.h"
#include "mg/mgSystem.h"
#include "vulkan/deviceAllocator.h"
#include "vulkan/linearHeapAllocator.h"
#include "vulkan/vkUtils.h"
#include <unordered_map>

namespace mg {

struct ImageInfo {
  VkImageType vkImageType;
  VkImageUsageFlags vkImageUsageFlags;
  VkImageLayout vkImageLayout;
  VkImageViewType vkImageViewType;
};

static ImageInfo createImageInfoFromType(mg::TEXTURE_TYPE type) {
  ImageInfo imageInfo = {};

  switch (type) {
  case mg::TEXTURE_TYPE::TEXTURE_1D:
    imageInfo.vkImageType = VK_IMAGE_TYPE_1D;
    imageInfo.vkImageViewType = VK_IMAGE_VIEW_TYPE_1D;
    imageInfo.vkImageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.vkImageLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    break;
  case mg::TEXTURE_TYPE::TEXTURE_2D:
    imageInfo.vkImageType = VK_IMAGE_TYPE_2D;
    imageInfo.vkImageViewType = VK_IMAGE_VIEW_TYPE_2D;
    imageInfo.vkImageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.vkImageLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    break;
  case mg::TEXTURE_TYPE::TEXTURE_3D:
    imageInfo.vkImageType = VK_IMAGE_TYPE_3D;
    imageInfo.vkImageViewType = VK_IMAGE_VIEW_TYPE_3D;
    imageInfo.vkImageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.vkImageLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    break;
  case mg::TEXTURE_TYPE::ATTACHMENT:
    imageInfo.vkImageType = VK_IMAGE_TYPE_2D;
    imageInfo.vkImageViewType = VK_IMAGE_VIEW_TYPE_2D;
    imageInfo.vkImageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                                  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    imageInfo.vkImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    break;
  case mg::TEXTURE_TYPE::DEPTH:
    imageInfo.vkImageType = VK_IMAGE_TYPE_2D;
    imageInfo.vkImageViewType = VK_IMAGE_VIEW_TYPE_2D;
    imageInfo.vkImageUsageFlags =
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    imageInfo.vkImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    break;
  default:
    mgAssert(false);
  }

  return imageInfo;
}

static VkSampler samplerToVkSampler(mg::TEXTURE_SAMPLER sampler) {
  VkSampler res;
  switch (sampler) {
  case mg::TEXTURE_SAMPLER::POINT_CLAMP_TO_BORDER:
    res = mg::vkContext.sampler.pointBorderSampler;
    break;
  case mg::TEXTURE_SAMPLER::LINEAR_CLAMP_TO_BORDER:
    res = mg::vkContext.sampler.linearBorderSampler;
    break;
  case mg::TEXTURE_SAMPLER::LINEAR_CLAMP_TO_EDGE:
    res = mg::vkContext.sampler.linearEdgeSampler;
    break;
  case mg::TEXTURE_SAMPLER::LINEAR_REPEAT:
    res = mg::vkContext.sampler.linearRepeat;
    break;
  default:
    mgAssert(false);
  }
  return res;
}

static void createDescriptorSets(const mg::CreateTextureInfo &textureInfo, mg::_TextureData *texture) {
  for (uint32_t i = 0; i < textureInfo.textureSamplers.size(); i++) {
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = mg::vkContext.descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &mg::vkContext.descriptorSetLayout.texture;

    vkAllocateDescriptorSets(mg::vkContext.device, &descriptorSetAllocateInfo, &texture->descriptorSets[i]);
    texture->textureSamplers[i] = textureInfo.textureSamplers[i];
  }
}

static void updateDescriptorSets(const mg::CreateTextureInfo &textureInfo, mg::_TextureData *texture) {
  VkDescriptorImageInfo vkDescriptorImageInfo = {};
  vkDescriptorImageInfo.imageView = texture->imageView;
  vkDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkWriteDescriptorSet textureWrite = {};
  textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  textureWrite.dstBinding = 0;
  textureWrite.dstArrayElement = 0;
  textureWrite.descriptorCount = 1;
  textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  textureWrite.pImageInfo = &vkDescriptorImageInfo;

  for (uint32_t i = 0; i < textureInfo.textureSamplers.size(); i++) {
    vkDescriptorImageInfo.sampler = samplerToVkSampler(textureInfo.textureSamplers[i]);
    textureWrite.dstSet = texture->descriptorSets[i];
    vkUpdateDescriptorSets(mg::vkContext.device, 1, &textureWrite, 0, nullptr);
  }
}

static void createDeviceTexture(const mg::CreateTextureInfo &textureInfo, const ImageInfo &imageInfo, mg::_TextureData *texture) {
  // staging memory
  VkCommandBuffer copyCommandBuffer;
  VkBuffer stagingBuffer;
  VkDeviceSize stagingOffset;
  void *stagingMemory = mg::mgSystem.linearHeapAllocator.allocateStaging(textureInfo.sizeInBytes, 16, &copyCommandBuffer,
                                                                         &stagingBuffer, &stagingOffset);
  memcpy(stagingMemory, textureInfo.data, textureInfo.sizeInBytes);

  // https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
  // Pipeline barrier before the copy to perform a layout transition
  VkImageMemoryBarrier preCopyMemoryBarrier = {};
  preCopyMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  preCopyMemoryBarrier.srcAccessMask = 0;
  preCopyMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  preCopyMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
  preCopyMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  preCopyMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  preCopyMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  preCopyMemoryBarrier.image = texture->image;
  preCopyMemoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

  vkCmdPipelineBarrier(copyCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &preCopyMemoryBarrier);

  VkBufferImageCopy vkBufferImageCopy = {};
  vkBufferImageCopy.bufferOffset = stagingOffset;
  vkBufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  vkBufferImageCopy.imageSubresource.mipLevel = 0;
  vkBufferImageCopy.imageSubresource.layerCount = 1;
  vkBufferImageCopy.imageExtent = textureInfo.size;

  vkCmdCopyBufferToImage(copyCommandBuffer, stagingBuffer, texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &vkBufferImageCopy);

  // Pipeline barrier before using the image data
  VkImageMemoryBarrier postCopyMemoryBarrier = {};
  postCopyMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  postCopyMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  postCopyMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  postCopyMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  postCopyMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  postCopyMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  postCopyMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  postCopyMemoryBarrier.image = texture->image;
  postCopyMemoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

  vkCmdPipelineBarrier(copyCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &postCopyMemoryBarrier);

  VkImageViewCreateInfo vkImageViewCreateInfo = {};
  vkImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  vkImageViewCreateInfo.image = texture->image;
  vkImageViewCreateInfo.viewType = imageInfo.vkImageViewType;
  vkImageViewCreateInfo.format = textureInfo.format;
  vkImageViewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
                                      VK_COMPONENT_SWIZZLE_A};
  vkImageViewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  checkResult(vkCreateImageView(mg::vkContext.device, &vkImageViewCreateInfo, nullptr, &texture->imageView));
}

static void createAttachmentTexture(const mg::CreateTextureInfo &textureInfo, const ImageInfo &imageInfo,
                                    mg::_TextureData *texture) {

  VkImageViewCreateInfo vkImageViewCreateInfo = {};
  vkImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  vkImageViewCreateInfo.image = texture->image;
  vkImageViewCreateInfo.viewType = imageInfo.vkImageViewType;
  vkImageViewCreateInfo.format = textureInfo.format;
  vkImageViewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  checkResult(vkCreateImageView(mg::vkContext.device, &vkImageViewCreateInfo, nullptr, &texture->imageView));
}

static void createDepthTexture(const mg::CreateTextureInfo &textureInfo, mg::_TextureData *texture) {
  VkImageViewCreateInfo vkImageViewCreateInfo = {};
  vkImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  vkImageViewCreateInfo.subresourceRange = {};
  vkImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
  vkImageViewCreateInfo.subresourceRange.levelCount = 1;
  vkImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
  vkImageViewCreateInfo.subresourceRange.layerCount = 1;

  vkImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  vkImageViewCreateInfo.format = textureInfo.format;
  vkImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  vkImageViewCreateInfo.image = texture->image;

  checkResult(vkCreateImageView(mg::vkContext.device, &vkImageViewCreateInfo, nullptr, &texture->imageView));
}

TextureContainer::~TextureContainer() { mgAssert(_idToTexture.empty()); }

void TextureContainer::destroyTextureContainer() {
  for (auto &texture : _idToTexture) {
    destroyTexture(texture.first);
  }
  _idToTexture.clear();
}

void TextureContainer::createTexture(const CreateTextureInfo &textureInfo) {
  const auto imageInfo = createImageInfoFromType(textureInfo.type);

  mg::_TextureData texture = {};
  texture.nrOfDescriptorSets = uint32_t(textureInfo.textureSamplers.size());
  texture.format = textureInfo.format;

  VkImageCreateInfo imageCreateInfo = {};
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.imageType = imageInfo.vkImageType;
  imageCreateInfo.format = textureInfo.format;
  imageCreateInfo.extent = textureInfo.size;
  imageCreateInfo.mipLevels = 1;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.usage = imageInfo.vkImageUsageFlags;
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageCreateInfo.initialLayout = imageInfo.vkImageLayout;
  checkResult(vkCreateImage(mg::vkContext.device, &imageCreateInfo, nullptr, &texture.image));

  VkMemoryRequirements vkMemoryRequirements;
  vkGetImageMemoryRequirements(mg::vkContext.device, texture.image, &vkMemoryRequirements);
  const auto memoryIndex = findMemoryTypeIndex(mg::vkContext.physicalDeviceMemoryProperties, vkMemoryRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  texture.heapAllocation = mg::mgSystem.textureDeviceMemoryAllocator.allocateDeviceOnlyMemory(
      memoryIndex, vkMemoryRequirements.size, vkMemoryRequirements.alignment);
  checkResult(
      vkBindImageMemory(mg::vkContext.device, texture.image, texture.heapAllocation.deviceMemory, texture.heapAllocation.offset));

  switch (textureInfo.type) {
  case TEXTURE_TYPE::TEXTURE_1D:
  case TEXTURE_TYPE::TEXTURE_2D:
  case TEXTURE_TYPE::TEXTURE_3D:
    createDeviceTexture(textureInfo, imageInfo, &texture);
    break;
  case TEXTURE_TYPE::ATTACHMENT:
    createAttachmentTexture(textureInfo, imageInfo, &texture);
    break;
  case TEXTURE_TYPE::DEPTH:
    createDepthTexture(textureInfo, &texture);
    break;
  default:
    mgAssert(false);
  };

  createDescriptorSets(textureInfo, &texture);
  updateDescriptorSets(textureInfo, &texture);
  _idToTexture.emplace(textureInfo.id, texture);
}

Texture TextureContainer::getTexture(const std::string &id, TEXTURE_SAMPLER sampler) {
  auto textureIt = _idToTexture.find(id);
  mgAssert(textureIt != std::end(_idToTexture));
  const auto &textureData = textureIt->second;

  if (sampler == TEXTURE_SAMPLER::NONE)
    mgAssert(textureData.nrOfDescriptorSets == 1);

  for (uint32_t i = 0; i < textureData.nrOfDescriptorSets; i++) {
    if (textureData.textureSamplers[i] == sampler || sampler == TEXTURE_SAMPLER::NONE) {
      Texture texture = {};
      texture.id = id;
      texture.descriptorSet = textureData.descriptorSets[i];
      texture.imageView = textureData.imageView;
      texture.format = textureData.format;
      return texture;
    }
  }
  mgAssertDesc(false, "Could not found descriptorSet matching the correct sampler" << uint32_t(sampler));
  return {};
}

void TextureContainer::destroyTexture(const std::string &id) {
  const auto textureIt = _idToTexture.find(id);
  mgAssert(textureIt != std::end(_idToTexture));
  const auto &texture = textureIt->second;
  vkDestroyImage(mg::vkContext.device, texture.image, nullptr);
  vkDestroyImageView(mg::vkContext.device, texture.imageView, nullptr);
  for (uint32_t i = 0; i < _TextureData::MAX_DESCRIPTOR_SET; i++) {
    if (texture.descriptorSets[i] != nullptr) {
      vkFreeDescriptorSets(mg::vkContext.device, mg::vkContext.descriptorPool, 1, &texture.descriptorSets[i]);
    }
  }
  mgSystem.textureDeviceMemoryAllocator.freeDeviceOnlyMemory(texture.heapAllocation);
}

void TextureContainer::removeTexture(const std::string &id) {
  destroyTexture(id);
  _idToTexture.erase(id);
}

} // namespace mg