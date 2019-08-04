#include "logger.h"
#include <cstdlib>
#include <stdio.h>

namespace mg {

  FILE *pFile = nullptr;

void logInfo(const char *str, const char *file, const int line) {
  pFile = fopen("output.txt", "a");
  printf("%s: %s(%d)\n", str, file, line);
  fprintf(pFile, "%s: %s(%d)\n", str, file, line);
  fclose(pFile);
}

void logAssert(const char *str) { printf("%s", str); }

} // namespace mg