#include "mgUtils.h"

#include <fstream>
#include <cassert>
#include <vector>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <iterator>
#include <locale>
#include <codecvt>
#include <sstream>

#include "mgAssert.h"

#include <glm/glm.hpp>

namespace mg {

glm::vec3 hexToRgb(const std::string &hex) {
  mgAssert(hex.size());
  bool hasHash = false;
  if(hex[0] == '#')
    hasHash = true;
  glm::vec3 rgb;
  std::stringstream stream(hex.c_str() + hasHash);
  uint32_t value;
  stream >> std::hex >> value;

  rgb.r = ((value >> 16) & 0xFF) / 255.0f;
  rgb.g = ((value >> 8) & 0xFF) / 255.0f;
  rgb.b = ((value) & 0xFF) / 255.0f;
  return rgb;
}

glm::vec4 hexToRgba(const std::string &hex) {
  mgAssert(hex.size());
  bool hasHash = false;
  if(hex[0] == '#')
    hasHash = true;
  glm::vec4 rgba;
  std::stringstream stream(hex.c_str() + hasHash);
  uint32_t value;
  stream >> std::hex >> value;

  rgba.r = ((value >> 24) & 0xFF) / 255.0f;
  rgba.g = ((value >> 16) & 0xFF) / 255.0f;
  rgba.b = ((value >> 8) & 0xFF) / 255.0f;
  rgba.a = ((value) & 0xFF) / 255.0f;
  return rgba; 
}

// in the format rgb(r,b,n);
glm::vec3 rgbStringToRgb(const std::string &str) {
  mgAssert(str.size());
  std::stringstream stream(str);
  stream.ignore(str.size(), '(');
  glm::vec3 res;
  char c;
  stream >> res.r >> c >> res.g >> c >> res.b;
  return res;// 255.0f;
}
std::string ltrim(const std::string &str) {
  std::string s = str;
  s.erase(begin(s), std::find_if(std::begin(s), std::end(s), [](const char c) { return !std::isspace(c); }));
  return s;
}
std::string rtrim(const std::string &str) {
  std::string s = str;
  s.erase(std::find_if(s.rbegin(), s.rend(), [](char c) { return !std::isspace(c); }).base(), end(s));
  return s;
}
std::string trim(const std::string &str) {
  std::string s = str;
  return ltrim(rtrim(s));
}

std::vector<float> uint16ToFloat32(uint16_t *data, uint32_t size) {
  std::vector<float> res(size);
  for(uint32_t i = 0; i < size; i++)
    res[i] = float(data[i]);
  return res;
}
std::vector<float> sint16ToFloat32(int16_t *data, uint32_t size) {
  std::vector<float> res(size);
  for(uint32_t i = 0; i < size; i++)
    res[i] = float(data[i]);
  return res;
}

std::vector<uint8_t> readBinaryFromDisc(const std::string &fileName) {
  std::ifstream file(fileName, std::ios::binary | std::ios::ate);

  mgAssertDesc(file.is_open(), "could not open file " + fileName);
  std::streamsize size = std::streamsize(file.tellg());
  file.seekg(0, std::ios::beg);

  std::vector<uint8_t> buffer(size);
  if(file.read((char*)buffer.data(), size))
    return buffer;
  else
    mgAssert(false);
  return std::vector<uint8_t>();
}

std::vector<char> readBinaryCharVecFromDisc(const std::string &fileName) {
  std::ifstream file(fileName, std::ios::binary | std::ios::ate);
  mgAssertDesc(file.is_open(), "could not open file " + fileName);
  const std::streamsize size = std::streamsize(file.tellg());
  file.seekg(0, std::ios::beg);

  std::vector<char> buffer(size);
  if(file.read((char*)buffer.data(), size))
    return buffer;
  else
    mgAssert(false);
  return std::vector<char>();
}

std::string readStringFromDisc(const std::string &fileName) {
#if defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK)
  auto macFileName = getMacResourcePath(fileName.c_str());
  std::ifstream file(macFileName);
#else
  std::ifstream file(fileName);
#endif
  mgAssert(file.is_open());
  std::string res;
  std::string line;
  while(std::getline(file, line)) {
    res += line + '\n';
  }
  file.close();

  return res;
}

template<typename Out>
void _split(const std::string &s, char delim, Out result) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}
    
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    _split(s, delim, std::back_inserter(elems));
    return elems;
}

std::string toLower(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  return str;
}

std::string getFontsPath() { return "../../../resources/fonts/"; }
std::string getTextureCursorsPath() { return  "../../resources/textures/cursors/"; }
std::string getTexturePath() { return "../../resources/textures/"; }
std::string getShaderPath() { return "../../../resources/shaders/build/"; }
std::string getDataPath() { return "../../../resources/data/"; }
std::string getTransferFunctionPath() { return "../../../resources/transferFunctions/"; }

} // namespace
