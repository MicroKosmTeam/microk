#pragma once
#include <stdint.h>
#include <stddef.h>
#include <mkmi.hpp>

extern void (*PrintK)(char *format, ...);

extern void *(*Malloc)(size_t size);
extern void (*Free)(void *p);

extern char *(*Strcpy)(char *strDest, const char *strSrc);

extern MKMI::BUFFER::Buffer (*BufferCreate)(MKMI::BUFFER::BufferType type, size_t size);
extern uint64_t (*BufferIOCtl)(MKMI::BUFFER::Buffer *buffer, MKMI::BUFFER::BufferOperation operation, ...);
extern uint64_t (*BufferDelete)(MKMI::BUFFER::Buffer *buffer);

inline void *operator new(size_t size) { return Malloc(size); }
