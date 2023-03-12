#include "module.hpp"

void (*PrintK)(char *format, ...);

void *(*Malloc)(size_t size);
void (*Free)(void *p);

void (*Memcpy)(void *dest, void *src, size_t n);
void (*Memset)(void *start, uint8_t value, uint64_t num);
int (*Memcmp)(const void* buf1, const void* buf2, size_t count);

char *(*Strcpy)(char *strDest, const char *strSrc);

void *(*RequestPage)();
void *(*RequestPages)(size_t pages);
