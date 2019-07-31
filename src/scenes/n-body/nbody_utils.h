#pragma once
#include <cinttypes>
#include "mg/storageContainer.h"
#include "mg/textureContainer.h"
struct ComputeData {
  uint32_t nrOfParticles;
  uint32_t workgroupSize = 256;
  mg::StorageId storageId;
  mg::TextureId particleId;
};

void initParticles(ComputeData *computeData);
