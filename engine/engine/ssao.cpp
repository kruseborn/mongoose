#include <cstdlib>
#include "glm/glm.hpp"
#include "glm/gtx/compatibility.hpp"
#include "vkUtils.h"
#include "ssao.h"
#include "textures.h"
#include <random>


std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between 0.0 - 1.0
std::default_random_engine generator;

KernelData ssaoKernelData;

void createNoiseTexture(Texture *texture) {
	int kernelSize = 16;
	for (int i = 0; i < kernelSize; ++i) {
		glm::vec3 sample(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator)
		);
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / kernelSize;
		scale = glm::lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernelData.kernel[i] = glm::vec4(sample, 1.0);
	}

	glm::vec3 noiseData[16] = {};
	for (int i = 0; i < 16; i++) {
		glm::vec3 noise(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			0.0f);
		noiseData[i] = noise;
	}
	createDeviceTexture(texture, sizeof(glm::vec3)*ssaoKernelData.kernelSize, ssaoKernelData.noiseSize, ssaoKernelData.noiseSize, noiseData, VK_FORMAT_R32G32B32_SFLOAT);
	ssaoKernelData.noiseScale = glm::vec2(vkContext.width / ssaoKernelData.noiseSize, vkContext.height / ssaoKernelData.noiseSize);
}
