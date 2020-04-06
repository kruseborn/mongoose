#pragma once

#include "tools.h"
#include <cstdint>
#include <glm/glm.hpp>

namespace mg {

struct FrameData {
  uint32_t width, height;

  struct {
    glm::vec2 xy, prevXY;
    bool left, middle, right;
  } mouse;
  struct {
    bool r, n, m, w, a, s, d;
    bool left, right, up, space;
  } keys;
  mg::Tool tool;
  float dt;
  uint32_t fps;
};

void initWindow(uint32_t width, uint32_t height);
void destroyWindow();

bool startFrame();
void endFrame();
FrameData getFrameData();
float getTime();

} // namespace mg