#pragma once
#include <sstream>

namespace _mgAssert {
void ReportFailure(const char* condition,
  const char* file,
  int line,
  const char* msg);

bool isDebuggerPresent();
}

#if defined(WIN32)
#define DEBUG_BREAK() __debugbreak()
#else 
#define DEBUG_BREAK()
#endif

#define mgAssert(cond) \
do \
{ \
    if (!(cond)) \
    { \
        _mgAssert::ReportFailure(#cond, __FILE__, __LINE__, 0); \
        if(_mgAssert::isDebuggerPresent()) DEBUG_BREAK(); else exit(1); \
    } \
} while(0)

#define mgAssertDesc(cond, msg) \
do \
{ \
    if (!(cond)) \
    { \
        std::ostringstream __stream; __stream << msg; \
        _mgAssert::ReportFailure(#cond, __FILE__, __LINE__, (__stream.str().c_str())); \
        if(_mgAssert::isDebuggerPresent()) DEBUG_BREAK(); else exit(1); \
    } \
} while(0)
