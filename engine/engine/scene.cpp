#include "scene.h"
#include "shaders.h"
#include "vulkanContext.h"
#include "vkUtils.h"
#include "pipelines.h"
#include "meshes.h"
#include "rendering.h"
#include "vkDefs.h"
#include "deferred.h"
#include "textures.h"
#include <iostream>



void initScene(Scene &scene) {
	Pipelines::init();

	initCamera(scene.camera, glm::vec3(0.5f, 200, 470), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

  	glm::mat4 viewMatrix = glm::lookAt(scene.camera.position,
                                           scene.camera.aim, scene.camera.up);
        glm::mat4 projMatrix = glm::perspective(
            glm::radians(70.f), vkContext.width / (float)vkContext.height, 0.1f,
            1000.0f);
        Meshes::init();
	scene.camera.angles.x = 0.03f;
	scene.camera.angles.y = 5.59f;
	//initCamera(scene.camera, glm::vec3(0.5f, 2, 2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

}
void drawScene(Scene &scene) {
	//scene.camera.angles.y += deltaTime * 0.5;
	//updateCamera(scene.camera, scene.inputState);
	//setCameraTransformation(scene.camera);
	
	

	glm::mat4 viewMatrix = glm::lookAt(
		scene.camera.position, 
		scene.camera.aim,
		scene.camera.up
	);

	//auto viewMatrix = glm::lookAt(glm::vec3(100, 350,250), glm::vec3(0.0, 1.0, 0), glm::vec3(0, 1, 0));
	glm::mat4 projMatrix = glm::perspective(glm::radians(70.f), vkContext.width / (float)vkContext.height, 0.1f, 1000.0f);


	preRendering();
	//beginRendering();
 	beginDeferredRendering();
	{
		renderMRT(projMatrix, viewMatrix);
		vkCmdNextSubpass(vkContext.commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
		renderSSAO(projMatrix, viewMatrix);
		vkCmdNextSubpass(vkContext.commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
		renderBlurSSAO(projMatrix, viewMatrix);
		vkCmdNextSubpass(vkContext.commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
		renderFinalDeferred(projMatrix, viewMatrix);
	
		static Pipeline *textPipeline = Pipelines::findPipeline("text");
		static Pipeline *depthPipeline = Pipelines::findPipeline("depth");


		static VkDescriptorSet nomalTexture = Textures::findTexture("normal")->descriptorSet;
		static VkDescriptorSet depthTexture = Textures::findTexture("depth")->descriptorSet;
		static VkDescriptorSet albedoTexture = Textures::findTexture("albedo")->descriptorSet;
		static VkDescriptorSet ssaoBlur = Textures::findTexture("ssaoblur")->descriptorSet;


		renderTextureBox(-0.98f + 0.32f,        -0.9f, 0.3f, 0.3f, nomalTexture, textPipeline);
		renderTextureBox(-0.98f + 0.32f*2.0f, -0.9f, 0.3f, 0.3f, depthTexture, depthPipeline);
		renderTextureBox(-0.98f + 0.32f*3.0f, -0.9f, 0.3f, 0.3f, albedoTexture, textPipeline);
		renderTextureBox(-0.98f + 0.32f*4.0f, -0.9f, 0.3f, 0.3f, ssaoBlur, textPipeline);

		renderFps();
	}
	endDefferedRendering();

	//beginRendering();
	//render();
	//endRendering();

	postRendering();

}
