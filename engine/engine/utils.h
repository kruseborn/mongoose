#pragma once
#include <malloc.h>
#include <glm/glm.hpp>

#define maxStackAllocation 1024
#define stackAlloc(size) \
(                        \
	_alloca(size)        \
)

void *readBinaryDataFromFile(const char *fileName);

glm::vec2 cartesianToSpherical(glm::vec3 cartesian);
glm::vec3 sphericalToCartesian(glm::vec2 spherical);
