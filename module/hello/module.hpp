#pragma once
#include <stdint.h>
#include <stddef.h>

extern void (*PrintK)(char *format, ...);

extern void *(*Malloc)(size_t size);
extern void (*Free)(void *p);

extern char *(*Strcpy)(char *strDest, const char *strSrc);

inline void *operator new(size_t size) { return Malloc(size); }
