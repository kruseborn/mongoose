#pragma once

#include <array>
#include <string>
#include <vector>

#include "fonts.h"

namespace mg {

enum class VIEW_ALIGNMENT { TOP_LEFT, BOTTOM_LEFT, TOP_RIGHT, BOTTOM_RIGHT, NONE };
enum class TEXT_ALIGNMENT { LEFT, RIGHT, SIZE };

struct Text {
  std::string text;
  glm::vec2 position;
  glm::vec4 color = {1, 1, 1, 1};
  mg::FONT_TYPE fontType = FONT_TYPE::MICRO_SS_14;
  TEXT_ALIGNMENT textAlignment = TEXT_ALIGNMENT::LEFT;
  VIEW_ALIGNMENT viewAlignment = VIEW_ALIGNMENT::TOP_LEFT;
};
struct _Text {
  std::string text;
  glm::vec2 position;
  glm::vec4 color;
};

struct Texts {

  enum { maxTextPerFont = 10 };
  _Text fontsToTexts[uint32_t(mg::FONT_TYPE::SIZE)][maxTextPerFont];
  uint32_t textsPerFont[uint32_t(mg::FONT_TYPE::SIZE)];

  struct Offsets {
    glm::vec2 startBottomLeft = {7.0f, 10.0f};
    glm::vec2 startBottomRight = {-20.0f, 10.0f};
    glm::vec2 startTopLeft = {7.0f, -5.0f};
    glm::vec2 startTopRight = {-20.0f, -5.0f};

    glm::vec2 bottomLeft = startBottomLeft;
    glm::vec2 bottomRight = startBottomRight;
    glm::vec2 topLeft = startTopLeft;
    glm::vec2 topRight = startTopRight;
  } offsets;
};

void pushText(Texts *texts, mg::Text text);

} // namespace mg