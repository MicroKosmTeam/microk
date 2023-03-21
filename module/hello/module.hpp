#pragma once
#include <stdint.h>
#include <stddef.h>
#include <mkmi.hpp>

extern void (*PrintK)(char *format, ...);

extern void *(*Malloc)(size_t size);
extern void (*Free)(void *p);

extern char *(*Strcpy)(char *strDest, const char *strSrc);

extern void (*Memcpy)(void *dest, void *src, size_t n);
extern void (*Memset)(void *start, uint8_t value, uint64_t num);
extern int (*Memcmp)(const void* buf1, const void* buf2, size_t count);

extern MKMI::BUFFER::Buffer *(*BufferCreate)(MKMI::BUFFER::BufferType type, size_t size);
extern uint64_t (*BufferIOCtl)(MKMI::BUFFER::Buffer *buffer, MKMI::BUFFER::BufferOperation operation, ...);
extern uint64_t (*BufferDelete)(MKMI::BUFFER::Buffer *buffer);

inline void *operator new(size_t size) { return Malloc(size); }
inline void *operator new[](size_t size) { return Malloc(size); }
