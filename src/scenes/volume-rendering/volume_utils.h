#pragma once
#include <glm/glm.hpp>
#include <mg/textureContainer.h>
#include <mg/meshUtils.h>

struct VolumeInfo {
  glm::vec3 corner;
  glm::ivec3 nrOfVoxels;
  glm::vec3 voxelSize;
  glm::vec3 size;
  uint16_t min, max;
  mg::TextureId textureId;
};

VolumeInfo parseDatFile();

glm::mat4 worldToBoxMatrix(const VolumeInfo &volumeInfo);
glm::mat4 boxToWorldMatrix(const VolumeInfo &volumeInfo);
