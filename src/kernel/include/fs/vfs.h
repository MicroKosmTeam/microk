#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
	uint32_t descriptor;
	uint8_t *buffer;
	uint64_t bufferSize;
	uint64_t bufferPos = 0;
} FILE;

struct VFilesystem{
	uint16_t mountpoints = 0;
	VFilesystem *up[256];
	char *mountpoint;
	char *drive;
	uint64_t flags;
};

extern FILE *stdout;
extern FILE *stdin;
extern FILE *stderr;
extern FILE *stdlog;

#ifdef __cplusplus
extern "C" {
#endif

void VFS_Tree();
void VFS_Init();
int VFS_Mount(VFilesystem *base, char *path, char *drive, uint64_t flags);
int VFS_Write(FILE *file, uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif

