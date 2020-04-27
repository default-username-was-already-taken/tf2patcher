#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "base.h"
#include "common.h"

bool read_mem(const void *base, void *buf, size_t sz);
bool set_mem(void *addr, const unsigned char pat[], size_t sz);

bool compare_mem(const void *dst, const void *pat, size_t sz);

void *find_mem_cl(const void *pattern, size_t sz);
void *find_mem(const void *pattern, size_t sz, unsigned char *search_base, size_t search_size);

#endif
