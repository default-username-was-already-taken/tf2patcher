// dont allow mingw to use msvcrt for stdio
#define __USE_MINGW_ANSI_STDIO 1

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// normalize platform-dependent definitions
#if defined(_WIN32)
    #define WINDOWS
#elif defined(__linux__) || defined(linux)
    #define LINUX
#endif

#ifdef WINDOWS
    #include "windows.h"
    #include "psapi.h"
#endif
