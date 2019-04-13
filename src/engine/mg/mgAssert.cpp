#include "mgAssert.h"

#if WIN32 
#include <Windows.h>
#endif

#if __linux__ 
#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sysctl.h>
#endif

#include <iostream>
#include <cstdio>
#include <cstdarg>

#include "logger.h"

namespace _mgAssert {

#if defined __linux__
// https://developer.apple.com/library/content/qa/qa1361/_index.html
static bool isDebuggerPresentLinux() {
  return false;
}
#endif

bool isDebuggerPresent() {
#if defined(WIN32)
  return IsDebuggerPresent();
#else
  return isDebuggerPresentLinux();
#endif
}

void ReportFailure(const char* condition,
  const char* file,
  const int line,
  const char* msg)
{
  std::stringstream stream;
  stream << "Assert Failure: " << file << "(" << line << ") ";
  stream << ((condition != nullptr) ? condition : "") << "";
  stream << ((msg != nullptr) ? ", " + std::string(msg) : "");
  LOGA(stream.str());
  std::cerr << msg << std::endl;
}

} // namespace
