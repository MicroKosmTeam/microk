#pragma once
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <fs/vfs.h>
#ifdef __cplusplus
extern "C" {
#endif
void fputc(char c, FILE *file);
void fputs(const char *str, FILE *file);
void fprintf(FILE *file, const char *format, ...);
void nfprintf(FILE *file, const char *format, ...);
void vfprintf(FILE *file, const char *format, va_list args);
void putc(char c);
void puts(const char *str);
void printf(const char *format, ...);
void dprintf(const char *format, ...);
#ifdef __cplusplus
}
#endif

