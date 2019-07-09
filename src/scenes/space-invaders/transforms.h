#pragma once
#include "invaders_utils.h"

struct Hashmap;
struct Aliens;

void transformPositions(float *xPositions, float *yPositions, uint32_t size, vec2 direction, float speed, float dt);
MinMax transformPositionsCalculateLimits(float *xPositions, float *yPositions, uint32_t size, vec2 direction, float speed, float dt);
void addTransformToGrid(const float *xPositions, const float *yPositions, uint32_t size, Hashmap *hashmap, float cellSize);
void changeAlienDirectionIfNeeded(Aliens *aliens, MinMax minMax, Limits limits, float dt);