#include "deferred.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "vulkanContext.h"
#include "meshes.h"

#include "vulkanContext.h"
#include "vkDefs.h"
#include "pipelines.h"
#include "textures.h"
#include "geometry.h"
#include "textures.h"
#include "ssao.h"
#include <random>



void renderMRT(const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix) {
	static Pipeline *mrtPipeline = Pipelines::findPipeline("mrt");
	vkCmdBindPipeline(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mrtPipeline->pipeline);

	VkBuffer uniform_buffer;
	uint32_t uniform_offset;
	VkDescriptorSet ubo_set;
	struct UboMesh {
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
		glm::mat4 mNormal;
	};
	UboMesh * ubo = (UboMesh*)allocateUniform(sizeof(UboMesh), &uniform_buffer, &uniform_offset, &ubo_set);

	ubo->projection = projectionMatrix;
	ubo->view = viewMatrix;
	ubo->model = glm::mat4();

	glm::mat4 mNormal = glm::mat4(glm::transpose(glm::inverse(glm::mat3(viewMatrix))));

	ubo->mNormal = mNormal;
	VkDescriptorSet descriptor_sets[1] = { ubo_set };
	vkCmdBindDescriptorSets(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mrtPipeline->layout, 0, 1, descriptor_sets, 1, &uniform_offset);

	Mesh *meshes = nullptr;
	uint32_t nrOfMeshes;
	Meshes::getMeshes(&meshes, &nrOfMeshes);
	for (uint32_t i = 0; i < nrOfMeshes; i++) {
		Mesh &mesh = meshes[i];
		vkCmdBindVertexBuffers(vkContext.commandBuffer, 0, 1, &mesh.buffer, &mesh.verticesOffset);
		vkCmdBindIndexBuffer(vkContext.commandBuffer, mesh.buffer, mesh.indicesOffset, VK_INDEX_TYPE_UINT32);
		vkCmdPushConstants(vkContext.commandBuffer, mrtPipeline->layout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(MeshProperties), (void*)&mesh.properties);
		vkCmdDrawIndexed(vkContext.commandBuffer, mesh.indexCount, 1, 0, 0, 0);
	}
}

