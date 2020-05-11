#pragma once
#include "makelevelset3.h"
#include <cinttypes>

struct Sdf {
  std::vector<float> grid;
  uint32_t x, y, z;
  Vec3f minBox;
  Vec3f maxBox;
};

Sdf objToSdf(std::string objPath, float dx, uint32_t padding);
