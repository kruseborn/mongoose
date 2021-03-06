#include "volume_utils.h"
#include "mg/mgSystem.h"
#include "mg/mgUtils.h"
#include <algorithm>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

static mg::TextureId uploadVolumeToGpu(const std::vector<uint16_t> &data, const glm::uvec3 &size) {
  mg::CreateTextureInfo createTextureInfo = {};
  createTextureInfo.id = "stagbeetle";
  createTextureInfo.type = mg::TEXTURE_TYPE::TEXTURE_3D;
  createTextureInfo.size = {size.x, size.y, size.z};
  createTextureInfo.data = (void *)data.data();
  createTextureInfo.sizeInBytes = mg::sizeofContainerInBytes(data);
  createTextureInfo.format = VK_FORMAT_R16_UNORM;

  return mg::mgSystem.textureContainer.createTexture(createTextureInfo);
}

VolumeInfo parseDatFile() {
  const auto filePath = mg::getDataPath() + "stagbeetle_dat/stagbeetle277x277x164.dat";
  auto stream = std::ifstream(filePath, std::fstream::binary);
  uint16_t sizeX, sizeY, sizeZ;
  stream.read((char *)&sizeX, sizeof(uint16_t));
  stream.read((char *)&sizeY, sizeof(uint16_t));
  stream.read((char *)&sizeZ, sizeof(uint16_t));

  std::vector<uint16_t> data(sizeX * sizeY * sizeZ);
  stream.read((char *)data.data(), mg::sizeofContainerInBytes(data));

  VolumeInfo volumeInfo = {};

  volumeInfo.textureId = uploadVolumeToGpu(data, {sizeX, sizeY, sizeZ});

  volumeInfo.corner = {0, 0, 0};
  volumeInfo.voxelSize = {1, 1, 1};
  volumeInfo.nrOfVoxels = {277, 277, 164};
  volumeInfo.min = std::numeric_limits<uint16_t>::max();
  volumeInfo.max = std::numeric_limits<uint16_t>::min();
  volumeInfo.size = {
      volumeInfo.nrOfVoxels.x * volumeInfo.voxelSize.x,
      volumeInfo.nrOfVoxels.y * volumeInfo.voxelSize.y,
      volumeInfo.nrOfVoxels.z * volumeInfo.voxelSize.z,
  };
  int count = 0;
  int index = 0;
  for (uint32_t i = 0; i < uint32_t(data.size()); i++) {
    volumeInfo.min = std::min(volumeInfo.min, data[i]);
    volumeInfo.max = std::max(volumeInfo.max, data[i]);
    if (data[i] > 2000) {
      count++;
      index = i;
    }
  }
  return volumeInfo;
}

glm::mat4 worldToBoxMatrix(const VolumeInfo &volumeInfo) { return glm::inverse(boxToWorldMatrix(volumeInfo)); }

glm::mat4 boxToWorldMatrix(const VolumeInfo &volumeInfo) {
  const auto scale = glm::scale(glm::mat4{1}, volumeInfo.size);
  const auto transform = glm::translate(glm::mat4{1}, volumeInfo.corner);
  return transform * scale;
}