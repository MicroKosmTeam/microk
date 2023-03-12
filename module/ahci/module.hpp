#pragma once
#include <stdint.h>
#include <stddef.h>

extern void (*PrintK)(char *format, ...);

extern void *(*Malloc)(size_t size);
extern void (*Free)(void *p);

extern void (*Memcpy)(void *dest, void *src, size_t n);
extern void (*Memset)(void *start, uint8_t value, uint64_t num);
extern int (*Memcmp)(const void* buf1, const void* buf2, size_t count);

extern char *(*Strcpy)(char *strDest, const char *strSrc);

extern void *(*RequestPage)();
extern void *(*RequestPages)(size_t pages);

inline void *operator new(size_t size) { return Malloc(size); }
