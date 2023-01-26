#pragma once
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <fs/vfs.h>
#ifdef __cplusplus
extern "C" {
#endif
void fputc(char c, fd_t file);
void fputs(const char *str, fd_t file);
void fprintf(fd_t file, const char *format, ...);
void vfprintf(fd_t file, const char *format, va_list args);
void putc(char c);
void puts(const char *str);
void printf(const char *format, ...);
#ifdef __cplusplus
}
#endif
