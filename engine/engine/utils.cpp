#include "utils.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <sys/stat.h>
#pragma warning( disable : 4996)

void* readBinaryDataFromFile(const char *fileName) {
	struct stat info;
	if (stat(fileName, &info) != 0) {
		printf("could not open file");
		exit(1);
	}

	void *content = malloc(info.st_size);
	FILE *fp = fopen(fileName, "rb");
	size_t blocks_read = fread(content, info.st_size, 1, fp);
	if (blocks_read != 1) {
		printf("could not get data from file\n");
		exit(1);
	}
	fclose(fp);
	return content;
}

glm::vec2 cartesianToSpherical(glm::vec3 cartesian) {
	glm::vec2 spherical;

	spherical.x = glm::atan(cartesian.y, cartesian.x) / 3.14159f;
	spherical.y = cartesian.z;

	return spherical * 0.5f + 0.5f;
}

glm::vec3 sphericalToCartesian(glm::vec2 spherical) {
	glm::vec2 sinCosTheta, sinCosPhi;

	spherical = spherical * 2.0f - 1.0f;
	sinCosTheta.x = sin(spherical.x * 3.14159f);
	sinCosTheta.y = cos(spherical.x * 3.14159f);
	sinCosPhi = glm::vec2(sqrt(1.0 - spherical.y * spherical.y), spherical.y);

	return glm::vec3(sinCosTheta.y * sinCosPhi.x, sinCosTheta.x * sinCosPhi.x, sinCosPhi.y);
}
