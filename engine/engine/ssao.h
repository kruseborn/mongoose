#pragma once
#include "glm\glm.hpp"

struct KernelData {
	static const int kernelSize = 16;
	static const int noiseSize = 4;
	static const int blurSize = 2;
	glm::vec2 noiseScale;
	glm::vec4 kernel[16] = {};
};
extern KernelData ssaoKernelData;