#include "fonts.h"

#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <fstream>

#include "mgAssert.h"

#include "mg/textureContainer.h"
#include "vulkan/pipelineContainer.h"
#include "mg/mgSystem.h"
#include "mg/texts.h"

namespace mg {
static const uint32_t horizontalDpi = 96;
static const uint32_t verticalDpi = 96;

void validateTexts(const mg::Texts &texts) {
  const auto &fontsToTexts = texts.fontsToTexts;
  for (uint32_t i = 0; i < uint32_t(mg::FONT_TYPE::SIZE); ++i) {
    for (auto &text : fontsToTexts[i]) {
      mg::mgSystem.fonts.validateFontCharacters((mg::FONT_TYPE)i, text.text);
    }
  }
}

void Fonts::init() {
  const auto error = FT_Init_FreeType(&_ftLibrary);
  mgAssertDesc(error == FT_Err_Ok, "FreeType not initialized properly, error code: " << error);

  //_fontTypeToFont[(uint32_t)mg::FONT_TYPE::MICRO_SS_9] = createFont(getFontsPath(), "micross.ttf", 9);
  //_fontTypeToFont[(uint32_t)mg::FONT_TYPE::MICRO_SS_10] = createFont(getFontsPath(), "micross.ttf", 10);
  _fontTypeToFont[(uint32_t)mg::FONT_TYPE::MICRO_SS_11] = createFont(getFontsPath(), "micross.ttf", 11);
  //_fontTypeToFont[(uint32_t)mg::FONT_TYPE::MICRO_SS_14] = createFont(getFontsPath(), "micross.ttf", 14);
}

void Fonts::destroy() {
  for(const auto& font : _fontTypeToFont) {
    mg::mgSystem.textureContainer.removeTexture(font.fontGlyphMap);
  }
  for(uint32_t i = 0; i < uint32_t(_fontTypeToFont.size()); ++i) {
    FT_Done_Face(_fontTypeToFont[i].ftFace);
  }
  FT_Done_FreeType(_ftLibrary);
}

void Fonts::validateFontCharacters(FONT_TYPE fontType, const std::string& text) {
  Font* font = accessFont(fontType);

  for(const auto& character : text) {
    const auto characterIt = font->characters.find(character);
    if(characterIt == std::end(font->characters))
      loadFontCharacter(font, character, font->fontMapSize.x, true);
  }
}

const Fonts::Font& Fonts::getFont(FONT_TYPE fontType) const {
  return _fontTypeToFont[(uint32_t)fontType];
}

const Fonts::Character& Fonts::getFontCharacter(FONT_TYPE fontType, char characterCode) const {
  return getFontCharacter(getFont(fontType), characterCode);
}

glm::uvec2 Fonts::getFontMapSize(FONT_TYPE fontType) const {
  return getFont(fontType).fontMapSize;
}

TextureId Fonts::getFontGlyphMap(FONT_TYPE fontType) const {
  return getFont(fontType).fontGlyphMap;
}

const Fonts::Character& Fonts::getFontCharacter(const Font& font, char characterCode) const {
  const auto it = font.characters.find(characterCode);
  if(it != std::end(font.characters)) {
    return it->second;
  }
  const auto whiteSpace = 32;
  return font.characters.at(whiteSpace);
}

float Fonts::getFontLineHeight(FONT_TYPE fontType) const {
  const auto &font = getFont(fontType);
  return font.lineHeight;
}

glm::vec4 Fonts::getFontBbox(FONT_TYPE fontType) const {
  const auto &font = getFont(fontType);
  const auto bBox = font.ftFace->bbox;
  const glm::vec4 resBBox = {float(bBox.xMin), float(bBox.xMax), float(bBox.yMin), float(bBox.yMax)};
  return resBBox;
}

float Fonts::getFontDescender(FONT_TYPE fontType) const {
  const auto &font = getFont(fontType);
  return font.descender;
}

float Fonts::getFontAscender(FONT_TYPE fontType) const {
  const auto &font = getFont(fontType);
  return font.ascender;
}

float Fonts::calcTextWidth(const std::string& text, FONT_TYPE fontType) const {
  float textWidth = 0.0f;
  const auto &font = getFont(fontType);
  for(uint32_t i = 0; i < uint32_t(text.size()); ++i) {  
    const auto &character = getFontCharacter(font, text[i]);
    textWidth += float(character.advance);
  }
  return textWidth;
}
Fonts::Font Fonts::createFont(const std::string& path, const std::string &name, uint32_t fontSize) {
  Font newFont;
  newFont.name = name;
  newFont.fontSize = fontSize;

  // Freetype settings
  {
    const auto fullPath = path + name;
    FT_Error error = FT_New_Face(_ftLibrary, fullPath.c_str(), 0, &newFont.ftFace);
    if(error == FT_Err_Unknown_File_Format)
      mgAssertDesc(false, "Font format is not supported");
    else if(error)
      mgAssertDesc(false, "Font file could not be opened");

    error = FT_Select_Charmap(newFont.ftFace, ft_encoding_unicode);
    mgAssertDesc(error == FT_Err_Ok, "FreeType charmap not set properly, error code: " << error);

    // Char height is specified in 1/64th of points.
    error = FT_Set_Char_Size(newFont.ftFace, 0, fontSize * 64, horizontalDpi, verticalDpi);
    mgAssertDesc(error == FT_Err_Ok, "FreeType face size not set properly, error code: " << error << ". Size = " << fontSize);

    newFont.lineHeight = (newFont.ftFace->bbox.yMax - newFont.ftFace->bbox.yMin) *
      (float(newFont.ftFace->size->metrics.y_ppem) / float(newFont.ftFace->units_per_EM));

    newFont.descender = float(newFont.ftFace->descender) * (float(newFont.ftFace->size->metrics.y_ppem) / float(newFont.ftFace->units_per_EM));
    newFont.ascender = float(newFont.ftFace->ascender) * (float(newFont.ftFace->size->metrics.y_ppem) / float(newFont.ftFace->units_per_EM));

    setupFontCharacterMap(&newFont);
  }

  return newFont;
}

Fonts::Font* Fonts::accessFont(FONT_TYPE fontType) {
  return &_fontTypeToFont[(uint32_t)fontType];
}

void Fonts::setupFontCharacterMap(Font* font) {
  const int32_t startingChar = 32;
  const int32_t endingChar = 256;
  for(int32_t characterCode = startingChar; characterCode <= endingChar; ++characterCode)
  {
    // Load character glyph
    auto characterIdx = FT_Get_Char_Index(font->ftFace, characterCode);
    if(FT_Load_Glyph(font->ftFace, characterIdx, FT_LOAD_RENDER | FT_LOAD_TARGET_MONO | FT_LOAD_MONOCHROME)) {
      mgAssertDesc(false, "Failed to load glyph");
    }

    font->fontMapSize.x += mg::alignUpPowerOfTwo(font->ftFace->glyph->bitmap.width, 4);
    font->fontMapSize.y = std::max(uint32_t(font->fontMapSize.y), font->ftFace->glyph->bitmap.rows);
  }

  // Generate empty texture to fill with subTexture later
  font->bitmapCharData.resize(font->fontMapSize.x * font->fontMapSize.y);

  uint32_t combinedWidth = 0;
  for(int32_t c = startingChar; c <= endingChar; ++c) {
    combinedWidth += loadFontCharacter(font, c, combinedWidth, false);
  }

  mg::CreateTextureInfo createTextureInfo = {};
  createTextureInfo.id = mg::MakeString() << font->name << font->fontSize << "_Dpi" << horizontalDpi << "x" << verticalDpi;
  createTextureInfo.type = mg::TEXTURE_TYPE::TEXTURE_2D;
  createTextureInfo.size = { uint32_t(font->fontMapSize.x), uint32_t(font->fontMapSize.y), 1 };
  createTextureInfo.format = VK_FORMAT_R8_UNORM;
  createTextureInfo.data = font->bitmapCharData.data();
  createTextureInfo.sizeInBytes = mg::sizeofContainerInBytes(font->bitmapCharData);

  font->fontGlyphMap = mg::mgSystem.textureContainer.createTexture(createTextureInfo);
}

int Fonts::loadFontCharacter(Font* font, int32_t characterCode, uint32_t offset, bool allocateNewTexture) {
  const auto characterIdx = FT_Get_Char_Index(font->ftFace, characterCode);

  // Load character glyph
  if (FT_Load_Glyph(font->ftFace, characterIdx, FT_LOAD_RENDER | FT_LOAD_TARGET_MONO | FT_LOAD_MONOCHROME)) {
    mgAssertDesc(false, "Failed to load glyph");
  }

  // Set up a character representation of useful properties
  Character character = {};
  character.textureOffset = offset;
  character.size = { font->ftFace->glyph->bitmap.width, font->ftFace->glyph->bitmap.rows };
  character.bearing = glm::ivec2{ font->ftFace->glyph->bitmap_left, font->ftFace->glyph->bitmap_top };
  // (note that advance is number of 1/64 pixels)
  // Bitshift by 6 to get value in pixels (2^6 = 64)
  character.advance = font->ftFace->glyph->advance.x >> 6;

  if(allocateNewTexture) {
    const glm::ivec2 newSize = glm::ivec2{font->fontMapSize.x + font->ftFace->glyph->bitmap.width, std::max(uint32_t(font->fontMapSize.y), font->ftFace->glyph->bitmap.rows)};
    std::vector<uint8_t> tmp(newSize.x * newSize.y);
    for(uint32_t y = 0; y < font->fontMapSize.y; y++) {
      for(uint32_t x = 0; x < font->fontMapSize.x; x++) {
        tmp[y * newSize.x + x] = font->bitmapCharData[y * font->fontMapSize.x + x];
      }
    }

    font->fontMapSize = newSize;
    font->bitmapCharData = tmp;
  }

  // Bitmaps from FreeType are 1 bit values. Use Bitmap_Convert to get 8 bit colors.
  FT_Bitmap dstBitmap8bit;
  FT_Bitmap_Init(&dstBitmap8bit);
  FT_Bitmap_Convert(_ftLibrary, &font->ftFace->glyph->bitmap, &dstBitmap8bit, 1);

  // Bitmap values are 0 or 1, we want 0 or 255
  const auto pitch = dstBitmap8bit.pitch;
  for(auto y = 0u; y < dstBitmap8bit.rows; ++y) {
    for(auto x = 0u; x < dstBitmap8bit.width; ++x) {
      dstBitmap8bit.buffer[y*pitch + x] = dstBitmap8bit.buffer[y*pitch + x] * 255u;
    }
  }

  for(auto y = 0u; y < dstBitmap8bit.rows; ++y) {
    for(auto x = 0u; x < dstBitmap8bit.width; ++x) {
      font->bitmapCharData[y*font->fontMapSize.x + x + offset] = dstBitmap8bit.buffer[y*pitch + x];
    }
  }

  // Add it to the texture map
  if(allocateNewTexture) {
    mg::mgSystem.textureContainer.removeTexture(font->fontGlyphMap);
    mg::CreateTextureInfo createTextureInfo = {};
    createTextureInfo.id = mg::MakeString() << font->name << font->fontSize << "_Dpi" << horizontalDpi << "x" << verticalDpi;
    createTextureInfo.type = mg::TEXTURE_TYPE::TEXTURE_2D;
    createTextureInfo.size = { font->fontMapSize.x, font->fontMapSize.y, 1 };
    createTextureInfo.format = VK_FORMAT_R8_UNORM;
    createTextureInfo.data = font->bitmapCharData.data();
    createTextureInfo.sizeInBytes = mg::sizeofContainerInBytes(font->bitmapCharData);

    font->fontGlyphMap = mg::mgSystem.textureContainer.createTexture(createTextureInfo);
  }

  FT_Bitmap_Done(_ftLibrary, &dstBitmap8bit);

  // Save it in a map for later accessing
  font->characters.emplace(std::piecewise_construct,
    std::forward_as_tuple(characterCode),
    std::forward_as_tuple(std::move(character))
  );
  character = {};

  // Return offset for next run
  return font->ftFace->glyph->bitmap.width;
}

} // namespace
