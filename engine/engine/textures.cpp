#include "textures.h"
#include <unordered_map>
#include "vulkanContext.h"
#include "cassert"
#include <cstring>
#include "deferredRenderPass.h"

namespace Textures {
	std::unordered_map<std::string, Texture> textures;
	void init() {
		Texture *fontTexture = Textures::newTexture("font");
		Texture *noiseTexture = Textures::newTexture("noise");

		createFontTexture(fontTexture);
		createNoiseTexture(noiseTexture);

		createDeferredTextures();
	}

	void deleteTexture(Texture texture) {
		vkDestroyImage(vkContext.device, texture.image, nullptr);
		vkDestroyImageView(vkContext.device, texture.imageView, nullptr);
		vkFreeMemory(vkContext.device, texture.deviceMemory, nullptr);
		vkDestroySampler(vkContext.device, texture.sampler, nullptr);
	}
	void destroy() {
		for (auto &texture : textures) {
			deleteTexture(texture.second);
		}
		textures.clear();
	}

	Texture *findTexture(const char* name) {
		auto res = textures.find(name);
		assert(res != std::end(textures));
		return &res->second;
	}
	Texture* newTexture(const char *name) {
		uint32_t strLenght = (uint32_t)strlen(name);
		assert(strLenght < MAX_TEXTURE_NAME_SIZE);
		Texture texture = {};
		strncpy(texture.name, name, strLenght + 1);

		textures.emplace(texture.name, texture);
		return &textures.find(texture.name)->second;
	}
	void freeTexture(const char* name) {
		auto res = textures.find(name);
		assert(res != std::end(textures));
		deleteTexture(res->second);
		textures.erase(res);
	}
}
