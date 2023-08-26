#if !defined(ISMSVC)
#if (defined(_WIN32) && !(defined(__MINGW32__) || defined(__MINGW64__)))
#define ISMSVC 1
#else
#define ISMSVC 0
#endif
#endif

#if ISMSVC
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif
