#include "module.hpp"

void (*PrintK)(char *format, ...);

void *(*Malloc)(size_t size);
void (*Free)(void *p);

char *(*Strcpy)(char *strDest, const char *strSrc);

MKMI::BUFFER::Buffer (*BufferCreate)(MKMI::BUFFER::BufferType type, size_t size);
uint64_t (*BufferIOCtl)(MKMI::BUFFER::Buffer *buffer, MKMI::BUFFER::BufferOperation operation, ...);
uint64_t (*BufferDelete)(MKMI::BUFFER::Buffer *buffer);
