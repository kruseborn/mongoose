#include "logger.h"
#include <cstdlib>

namespace mg {

void logInfo(const char *str, const char* file, const int line) {
  printf("%s: %s(%d)\n", str, file, line);
}

void logAssert(const char *str) {
  printf("%s", str);
}

} // namespace