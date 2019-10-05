#include "camera.h"
#include "logger.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace mg {

Camera create3DCamera(const glm::vec3 &position, const glm::vec3 &aim, const glm::vec3 &up) {
  Camera camera = {};
  camera.startPosition = camera.position = position;
  camera.startAim = camera.aim = aim;
  camera.startUp = camera.up = up;
  camera.startFov = camera.fov = 45.0f;

  return camera;
}

void setCameraTransformation(Camera *camera) {
  camera->position = camera->startPosition + camera->pan;
  camera->aim = camera->startAim + camera->pan;

  // rotation
  // x axis
  const auto xdir = camera->startPosition - camera->startAim;
  
  const auto xrotation = glm::angleAxis(camera->angles.x, glm::vec3{ 0, 1, 0 });
  const auto xnewDir = xrotation * xdir;
  camera->position = camera->startAim + xnewDir;
  camera->up = { 0,1,0 };
  
  // y axis
  const auto ydir = camera->position - camera->startAim;
  auto rightAxis = glm::cross(glm::normalize(ydir), camera->up);
  const auto normAxis = glm::normalize(rightAxis);
  const auto  yrotation = glm::angleAxis(-camera->angles.y, normAxis);
  const auto ynewDir = yrotation * ydir;
  camera->up = yrotation * camera->up;
  camera->position = camera->startAim + ynewDir;

  // panning
  // move x
  const auto dir = camera->position - camera->startAim;
  rightAxis = glm::cross(glm::normalize(dir), camera->up);
  const auto rightAxisNormalize = glm::normalize(rightAxis);
  const auto rightAxisNormalizeScaled = rightAxisNormalize * -camera->pan.x * 10.0f;
  camera->position = camera->position + rightAxisNormalizeScaled;
  camera->aim = camera->startAim + rightAxisNormalizeScaled;

  // move y
  const auto nup = glm::normalize(camera->up);
  const auto sup = nup * -camera->pan.z * 10.0f;
  camera->position = camera->position + sup;
  camera->aim = camera->aim + sup;
}

} // namespace