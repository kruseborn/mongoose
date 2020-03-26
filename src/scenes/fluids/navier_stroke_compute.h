#pragma once
#include <cinttypes>
#include "mg/storageContainer.h"

namespace mg {

  struct ComputeData {
    mg::StorageId velocityId, prevVelocityId, density, prevDensity;
  };

  ComputeData createCompute2();

  void addVec(ComputeData &computeData);

}// namespace mg