#include "rendering.h"
#include "vulkanContext.h"
#include "vkDefs.h"
#include "glm/gtc/matrix_transform.hpp"
#include <cstdio>
#include "meshes.h"
#include "pipelines.h"
struct Ubo {
	glm::mat4 mvp;
};

static Pipeline *textPipeline = nullptr;

void renderTextureBox(float x, float y, float w, float h, VkDescriptorSet texture, Pipeline *pipeline) {
	VkBuffer buffer;
	VkDeviceSize buffer_offset;
	PositionVertex* vertices = (PositionVertex*)allocateBuffer(6 * sizeof(PositionVertex), &buffer, &buffer_offset);

	PositionVertex corner_verts[4] = {};

	corner_verts[0].position.x = x;
	corner_verts[0].position.y = y;
	corner_verts[0].position.z = 0.0f;
	corner_verts[0].position.w = 0.0f;

	corner_verts[1].position.x = x + float(w);
	corner_verts[1].position.y = y;
	corner_verts[1].position.z = 1.0f;
	corner_verts[1].position.w = 0.0f;

	corner_verts[2].position.x = x + float(w);
	corner_verts[2].position.y = y + float(h);
	corner_verts[2].position.z = 1.0f;
	corner_verts[2].position.w = 1.0f;

	corner_verts[3].position.x = x;
	corner_verts[3].position.y = y + float(h);
	corner_verts[3].position.z = 0.0f;
	corner_verts[3].position.w = 1.0f;

	vertices[0] = corner_verts[0];
	vertices[1] = corner_verts[1];
	vertices[2] = corner_verts[2];
	vertices[3] = corner_verts[2];
	vertices[4] = corner_verts[3];
	vertices[5] = corner_verts[0];

	VkBuffer uniform_buffer;
	uint32_t uniform_offset;
	VkDescriptorSet ubo_set;
	Ubo * ubo = (Ubo*)allocateUniform(sizeof(Ubo), &uniform_buffer, &uniform_offset, &ubo_set);
	ubo->mvp = glm::mat4();

	VkDescriptorSet descriptor_sets[2] = { ubo_set, texture };
	vkCmdBindDescriptorSets(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 2, descriptor_sets, 1, &uniform_offset);

	vkCmdBindVertexBuffers(vkContext.commandBuffer, 0, 1, &buffer, &buffer_offset);
	vkCmdBindPipeline(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
	vkCmdDraw(vkContext.commandBuffer, 6, 1, 0, 0);
}

//globals
float realtime;
float deltaTime;
uint32_t framecount;
static double frame_ms = 0.0;

void renderFps() {
	static double oldtime = 0;
	static double lastfps = 0;
	static int oldframecount = 0;

	double elapsed_time = realtime - oldtime;
	int frames = framecount - oldframecount;

	if (elapsed_time < 0 || frames < 0) {
		oldtime = realtime;
		oldframecount = framecount;
		return;
	}
	// update value every 3/4 second
	if (elapsed_time > 0.75) {
		lastfps = frames / elapsed_time;
		oldtime = realtime;
		oldframecount = framecount;
		frame_ms = elapsed_time / frames * 1000.0;
	}
	char	 st[20];
	snprintf(st, sizeof(st), "%4.0f fps, %4.0f ms", lastfps, frame_ms);
	renderText(st, 10, 10, TextAlign::alignLeft);
}


void renderMeshes(MeshManager &meshManager) {
	static Pipeline *polygonPipeline = Pipelines::findPipeline("polygon");
	VkBuffer uniform_buffer;
	uint32_t uniform_offset;
	VkDescriptorSet ubo_set;
	struct UboMesh {
		glm::mat4 projection, view, model;
		glm::vec4 lightPosition;
	};
	UboMesh * ubo = (UboMesh*)allocateUniform(sizeof(UboMesh), &uniform_buffer, &uniform_offset, &ubo_set);

	auto viewMatrix = glm::lookAt(glm::vec3(0.0, 1.0, 2.5), glm::vec3(0.0, 1.0, 0), glm::vec3(0, -1, 0));
	auto projMatrix = glm::perspective(glm::radians(70.f), vkContext.width / (float)vkContext.height, 0.1f, 1000.0f);

	glm::mat4 modelMatrix;
	modelMatrix = glm::scale(modelMatrix, glm::vec3(10, 10, 10));
	ubo->projection = projMatrix;
	ubo->view = viewMatrix;
	ubo->model = modelMatrix;
	ubo->lightPosition = glm::vec4(0.0f, 1.98f, 0.0f, 0.0f);

	VkDescriptorSet descriptor_sets[1] = { ubo_set };
	vkCmdBindDescriptorSets(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, polygonPipeline->layout, 0, 1, descriptor_sets, 1, &uniform_offset);
	Mesh *meshes;
	uint32_t nrOfMeshes;
	Meshes::getMeshes(&meshes, &nrOfMeshes);
	for (uint32_t i = 0; i < nrOfMeshes; i++) {
		Mesh &mesh = meshes[i];
		vkCmdBindVertexBuffers(vkContext.commandBuffer, 0, 1, &mesh.buffer, &mesh.verticesOffset);
		vkCmdBindIndexBuffer(vkContext.commandBuffer, mesh.buffer, mesh.indicesOffset, VK_INDEX_TYPE_UINT32);
		vkCmdBindPipeline(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, polygonPipeline->pipeline);
		vkCmdPushConstants(vkContext.commandBuffer, polygonPipeline->layout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(MeshProperties), (void*)&mesh.properties);

		vkCmdDrawIndexed(vkContext.commandBuffer, mesh.indexCount, 1, 0, 0, 0);
	}
}