void renderSSAO(const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix) {
	static Pipeline *ssaoPipeline = Pipelines::findPipeline("ssao");
	VkBuffer buffer;
	VkDeviceSize bufferOffset;
	PositionVertex *vertices = (PositionVertex*)allocateBuffer(sizeof(PositionVertex) * 4, &buffer, &bufferOffset);


	VkBuffer indexBuffer;
	VkDeviceSize indexBufferOffset;
	uint32_t *indices = (uint32_t*)allocateBuffer(sizeof(uint32_t) * 6, &indexBuffer, &indexBufferOffset);

	geometry::fullscreenCube(vertices, indices);

	VkBuffer uniform_buffer;
	uint32_t uniform_offset;
	VkDescriptorSet ubo_set;

	struct SSAOUBO {
		glm::mat4 projection;
		glm::mat4 mvp;
		glm::vec4 kernel[16];
		glm::vec2 noiseScale;
	};
	SSAOUBO * ubo = (SSAOUBO*)allocateUniform(sizeof(SSAOUBO), &uniform_buffer, &uniform_offset, &ubo_set);
	ubo->mvp = glm::mat4();
	memcpy(ubo->kernel, ssaoKernelData.kernel, sizeof(glm::vec4) * 16);
	ubo->projection = projectionMatrix;
	ubo->noiseScale = ssaoKernelData.noiseScale;

	static Texture *noiseTexture = Textures::findTexture("noise");
	static Texture *normalTexture = Textures::findTexture("normal");
	static Texture *depthTexture = Textures::findTexture("depth");
	
	VkDescriptorSet descriptor_sets[4] = { ubo_set, normalTexture->descriptorSet, noiseTexture->descriptorSet, depthTexture->descriptorSet };
	vkCmdBindDescriptorSets(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ssaoPipeline->layout, 0, 4, descriptor_sets, 1, &uniform_offset);

	vkCmdBindPipeline(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ssaoPipeline->pipeline);
	vkCmdBindVertexBuffers(vkContext.commandBuffer, 0, 1, &buffer, &bufferOffset);
	vkCmdBindIndexBuffer(vkContext.commandBuffer, indexBuffer, indexBufferOffset, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(vkContext.commandBuffer, 6, 1, 0, 0, 0);
}

struct Light {
	glm::vec4 position;
	glm::vec3 color;
	float radius;
};
struct UboFinal {
	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 model;
	glm::mat4 mvp;
	Light lights[32];
};
static void setLightPositions(UboFinal *ubo) {
	std::uniform_real_distribution<float> x(-300.0, 300.0);
	std::uniform_real_distribution<float> y(40, 60);
	std::uniform_real_distribution<float> z(-200, 200);

	std::uniform_real_distribution<float> color(0.0, 1.0);
	std::uniform_real_distribution<float> radius(1000, 3000);

	//101 107
	std::default_random_engine random(101);

	for (uint32_t i = 0; i < 32; i++) {
		ubo->lights[i].position = glm::vec4(x(random), y(random), z(random), 1.0f);
		ubo->lights[i].color = glm::vec3(color(random), color(random), color(random));
		ubo->lights[i].radius = radius((random));

	}
}

void renderBlurSSAO(const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix) {
	static Pipeline *pipeline = Pipelines::findPipeline("ssaoBlur");
	VkBuffer buffer;
	VkDeviceSize bufferOffset;
	PositionVertex *vertices = (PositionVertex*)allocateBuffer(sizeof(PositionVertex) * 4, &buffer, &bufferOffset);

	VkBuffer indexBuffer;
	VkDeviceSize indexBufferOffset;
	uint32_t *indices = (uint32_t*)allocateBuffer(sizeof(uint32_t) * 6, &indexBuffer, &indexBufferOffset);

	geometry::fullscreenCube(vertices, indices);

	struct Ubo {
		glm::mat4 mvp;
		glm::vec2 size;
	};
	VkBuffer uniform_buffer;
	uint32_t uniform_offset;
	VkDescriptorSet ubo_set;
	Ubo * ubo = (Ubo*)allocateUniform(sizeof(Ubo), &uniform_buffer, &uniform_offset, &ubo_set);
	ubo->mvp = glm::mat4();
	ubo->size = glm::vec2(vkContext.width, vkContext.height);

	static Texture *ssaoTexture = Textures::findTexture("ssao");

	VkDescriptorSet descriptor_sets[2] = { ubo_set, ssaoTexture->descriptorSet };
	vkCmdBindDescriptorSets(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 2, descriptor_sets, 1, &uniform_offset);

	vkCmdBindPipeline(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
	vkCmdBindVertexBuffers(vkContext.commandBuffer, 0, 1, &buffer, &bufferOffset);
	vkCmdBindIndexBuffer(vkContext.commandBuffer, indexBuffer, indexBufferOffset, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(vkContext.commandBuffer, 6, 1, 0, 0, 0);
}

void renderFinalDeferred(const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix) {
	static Pipeline *deferredPipeline = Pipelines::findPipeline("deferred");
	VkBuffer buffer;
	VkDeviceSize bufferOffset;
	PositionVertex *vertices = (PositionVertex*)allocateBuffer(sizeof(PositionVertex) * 4, &buffer, &bufferOffset);

	VkBuffer indexBuffer;
	VkDeviceSize indexBufferOffset;
	uint32_t *indices = (uint32_t*)allocateBuffer(sizeof(uint32_t) * 6, &indexBuffer, &indexBufferOffset);

	geometry::fullscreenCube(vertices, indices);

	VkBuffer uniform_buffer;
	uint32_t uniform_offset;
	VkDescriptorSet ubo_set;
	UboFinal * ubo = (UboFinal*)allocateUniform(sizeof(UboFinal), &uniform_buffer, &uniform_offset, &ubo_set);

	ubo->view = viewMatrix;
	ubo->model = glm::mat4();
	ubo->mvp = glm::mat4();
	ubo->projection = projectionMatrix;
	setLightPositions(ubo);

	static VkDescriptorSet normalTexture = Textures::findTexture("normal")->descriptorSet;
	static VkDescriptorSet albedoTexture = Textures::findTexture("albedo")->descriptorSet;
	static VkDescriptorSet ssaoBlured = Textures::findTexture("ssaoblur")->descriptorSet;
	static VkDescriptorSet depthTexture = Textures::findTexture("depth")->descriptorSet;

	VkDescriptorSet descriptor_sets[5] = { ubo_set, normalTexture, albedoTexture, ssaoBlured, depthTexture };
	vkCmdBindDescriptorSets(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredPipeline->layout, 0, 5, descriptor_sets, 1, &uniform_offset);

	vkCmdBindPipeline(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredPipeline->pipeline);
	vkCmdBindVertexBuffers(vkContext.commandBuffer, 0, 1, &buffer, &bufferOffset);
	vkCmdBindIndexBuffer(vkContext.commandBuffer, indexBuffer, indexBufferOffset, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(vkContext.commandBuffer, 6, 1, 0, 0, 0);
}
