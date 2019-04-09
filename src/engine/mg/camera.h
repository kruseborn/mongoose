#pragma once

#include <glm/glm.hpp>

namespace mg {

struct Camera {
  glm::vec3 angles, startAngles;
  glm::vec3 pan, startPan;
  glm::vec3 position, startPosition;
  glm::vec3 aim, startAim, up, startUp;
  float zoom, startZoom;
  float fov, startFov;
};

Camera create3DCamera(const glm::vec3 &position, glm::vec3 &aim, const glm::vec3 &up);
void setCameraTransformation(Camera *camera);

}
