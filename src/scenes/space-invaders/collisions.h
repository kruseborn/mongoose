#pragma once

#include "array.h"

struct Hashmap;

struct CheckCollisionInfo {
  struct {
    const float *x;
    const float *y;
    float radius;
    uint32_t size;
  } gridObjects, collisionObjects;
  float cellSize;
};

struct CollisionIds {
  Array<uint32_t> gridObjects;
  Array<uint32_t> collisionObjects;
};

void getCollisionIds(const Hashmap &hashmap, const CheckCollisionInfo &checkCollisionInfo, CollisionIds *ids);
