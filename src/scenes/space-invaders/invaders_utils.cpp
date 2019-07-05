#include <cinttypes>
#include <cassert>

#ifdef _WIN32
#include <intrin.h>
uint32_t ctz(uint32_t value) {
  assert(value);
  unsigned long trailingZero = 0;
  const auto res = _BitScanForward(&trailingZero, value);
  assert(res);
  return (trailingZero);
}
#else
uint32_t ctz(uint32_t value) { return __builtin_ctz(value); }
#endif
