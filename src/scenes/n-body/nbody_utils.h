#pragma once
#include <cinttypes>
#include "mg/storageContainer.h"

struct ComputeData {
  uint32_t nrOfParticles;
  uint32_t workgroupSize = 256;
  mg::StorageId storageId;
};

void initParticles(ComputeData *computeData);
