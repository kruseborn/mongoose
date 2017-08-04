#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

struct Camera {
	float fov = 0.785398163f; //rad
	float zoom = 0;
	glm::vec3 angles = glm::vec3(0, 0, 0);
	glm::vec3 pan = glm::vec3(0, 0, 0);

	glm::vec3 originalUp;
	glm::vec3 up;
	glm::vec3 position;
	glm::vec3 originalPosition;
	glm::vec3 originalAim;
	glm::vec3 aim;
};

struct InputState {
	glm::vec2 mousePos;
	glm::vec2 prevMousePos;
};

void updateCamera(Camera &camera, const InputState &inputState);
void initCamera(Camera &camera, const glm::vec3 &position, const glm::vec3 &aim, const glm::vec3 &up);
void setCameraTransformation(Camera &camera);
