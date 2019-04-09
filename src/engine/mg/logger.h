#pragma once

#include <sstream>

#if defined(WIN32)
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif
#define LOG(msg) do \
{ std::ostringstream __str; __str << msg; mg::logInfo(__str.str().c_str(), __FILENAME__, __LINE__); \
} while(0)

#define LOGA(msg) do \
{ mg::logAssert(msg.c_str()); \
} while(0)

namespace mg {

void logInfo(const char *str, const char* file, const int line);
void logAssert(const char *str);

} // namespace
