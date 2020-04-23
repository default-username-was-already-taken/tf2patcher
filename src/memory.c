#include "base.h"
#include "common.h"


// read sz bytes from base
bool read_mem(const void *base, void *buf, size_t sz)
{
    char error_buf[8192];
    SIZE_T bytes_read = 0;

    assert(base && buf && sz);

    if (!ReadProcessMemory(pinfo.process, base, buf, sz, &bytes_read)) {
        fprintf(stderr, "Read error: %s\n", describe_error(error_buf, sizeof(error_buf)));
    } else {
        if (bytes_read != sz) {
            fprintf(stderr, "Read error");
        } else {
            return true;
        }
    }

    return false;
}


bool compare_mem(const void *dst, const void *pat, size_t sz)
{
    assert(dst && pat && sz);

    const unsigned char *dst_s = dst;
    const unsigned char *pat_s = pat;

    while (sz--) {
        if (*pat_s != 0xFF && *dst_s != *pat_s) {
            return false;
        }

        dst_s++;
        pat_s++;
    }

    return true;
}

void *find_mem(const void *pattern, size_t sz, unsigned char *search_base, size_t search_size)
{
    unsigned char buf[4096];
    size_t num_bad_read = 0;

    assert(pattern && sz && search_base && search_size);

    unsigned char *this = search_base;
    unsigned char *end = search_base + search_size;

    while (this < end) {
        size_t this_bytes = end - this;
        if (this_bytes > sizeof(buf)) {
            this_bytes = sizeof(buf);
        }

        if (!read_mem(this, buf, this_bytes)) {
            if (++num_bad_read < 10) {
                fprintf(stderr, "Cannot read %zu bytes from %p\n", this_bytes, this);
            }
        } else {
            if (this_bytes >= sz) {
                for (size_t i = 0; i < this_bytes - sz + 1; i++) {
                    if (compare_mem(&buf[i], pattern, sz)) {
                        return this + i;
                    }
                }

                if (this + this_bytes < end) {
                    this_bytes -= (sz - 1); // if it's not the end, go back
                }
            }
        }

        this += this_bytes;
    }

    return NULL;
}


// wrapper for searching in client library
void *find_mem_cl(const void *pattern, size_t sz)
{
    return find_mem(pattern, sz, pinfo.cl_base, pinfo.cl_size);
}




bool set_mem(void *addr, const unsigned char pat[], size_t sz)
{
    char error_buf[8192];
    SIZE_T bytes_written = 0;

    assert(addr && pat && sz);

    if (!WriteProcessMemory(pinfo.process, addr, pat, sz, &bytes_written)) {
        fprintf(stderr, "Write error: %s\n", describe_error(error_buf, sizeof(error_buf)));
    } else {
        if (bytes_written != sz) {
            fprintf(stderr, "Write error");
        } else {
            return true;
        }
    }

    return false;
}
