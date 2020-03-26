#include "navier_stroke_compute.h"
#include "mg/mgSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace mg {

ComputeData createCompute2() {
  //ComputeData computeData = {};
  //computeData.N = 256;
  //uint32_t size = (computeData.N + 2) * (computeData.N + 2);

  //std::vector<glm::vec2> uv(size);

  // computeData.velocityId = mg::mgSystem.storageContainer.createStorage(uv.data(), mg::sizeofContainerInBytes(uv));
  // computeData.velocityId = mg::mgSystem.storageContainer.createStorage(uv.data(), mg::sizeofContainerInBytes(uv));
  return {};
}

void addVec(ComputeData &computeData) {}
} // namespace mg