#include "texts.h"

#include "mgAssert.h"

#include "vulkan/vkContext.h"
#include "mg/mgSystem.h"

namespace mg {

static void setPositionFromViewAlignment(Texts::Offsets *offsets, mg::Text *text, float fontLineHeight, const Fonts &fonts) {
  const glm::vec4 viewport{0, 0, vkContext.screen.width, vkContext.screen.height};

  switch (text->viewAlignment) {
  case VIEW_ALIGNMENT::BOTTOM_LEFT: {
    offsets->bottomLeft.y -= fonts.getFontDescender(text->fontType);
    text->position = offsets->bottomLeft;
    offsets->bottomLeft.y += fonts.getFontAscender(text->fontType) + 2.0f;
  } break;
  case VIEW_ALIGNMENT::BOTTOM_RIGHT: {
    glm::vec2 newPosition{viewport.z, 0.0f};
    offsets->bottomRight.y -= fonts.getFontDescender(text->fontType);
    newPosition += offsets->bottomRight;
    text->position = newPosition;

    offsets->bottomRight.y += fonts.getFontAscender(text->fontType) + 2.0f;
  } break;
  case VIEW_ALIGNMENT::TOP_LEFT: {
    glm::vec2 newPosition{0.0f, viewport.w};
    offsets->topLeft.y -= fontLineHeight;
    offsets->topLeft.y -= fonts.getFontDescender(text->fontType);

    newPosition += offsets->topLeft;
    text->position = newPosition;

    offsets->topLeft.y += fonts.getFontDescender(text->fontType);
  } break;
  case VIEW_ALIGNMENT::TOP_RIGHT: {
    glm::vec2 newPosition{viewport.z, viewport.w};
    offsets->topRight.y -= fontLineHeight;
    offsets->topRight.y -= fonts.getFontDescender(text->fontType);

    newPosition += offsets->topRight;
    text->position = newPosition;

    offsets->topRight.y += fonts.getFontDescender(text->fontType);
  } break;
  case VIEW_ALIGNMENT::NONE:
    mgAssertDesc(false, "Invalid view alignment");
  default:
    break;
  }
}

static void setPositionFromTextAlignment(mg::Text *text, const Fonts &fonts) {
  switch (text->textAlignment) {
  case TEXT_ALIGNMENT::LEFT:
  case TEXT_ALIGNMENT::SIZE:
    break;
  case TEXT_ALIGNMENT::RIGHT:
    text->position.x -= fonts.calcTextWidth(text->text, text->fontType);
    break;
  default:
    break;
  }
}

void pushText(Texts *texts, mg::Text text) {
  if (text.viewAlignment != VIEW_ALIGNMENT::NONE) {
    const auto fontLineHeight = mg::mgSystem.fonts.getFontLineHeight(text.fontType);
    setPositionFromViewAlignment(&texts->offsets, &text, fontLineHeight, mg::mgSystem.fonts);
  }
  setPositionFromTextAlignment(&text, mg::mgSystem.fonts);

  const auto fontType = (uint32_t)text.fontType;
  mgAssert(texts->textsPerFont[fontType] + 1 < Texts::maxTextPerFont);

  _Text _text = {text.text, text.position, text.color};
  texts->fontsToTexts[(uint32_t)text.fontType][texts->textsPerFont[fontType]++] = _text;
}

} // namespace mg