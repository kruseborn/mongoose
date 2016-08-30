#pragma once
#include "vulkanContext.h"
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include "camera.h"


struct Scene {
	Camera camera;
	InputState inputState;
};

void initScene(Scene &scene);
void drawScene(Scene &scene);