#pragma once
#include <stdint.h>
#include <glm/glm.hpp>

namespace mg {

struct Camera;
struct FrameData;
enum class Tool {
  ROTATE, PAN, ZOOM, NONE
};

void handleTools(const FrameData &frameData, Camera *camera);

} // namespace