#pragma once

#include <string>
#include <imgui.h>

namespace mg {
struct RenderContext;
struct FrameData;

class Imgui {
public:
  void CreateContext();
  void destroy();

  void addLog(const char* fmt, ...) IM_FMTARGS(2);
  void draw(const RenderContext &renderContext, const FrameData &frameData) const;

  bool isVisible;
  uint64_t lastTimePressed;
private:
  void drawUI(const FrameData &frameData) const;
  void drawLog() const;
  void drawAllocations() const;

  std::string _fontId = "imgui font";

  struct Log {
    ImGuiTextBuffer     buffer;
    ImVector<int>       lineOffsets;        // Index to lines offset
    bool                scrollToBottom;
  } _log;
};

} // mg namespace