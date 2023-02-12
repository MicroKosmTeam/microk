#pragma once
#include <stddef.h>
#include <stdint.h>
#include <cdefs.h>
#include <fs/driver.h>

#define VFS_FILE_STDOUT		0x0000
#define VFS_FILE_STDIN		0x0001
#define VFS_FILE_STDERR		0x0002
#define VFS_FILE_STDLOG		0x0003

#define VFS_FLAG_ROOT		0x0001
#define VFS_FLAG_RAMFS		0x0002
#define VFS_FLAG_DEVFS		0x0004
#define VFS_FLAG_SYSFS		0x0006
#define VFS_FLAG_PROC		0x0008

#define VFS_NODE_FILE		0x0001
#define VFS_NODE_DIRECTORY	0x0002
#define VFS_NODE_CHARDEVICE	0x0004
#define VFS_NODE_BLOCKDEVICE	0x0006
#define VFS_NODE_FIFO           0x0008
#define VFS_NODE_PIPE		0x000A
#define VFS_NODE_SYMLINK	0x000C
#define VFS_NODE_MOUNTPOINT	0x000E

#define VFS_NODE_MAJOR_STDIO    0x0001

#define VFS_NODE_MINOR_STDOUT   0x0000
#define VFS_NODE_MINOR_STDIN    0x0001
#define VFS_NODE_MINOR_STDERR   0x0002
#define VFS_NODE_MINOR_STDLOG   0x0003

struct VFilesystem {
	uint64_t descriptor;
	FSDriver *driver;
	FSNode *node;
	FSNode *mountdir;
};

extern FILE *stdout;
extern FILE *stdin;
extern FILE *stderr;
extern FILE *stdlog;
extern VFilesystem *rootfs;
extern VFilesystem *sysfs;
extern VFilesystem *devtmpfs;
extern VFilesystem *proc;

#ifdef __cplusplus
extern "C" {
#endif
FSNode *VFSFindDir(VFilesystem *fs, FSNode *node, const char *name);
FSNode *VFSReadDir(VFilesystem *fs, FSNode *node, uint64_t index);
uint64_t VFSMakeDir(VFilesystem *fs, FSNode *node, const char *name, uint64_t mask, uint64_t uid, uint64_t gid);
uint64_t VFSMakeFile(VFilesystem *fs, FSNode *node, const char *name, uint64_t mask, uint64_t uid, uint64_t gid);
VFilesystem *VFSMountFS(FSNode *mountroot, FSDriver *fsdriver);
FILE *VFSOpen(VFilesystem *fs, FSNode *node, size_t bufferSize);
void VFSClose(VFilesystem *fs, FILE *file);
uint64_t VFSRead(VFilesystem *fs, FILE *file, uint64_t offset, size_t size, uint8_t **buffer);
uint64_t VFSWrite(VFilesystem *fs, FILE *file, uint64_t offset, size_t size, uint8_t buffer);
VFilesystem *VFSMountFS(FSNode *mountroot, FSDriver *fsdriver);
FILE *VFSMakeNode(VFilesystem *fs, FSNode *node, const char *name, uint64_t type, uint64_t nodeMajor, uint64_t nodeMinor);
void VFS_Init();
void VFS_LS(char *path);
void VFS_Mkdir(char *path, char *name);
void VFS_Touch(char *path, char *name);
void VFS_Print(VFilesystem *fs);
int VFS_Write(FILE *file, uint8_t *data, size_t size);
int VFS_Read(FILE *file, uint8_t **buffer, size_t size);

#ifdef __cplusplus
}
#endif

