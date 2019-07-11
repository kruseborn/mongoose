#pragma once
#include <cinttypes>
#include "mg/storageContainer.h"

struct ComputeData {
  uint32_t width = 3200;
  uint32_t height = 2400;
  uint32_t workgroupSize = 32;
  mg::StorageId storageId;
};

void initParticles();
