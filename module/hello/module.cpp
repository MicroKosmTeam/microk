#include "module.hpp"

void (*PrintK)(char *format, ...);

void *(*Malloc)(size_t size);
void (*Free)(void *p);

char *(*Strcpy)(char *strDest, const char *strSrc);

void (*Memcpy)(void *dest, void *src, size_t n);
void (*Memset)(void *start, uint8_t value, uint64_t num);
int (*Memcmp)(const void* buf1, const void* buf2, size_t count);

MKMI::Buffer *(*BufferCreate)(uint64_t code, MKMI::BufferType type, size_t size);
uint64_t (*BufferIO)(MKMI::Buffer *buffer, MKMI::BufferOperation operation, ...);
uint64_t (*BufferDelete)(MKMI::Buffer *buffer);
