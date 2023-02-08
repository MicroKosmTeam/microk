#pragma once
#include <stddef.h>
#include <stdint.h>
#include <fs/driver.h>

#define VFS_FILE_STDOUT		0x0000
#define VFS_FILE_STDIN		0x0001
#define VFS_FILE_STDERR		0x0002
#define VFS_FILE_STDLOG		0x0003

#define VFS_FLAG_ROOT		0x0001
#define VFS_FLAG_RAMFS		0x0002
#define VFS_FLAG_DEVFS		0x0004
#define VFS_FLAG_SYSFS		0x0008
#define VFS_FLAG_PROC		0x0010

#define VFS_NODE_FILE		0x0001
#define VFS_NODE_DIRECTORY	0x0002
#define VFS_NODE_CHARDEVICE	0x0003
#define VFS_NODE_BLOCKDEVICE	0x0004
#define VFS_NODE_PIPE		0x0005
#define VFS_NODE_SYMLINK	0x0006
#define VFS_NODE_MOUNTPOINT	0x0008

struct FILE {
	uint64_t descriptor;
	FSNode *node;
	uint8_t *buffer;
	uint64_t bufferSize;
	uint64_t bufferPos = 0;
};

struct VFilesystem{
	FSNode *node;
	FSDriver *driver;
};

struct DirectoryEntry {
	char name[128];
	uint64_t inode;
};

extern FILE *stdout;
extern FILE *stdin;
extern FILE *stderr;
extern FILE *stdlog;

#ifdef __cplusplus
extern "C" {
#endif
uint64_t VFS_ReadNode(FSNode *node, uint64_t offset, uint64_t size, uint8_t **buffer);
uint64_t VFS_WriteNode(FSNode *node, uint64_t offset, uint64_t size, uint8_t *buffer);
void VFS_OpenNode(FSNode *node, uint8_t read, uint8_t write);
void VFS_CloseNode(FSNode *node);
struct DirectoryEntry *VFS_ReadDirNode(FSNode *node, uint64_t index);
FSNode *VFS_FindDirNode(FSNode *node, char *name);
void VFS_Init();
int VFS_Write(FILE *file, uint8_t *data, size_t size);
int VFS_Read(FILE *file, uint8_t **buffer, size_t size);

#ifdef __cplusplus
}
#endif

