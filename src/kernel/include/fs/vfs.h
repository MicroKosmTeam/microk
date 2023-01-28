#pragma once
#include <stddef.h>
#include <stdint.h>

#define VFS_FILE_STDOUT 0
#define VFS_FILE_STDIN  1
#define VFS_FILE_STDERR 2
#define VFS_FILE_STDLOG 3

typedef struct {
	uint32_t descriptor;
} FILE;

typedef int fd_t;

extern FILE *stdout;
extern FILE *stdlog;

#ifdef __cplusplus
extern "C" {
#endif

void VFS_Init();

int VFS_WriteFile(fd_t file, uint8_t *data, size_t size);
int VFS_Write(FILE *file, uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif

