#pragma once

#include <sstream>
#include <chrono>
#include <algorithm>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>

#define mg_typeof(value) std::remove_reference<decltype(value)>::type
#define mg_typeofElement(value) typename std::remove_reference<decltype(value)>::type::value_type

namespace mg {
struct Camera;

template<class T>
uint32_t sizeofContainerInBytes(const T &container) {
  return uint32_t(sizeof(mg_typeofElement(container)) * container.size());
}

template <typename T, uint32_t N>
constexpr uint32_t countof(T(&)[N]) {
  return N;
}

template <typename T, uint32_t N>
constexpr uint32_t sizeofArrayInBytes(T(&)[N]) {
  return N * sizeof(T);
}

struct nonCopyable {
  nonCopyable() = default;
  nonCopyable(const nonCopyable &) = delete;
  nonCopyable& operator=(const nonCopyable &x) = delete;
  nonCopyable(nonCopyable &&) noexcept = default;
  nonCopyable& operator=(nonCopyable &&x) = default;
};

template<class T>
inline T clamp(T val, T minVal, T maxVal) {
  return std::min(std::max(val, minVal), maxVal);
}

inline float reinterval(float inVal, float oldMin, float oldMax, float newMin, float newMax) {
  if((oldMax - oldMin) == 0) return newMax;
  return (((inVal - oldMin) * (newMax - newMin)) / (oldMax - oldMin)) + newMin;
}
inline float reintervalClamped(float inVal, float oldMin, float oldMax, float newMin, float newMax) {
  float value = reinterval(inVal, oldMin, oldMax, newMin, newMax);
  return clamp(value, newMin, newMax);
}

inline float mix(float x, float y, float a) {
  return x * (1 - a) + y * a;
}

glm::vec3 hexToRgb(const std::string &hex);
glm::vec4 hexToRgba(const std::string &hex);

glm::vec3 rgbStringToRgb(const std::string &str);

namespace timer {
typedef std::chrono::time_point<std::chrono::high_resolution_clock> Time;

inline Time now() {
  return std::chrono::high_resolution_clock::now();
}
inline uint64_t durationInMs(const Time &start, const Time &end) {
  return uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
}
inline uint64_t durationInUs(const Time &start, const Time &end) {
  return uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
}
}

std::string rtrim(const std::string &s);
std::string ltrim(const std::string &s);
std::string trim(const std::string &s);

std::vector<float> uint16ToFloat32(uint16_t *data, uint32_t size);
std::vector<float> sint16ToFloat32(int16_t *data, uint32_t size);

std::vector<uint8_t> readBinaryFromDisc(const std::string &name);
std::vector<char> readBinaryCharVecFromDisc(const std::string &name);
std::string readStringFromDisc(const std::string &fileName);

template <typename Iter>
int32_t indexOf(Iter first, Iter last, const typename std::iterator_traits<Iter>::value_type& x) {
  int32_t i = 0;
  while(first != last && *first != x)
    ++first, ++i;
  if(first == last) return -1;
  return i;
}

template<typename T, typename Func>
std::vector<T> filter(const std::vector<T> &vec, Func func) {
  std::vector<T> res;
  std::copy_if(std::begin(vec), std::end(vec), std::back_inserter(res), func);
  return res;
}

template<typename T>
T unique(const T &inData) {
  const std::unordered_set<typename T::value_type> uniqueSet(std::begin(inData), std::end(inData));
  T res;
  res.reserve(uniqueSet.size());
  res.assign(std::begin(uniqueSet), std::end(uniqueSet));
  return res;
}

template <typename T>
bool isSorted(const T& first, const T& last) {
  for(T it = first, it_end = std::prev(last); it != it_end; ++it) {
    if(*next(it) < *it)
      return false;
  }
  return true;
}

template <typename C>
bool isSorted(const C& container) {
  return isSorted(std::begin(container), std::end(container));
}

std::vector<std::string> split(const std::string &s, char delim);
std::string toLower(std::string str);

inline bool sameSign(float a, float b) {
  return a * b >= 0.0f;
};

inline uint32_t alignUpPowerOfTwo(uint32_t n, uint32_t alignment) {
  return (n + (alignment - 1)) & ~(alignment - 1);
}
inline uint64_t alignUpPowerOfTwo(uint64_t n, uint64_t alignment) {
  return (n + (alignment - 1)) & ~(alignment - 1);
}

std::string getFontsPath();
std::string getTextureCursorsPath();
std::string getTexturePath();
std::string getShaderPath();
std::string getDataPath();
std::string getTransferFunctionPath();

class MakeString {
public:
  template <typename T>
  MakeString& operator<<(const T& val) {
    _stream << val;
    return *this;
  }
  operator std::string() const {
    return _stream.str();
  }
private:
  std::ostringstream _stream;
};

} //namespace
