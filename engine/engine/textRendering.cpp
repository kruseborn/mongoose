#pragma once
#include "rendering.h"
#include "stb_font_consolas_24_latin1.inl"
#include <cstdint>
#include <cassert>
#include "vulkanContext.h"
#include "vkDefs.h"
#include "vkUtils.h"
#include "glm/gtc/matrix_transform.hpp"
#include "textures.h"
#include "pipelines.h"

// Defines for the STB font used
// STB font files can be found at http://nothings.org/stb/font/
#define STB_FONT_NAME stb_font_consolas_24_latin1
#define STB_FONT_WIDTH STB_FONT_consolas_24_latin1_BITMAP_WIDTH
#define STB_FONT_HEIGHT STB_FONT_consolas_24_latin1_BITMAP_HEIGHT 
#define STB_FIRST_CHAR STB_FONT_consolas_24_latin1_FIRST_CHAR
#define STB_NUM_CHARS STB_FONT_consolas_24_latin1_NUM_CHARS

#define MAX_CHAR_COUNT 1024

static 	stb_fontchar stbFontData[STB_NUM_CHARS];

struct Ubo {
	glm::mat4 mvp;
};

void createFontTexture(Texture *texture) {
	unsigned char fontpixels[STB_FONT_HEIGHT][STB_FONT_WIDTH];
	STB_FONT_NAME(stbFontData, fontpixels, STB_FONT_HEIGHT);

	createDeviceTexture(texture, STB_FONT_WIDTH * STB_FONT_HEIGHT, STB_FONT_WIDTH, STB_FONT_HEIGHT, fontpixels, VK_FORMAT_R8_UNORM);
}

void renderText(char *text, float x, float y, TextAlign align) {
	static Pipeline *pipeline = Pipelines::findPipeline("drawText");
	const float charW = 1.5f / vkContext.width;
	const float charH = 1.5f / vkContext.height;

	float fbW = (float)vkContext.width;
	float fbH = (float)vkContext.height;
	x = (x / fbW * 2.0f) - 1.0f;
	y = (y / fbH * 2.0f) - 1.0f;

	// Calculate text width
	float textWidth = 0;
	uint32_t nrOfLetters = 0;
	for (char *letter = text; *letter != 0; letter++) {
		stb_fontchar *charData = &stbFontData[(uint32_t)*letter - STB_FIRST_CHAR];
		textWidth += charData->advance * charW;
		nrOfLetters++;
	}
	switch (align) {
	case alignRight:
		x -= textWidth;
		break;
	case alignCenter:
		x -= textWidth / 2.0f;
		break;
	case alignLeft:
		break;
	}
	VkBuffer buffer;
	VkDeviceSize buffer_offset;
	PositionVertex* vertices = (PositionVertex*)allocateBuffer(nrOfLetters * 4 * sizeof(PositionVertex), &buffer, &buffer_offset);

	VkBuffer indexBuffer;
	VkDeviceSize indexBufferOffset;
	uint32_t * indices = (uint32_t*)allocateBuffer(nrOfLetters * 6 * sizeof(uint32_t), &indexBuffer, &indexBufferOffset);

	uint32_t vertexIndex = 0;
	uint32_t indicesIndex = 0;
	for (char *letter = text; *letter != 0; letter++, vertexIndex += 4, indicesIndex += 6) {
		stb_fontchar *charData = &stbFontData[(uint32_t)*letter - STB_FIRST_CHAR];

		vertices[vertexIndex + 0].position.x = (x + (float)charData->x0 * charW);
		vertices[vertexIndex + 0].position.y = (y + (float)charData->y0 * charH);
		vertices[vertexIndex + 0].position.z = charData->s0;
		vertices[vertexIndex + 0].position.w = charData->t0;

		vertices[vertexIndex + 1].position.x = (x + (float)charData->x1 * charW);
		vertices[vertexIndex + 1].position.y = (y + (float)charData->y0 * charH);
		vertices[vertexIndex + 1].position.z = charData->s1;
		vertices[vertexIndex + 1].position.w = charData->t0;

		vertices[vertexIndex + 2].position.x = (x + (float)charData->x1 * charW);
		vertices[vertexIndex + 2].position.y = (y + (float)charData->y1 * charH);
		vertices[vertexIndex + 2].position.z = charData->s1;
		vertices[vertexIndex + 2].position.w = charData->t1;

		vertices[vertexIndex + 3].position.x = (x + (float)charData->x0 * charW);
		vertices[vertexIndex + 3].position.y = (y + (float)charData->y1 * charH);
		vertices[vertexIndex + 3].position.z = charData->s0;
		vertices[vertexIndex + 3].position.w = charData->t1;
		x += charData->advance * charW;

		indices[indicesIndex + 0] = vertexIndex + 0; indices[indicesIndex + 1] = vertexIndex + 2; indices[indicesIndex + 2] = vertexIndex + 1;
		indices[indicesIndex + 3] = vertexIndex + 0; indices[indicesIndex + 4] = vertexIndex + 3; indices[indicesIndex + 5] = vertexIndex + 2;
	}
	VkBuffer uniform_buffer;
	uint32_t uniform_offset;
	VkDescriptorSet ubo_set;
	Ubo *ubo = (Ubo*)allocateUniform(sizeof(Ubo), &uniform_buffer, &uniform_offset, &ubo_set);

	ubo->mvp = glm::mat4(1.0f);
	static Texture *fontTexture = Textures::findTexture("font");
	VkDescriptorSet descriptor_sets[2] = { ubo_set, fontTexture->descriptorSet };
	vkCmdBindDescriptorSets(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 2, descriptor_sets, 1, &uniform_offset);
	vkCmdBindPipeline(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

	vkCmdBindVertexBuffers(vkContext.commandBuffer, 0, 1, &buffer, &buffer_offset);
	vkCmdBindIndexBuffer(vkContext.commandBuffer, indexBuffer, indexBufferOffset, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(vkContext.commandBuffer, nrOfLetters * 6, 1, 0, 0, 0);
}
