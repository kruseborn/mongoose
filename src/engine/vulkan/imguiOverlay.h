#pragma once

#include <string>
#include <imgui.h>
#include "mg/textureContainer.h"

namespace mg {
struct RenderContext;
struct FrameData;

class Imgui {
public:
  void CreateContext();
  void destroy();

  typedef void DrawFunc(const FrameData &frameData, void *data);
  void draw(const RenderContext &renderContext, const FrameData &frameData, DrawFunc *drawFunc, void *data) const;

  bool isVisible;
  uint64_t lastTimePressed;

  mg::TextureId _fontId;


};

} // mg namespace