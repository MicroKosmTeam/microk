#pragma once
#include <stddef.h>
#include <stdint.h>

#define VFS_FILE_STDOUT 0
#define VFS_FILE_STDIN  1
#define VFS_FILE_STDERR 2
#define VFS_FILE_STDLOG 3

typedef int fd_t;

#ifdef __cplusplus
extern "C" {
#endif

int VFS_WriteFile(fd_t file, uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif
