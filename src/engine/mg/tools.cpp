#include "tools.h"

#include <algorithm>

#include "camera.h"
#include "mgAssert.h"
#include "logger.h"
#include "window.h"

namespace mg {

static void zoom3D(const FrameData &frameData, Camera *camera) {
  const float zoomSpeed3D = 60.0f;
  auto delta = frameData.mouse.xy.y - frameData.mouse.prevXY.y;
  auto fov = camera->fov;

  camera->fov = fov - delta * zoomSpeed3D;
  camera->fov = std::min(camera->fov, 179.0f);
  camera->fov = std::max(camera->fov, 1.0f);
}

static void rotate(const FrameData &frameData, Camera *camera) {
  const auto deltax = (frameData.mouse.xy.x - frameData.mouse.prevXY.x);
  const auto deltay = (frameData.mouse.xy.y - frameData.mouse.prevXY.y);

  const auto rotationSpeed = 16.0f;
  camera->angles.x = (deltax * rotationSpeed) + camera->angles.x;
  camera->angles.y = deltay * rotationSpeed + camera->angles.y;
}

static void pan3D(const FrameData &frameData, Camera *camera) {
  const float panSpeed = camera->fov;
  const auto deltax = (frameData.mouse.xy.x - frameData.mouse.prevXY.x) * panSpeed + camera->pan.x;
  const auto deltaz = (frameData.mouse.xy.y - frameData.mouse.prevXY.y) * panSpeed + camera->pan.z;
  camera->pan.x = deltax;
  camera->pan.z = deltaz;
}

void handleTools(const FrameData &frameData, Camera *camera) {
  switch(frameData.tool) {
  case mg::Tool::PAN:
    pan3D(frameData, camera);
    break;
  case mg::Tool::ROTATE:
    rotate(frameData, camera);
    break;
  case mg::Tool::ZOOM:
    zoom3D(frameData, camera);
    break;
  case mg::Tool::NONE:
    break;
  default:
    mgAssert(false);
  }
}

} // namespace