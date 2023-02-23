#pragma once
#include <stdint.h>
#include <stddef.h>
#include <limine.h>

extern uint64_t hhdm;
#ifdef __cplusplus
extern "C" {
#endif
uint64_t get_memory_size(limine_memmap_entry **mMap, uint64_t mMapEntries);
void memset(void *start, uint8_t value, uint64_t num);
void memcpy(void *dest, void *src, size_t n);
int memcmp(const void* buf1, const void* buf2, size_t count);
void *vmalloc(size_t size);
void *malloc(size_t size);
void free(void *address);
#ifdef __cplusplus
}
#endif

