#include "module.hpp"

void (*PrintK)(char *format, ...);

void *(*Malloc)(size_t size);
void (*Free)(void *p);

char *(*Strcpy)(char *strDest, const char *strSrc);
