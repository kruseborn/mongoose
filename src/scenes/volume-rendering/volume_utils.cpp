#include "volume_utils.h"
#include "mg/mgSystem.h"
#include "mg/mgUtils.h"
#include <algorithm>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

void uploadVolumeToGpu(const std::vector<uint16_t> &data, const glm::uvec3 &size) {
  mg::CreateTextureInfo createTextureInfo = {};
  createTextureInfo.id = "stagbeetle";
  createTextureInfo.textureSamplers = {mg::TEXTURE_SAMPLER::LINEAR_CLAMP_TO_BORDER};
  createTextureInfo.type = mg::TEXTURE_TYPE::TEXTURE_3D;
  createTextureInfo.size = {size.x, size.y, size.z};
  createTextureInfo.data = (void *)data.data();
  createTextureInfo.sizeInBytes = mg::sizeofContainerInBytes(data);
  createTextureInfo.format = VK_FORMAT_R16_UNORM;

  mg::mgSystem.textureContainer.createTexture(createTextureInfo);
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

  uploadVolumeToGpu(data, {sizeX, sizeY, sizeZ});

  VolumeInfo volumeInfo = {};
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
  for (uint32_t i = 0; i < uint32_t(data.size()); i++) {
    volumeInfo.min = std::min(volumeInfo.min, data[i]);
    volumeInfo.max = std::max(volumeInfo.max, data[i]);
  }
  return volumeInfo;
}

//   v6----- v5
//   /|      /|
//  v2------v3|
//  | |     | |
//  | |v7---|-|v4
//  |/      |/
//  v0------v1
VolumeCube createVolumeCube(const VolumeInfo &volumeInfo) {
  const auto boxToWorld = boxToWorldMatrix(volumeInfo);

  const auto v0 = glm::vec3{boxToWorld * glm::vec4{0, 0, 0, 1}};
  const auto v1 = glm::vec3{boxToWorld * glm::vec4{1, 0, 0, 1}};
  const auto v2 = glm::vec3{boxToWorld * glm::vec4{0, 1, 0, 1}};
  const auto v3 = glm::vec3{boxToWorld * glm::vec4{1, 1, 0, 1}};

  const auto v4 = glm::vec3{boxToWorld * glm::vec4{1, 0, 1, 1}};
  const auto v5 = glm::vec3{boxToWorld * glm::vec4{1, 1, 1, 1}};
  const auto v6 = glm::vec3{boxToWorld * glm::vec4{0, 1, 1, 1}};
  const auto v7 = glm::vec3{boxToWorld * glm::vec4{0, 0, 1, 1}};

  VolumeCube volumeCube = {{v0, v1, v2, v3, v4, v5, v6, v7}, {0, 1, 3, 3, 2, 0,   // front
                                                              1, 4, 3, 4, 5, 3,   // right
                                                              7, 0, 2, 7, 2, 6,   // left
                                                              2, 3, 5, 2, 5, 6,   // top
                                                              4, 1, 0, 4, 0, 7,   // bottom
                                                              4, 7, 5, 7, 6, 5}}; // back
  return volumeCube;
}

glm::mat4 worldToBoxMatrix(const VolumeInfo &volumeInfo) { return glm::inverse(boxToWorldMatrix(volumeInfo)); }

glm::mat4 boxToWorldMatrix(const VolumeInfo &volumeInfo) {
  const auto scale = glm::scale(glm::mat4{1}, volumeInfo.size);
  const auto transform = glm::translate(glm::mat4{1}, volumeInfo.corner);
  return transform * scale;
}