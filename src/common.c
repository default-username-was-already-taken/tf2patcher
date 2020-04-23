#include "base.h"
#include "common.h"

#ifdef WINDOWS
    const char path_sep = '\\';
    const char *path_sep_str = "\\";
#elif LINUX
    const char path_sep = '/';
    const char *path_sep_str = "/";
#endif


pinfo_t pinfo = {
    .verbose_mode = false
};



void verbose_print(const char *format, ...)
{
    if (pinfo.verbose_mode) {
        va_list va;
        va_start(va, format);
        vprintf(format, va);
        va_end(va);
    }
}


// a mix of strcat and strlcpy for easy string chaining
// dst: destination buffer
// src: source 0-term string
// sz: total buffer size
char *strlecpy(char *dst, const char *src, size_t sz)
{
    assert(dst && src);

    if (sz) {
        while (--sz && *src != '\0') {
            *dst++ = *src++;
        }

        *dst = '\0';
    }

    return dst;
}


// given a full file path, extract just the name part
char *extract_file_name(const char *name, char *buf, size_t sz)
{
    assert(name && buf && sz);

    const char *s = strrchr(name, path_sep);
    #ifdef WINDOWS
        if (!s) { s = strrchr(name, ':'); } // handle cases like C:test.txt
    #endif

    strlecpy(buf, s ? s + 1 : name, sz);
    return buf;
}


#ifdef WINDOWS
    // describe error code provided by GetLastError
    char* describe_error(char *buf, size_t sz)
    {
        assert(buf && sz);

        DWORD err = GetLastError();
        if (!err) {
            *buf = '\0';
        } else {
            if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, sz, NULL)) {
                strlecpy(buf, "Unknown error", sz);
            }
        }

        return buf;
    }
#endif
