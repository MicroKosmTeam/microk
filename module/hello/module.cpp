#include "module.hpp"

void (*PrintK)(char *format, ...);

void *(*Malloc)(size_t size);
void (*Free)(void *p);

char *(*Strcpy)(char *strDest, const char *strSrc);

void (*Memcpy)(void *dest, void *src, size_t n);
void (*Memset)(void *start, uint8_t value, uint64_t num);
int (*Memcmp)(const void* buf1, const void* buf2, size_t count);

MKMI::BUFFER::Buffer *(*BufferCreate)(MKMI::BUFFER::BufferType type, size_t size);
uint64_t (*BufferIOCtl)(MKMI::BUFFER::Buffer *buffer, MKMI::BUFFER::BufferOperation operation, ...);
uint64_t (*BufferDelete)(MKMI::BUFFER::Buffer *buffer);
