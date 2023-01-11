#pragma once
#include <mm/memory.h>

extern "C" void memset(void *start, uint8_t value, uint64_t num);
extern "C" void memcpy(void *dest, void *src, size_t n);
