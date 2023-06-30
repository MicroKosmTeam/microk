#pragma once
#include <stdint.h>
#include <stddef.h>

#define MAX_FILE_NAME_LENGTH 512

#ifdef __cplusplus
extern "C" {
#endif 

typedef uintmax_t inode_t;
typedef uint32_t gid_t;
typedef uint32_t uid_t;
typedef uint32_t mode_t;
typedef intmax_t time_t;
typedef uintmax_t fd_t;
typedef uintmax_t fs_t;
typedef uintmax_t driver_t;

typedef struct {
	fs_t FilesystemDescriptor;
	inode_t InodeNumber;
	
	uint8_t Properties;

	mode_t Permissions;
	uid_t Owner;
	gid_t Group;
	size_t Size;

	time_t CreationTime;
	time_t ModificationTime;
	time_t AccessTime;

	char Name[MAX_FILE_NAME_LENGTH];
} Inode;

typedef struct {
	fd_t FileDescriptor;
} File;

typedef struct {
	fs_t FilesystemDescriptor;
	driver_t DriverID;

	uintmax_t BlockSize;
	uintmax_t TotalBlocks;
	uintmax_t FreeBlocks;

	uintmax_t InodeSize;
	uintmax_t TotalInodes;
	uintmax_t FreeInodes;

	Inode *RootNode;
} Filesystem;

void VFSInit();

driver_t RegisterDriver();
void UnregisterDriver(driver_t driver);

fs_t RegisterFilesystem(driver_t driverID);
void UnregisterFilesystem(fs_t filesystem);

int MountFilesystem(Filesystem *fs, const char *path);
int UnmountFilesystem(const char *path);

#ifdef __cplusplus
}
#endif
