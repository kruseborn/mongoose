#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <vector>
#include "mg/textureContainer.h"
#include "mgUtils.h"
#include <freetype/ftbitmap.h>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace mg {
enum class FONT_TYPE { MICRO_SS_9, MICRO_SS_10, MICRO_SS_11, MICRO_SS_14, SIZE };
struct Texts;

void validateTexts(const mg::Texts &texts);


class Fonts : mg::nonCopyable {
public:
  struct Character {
    uint32_t textureOffset; // Offset in texture map
    glm::uvec2 size;        // Size of glyph
    glm::ivec2 bearing;     // Offset from baseline to left/top of glyph
    int32_t advance;        // GLuint Advance;    // Offset to advance to next glyph
  };

  void init();    // Needed because vulkan has not been initialized before scene is created
  void destroy(); // Needed because vulkan has been destoyed before scene is destroyed

  void validateFontCharacters(FONT_TYPE fontType, const std::string &text);
  float getFontLineHeight(FONT_TYPE fontType) const;
  glm::vec4 getFontBbox(FONT_TYPE fontType) const;
  float getFontAscender(FONT_TYPE fontType) const;
  float getFontDescender(FONT_TYPE fontType) const;

  const Character &getFontCharacter(FONT_TYPE fontType, char characterCode) const;
  glm::uvec2 getFontMapSize(FONT_TYPE fontType) const;
  mg::Texture getFontGlyphMap(FONT_TYPE fontType) const;

  float calcTextWidth(const std::string &text, FONT_TYPE fontType) const;

private:
  struct Font {
    std::string name;
    uint32_t fontSize;
    FT_Face ftFace;
    glm::uvec2 fontMapSize = {};
    float lineHeight;
    float descender;
    float ascender;

    mg::Texture fontGlyphMap;
    std::vector<uint8_t> bitmapCharData;
    std::unordered_map<int32_t, Character> characters;
  };

  Font createFont(const std::string &path, const std::string &name, uint32_t fontSize);
  Font *accessFont(FONT_TYPE fontType);

  void setupFontCharacterMap(Font *font);
  int loadFontCharacter(Font *font, int32_t characterCode, uint32_t offset, bool allocateNewTexture);
  const Character &getFontCharacter(const Font &font, char characterCode) const;
  const Font &getFont(FONT_TYPE fontType) const;

private:
  std::array<Font, uint32_t(FONT_TYPE::SIZE)> _fontTypeToFont;
  FT_Library _ftLibrary;
};

} // namespace mg
