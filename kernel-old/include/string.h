#pragma once
#include <mm/memory.h>

#ifdef __cplusplus
extern "C" {
#endif
void memset(void *start, uint8_t value, uint64_t num);
void memcpy(void *dest, void *src, size_t n);
#ifdef __cplusplus
}
#endif

