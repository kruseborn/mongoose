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
  // Returns true if the current process is being debugged (either
  // running under the debugger or has a debugger attached post facto).
  int                 junk;
  int                 mib[4];
  struct kinfo_proc   info;
  size_t              size;

  // Initialize the flags so that, if sysctl fails for some bizarre
  // reason, we get a predictable result.
  info.kp_proc.p_flag = 0;
  // Initialize mib, which tells sysctl the info we want, in this case
  // we're looking for information about a specific process ID.

  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PID;
  mib[3] = getpid();
  // Call sysctl.

  size = sizeof(info);
  junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
  assert(junk == 0);

  // We're being debugged if the P_TRACED flag is set.
  return ((info.kp_proc.p_flag & P_TRACED) != 0);
}
#endif

bool isDebuggerPresent() {
#if defined(WIN32)
  return IsDebuggerPresent();
#else
  return isDebuggerPresentLinux;
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
