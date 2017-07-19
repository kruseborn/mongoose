#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#pragma warning( disable : 4996)


#include "glm/glm.hpp"

#include "vulkanContext.h"
#include "scene.h"
#include "shaders.h"
#include "deferred.h"
#include "vkDefs.h"
#include "GLFW/glfw3.h"

int main() {

	initVulkan(1500, 1024);
	Scene scene;
	initScene(scene);

	while (!windowShouldClose()) {
		scene.inputState.prevMousePos = scene.inputState.mousePos;
		getMousePosition(&scene.inputState.mousePos.x, &scene.inputState.mousePos.y);
		realtime = float(glfwGetTime());
		static float prevTime = realtime;
		deltaTime = realtime - prevTime;
		prevTime = realtime;
		framecount++;

		drawScene(scene);
		windowPollEvents();
	}
	destroyVulkanContext();
	return 0;
}
