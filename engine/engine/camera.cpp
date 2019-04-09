#include "camera.h"
#include "vulkanContext.h"
#include "glm/gtc/quaternion.hpp"

void initCamera(Camera &camera, const glm::vec3 &position, const glm::vec3 &aim, const glm::vec3 &up) {
	camera.position = position;
	camera.originalPosition = position;
	camera.aim = aim;
	camera.originalAim = aim;
	camera.up = up;
	camera.originalUp = up;
}

void updateCamera(Camera &camera, const InputState &inputState) {
	if (isMouseButtonDown()) {
		camera.angles.y += (inputState.prevMousePos.x - inputState.mousePos.x)*0.01f;
		camera.angles.x += (inputState.mousePos.y - inputState.prevMousePos.y)*0.01f;
	}
	if (isKeyDown('w')) {
		camera.zoom -= 10.0f;
	}
	if (isKeyDown('s')) {
		camera.zoom += 10.f;
	}
	if (isKeyDown('a')) {
		camera.angles.y += 0.1f;
	}
	if (isKeyDown('d')) {
		camera.angles.y -= 0.1f;
	}
}

void setCameraTransformation(Camera &camera) {
	//y rotation
	//glm::vec3 dirY = camera.originalPosition - camera.originalAim;
	//glm::quat yRotation = glm::angleAxis(camera.angles.y, glm::vec3(0, 1, 0));
	//camera.position = camera.originalAim + dirY * yRotation;

	////x rotation
	//glm::vec3 dir = camera.position - camera.originalAim;
	//glm::vec3 normAxis = glm::normalize(glm::cross(glm::normalize(dir), camera.originalUp));
	//glm::quat rotation = glm::angleAxis(camera.angles.x, normAxis);
	//camera.position = camera.originalAim + dir * rotation;
	//camera.up = camera.originalUp * rotation;

	////zoom
	//glm::vec3 dirZ = camera.aim + camera.position;
	//glm::vec3 nDir = glm::normalize(dirZ);
	//glm::vec3 z = nDir * camera.zoom;
	//camera.position += z;


	
	//pan
	//move x
	/*glm::vec3 dirX = camera.position - camera.originalAim;
	glm::vec3 rightAxisX = glm::cross(glm::normalize(dirX), camera.up);
	glm::vec3 r = glm::normalize(rightAxisX);
	glm::vec3 rr = r * camera.pan[0] * 10.0f;
	camera.position = camera.position + rr;
	camera.aim = camera.originalAim + rr;

	//move y
	glm::vec3 nup = glm::normalize(camera.up);
	glm::vec3 sup = nup * camera.pan[2] * 10.0f;
	camera.position = camera.position + sup;
	camera.aim = camera.aim + sup;
	*/
	//zoom
	}
//
//void updateMouse(Camera &camera, float deltaTime) {
//	double xpos, ypos;
//	getMousePosition(&xpos, &ypos);
//
//	camera.horizontalAngle += camera.mouseSpeed * deltaTime * (vkContext.width / 2.0f - xpos);
//	camera.verticalAngle += camera.mouseSpeed * deltaTime * (vkContext.height / 2.0f - ypos);
//
//	glm::vec3 direction(
//		cos(camera.verticalAngle) * sin(camera.horizontalAngle),
//		sin(camera.verticalAngle),
//		cos(camera.verticalAngle) * cos(camera.horizontalAngle)
//	);
//
//	camera.lookat = camera.position + direction;
//	// Right vector
//	glm::vec3 right = glm::vec3(
//		sin(camera.horizontalAngle - 3.14f / 2.0f),
//		0,
//		cos(camera.horizontalAngle - 3.14f / 2.0f)
//	);
//	camera.up = glm::cross(right, direction);
//
//	if (isKeyDown('w')) {
//		camera.position += direction * deltaTime * camera.speed;
//	}
//	if (isKeyDown('s')) {
//		camera.position -= direction * deltaTime * camera.speed;
//	}
//	if (isKeyDown('d')) {
//		camera.position += right * deltaTime * camera.speed;
//	}
//	if (isKeyDown('a')) {
//		camera.position -= right * deltaTime * camera.speed;
//	}
//}
