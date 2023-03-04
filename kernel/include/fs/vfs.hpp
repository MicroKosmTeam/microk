#pragma once
#include <stdint.h>
#include <stddef.h>
#include <init/kinfo.hpp>

#define VFS_NODE_FILE		0x0001
#define VFS_NODE_DIRECTORY	0x0002
#define VFS_NODE_CHARDEVICE	0x0004
#define VFS_NODE_BLOCKDEVICE	0x0006
#define VFS_NODE_FIFO           0x0008
#define VFS_NODE_PIPE		0x000A
#define VFS_NODE_SYMLINK	0x000C
#define VFS_NODE_MOUNTPOINT	0x000E

struct FSNode;

/* FILE
 *  The standard file I/O struct
 *
 */
struct FILE {
	FSNode *node;		// The node that is linked to this file
	uint64_t *buffer;	// The loaded contents of the file
	uint64_t descriptor;	// The descriptor of the file
	uint64_t bufferSize;	// The size read/write buffer
	uint64_t bufferPos;	// Where we last wrote to/read from
};

/* FSDriver
 *  This is the basic class for any filesystem driver
 *
 */
class FSDriver {
public:
	virtual void            FSInit(FSNode *mountpoint) = 0;

	virtual void            FSDelete() = 0;

	virtual FSNode         *FSMakeFile(FSNode *node, const char *name, uint64_t uid, uint64_t gid, uint64_t mask) = 0;

	virtual FILE           *FSOpenFile(FSNode *node, uint64_t descriptor) = 0;

	virtual uint64_t        FSReadFile(FILE *file, uint64_t offset, size_t size, uint8_t **buffer) = 0;

	virtual uint64_t        FSWriteFile(FILE *file, uint64_t offset, size_t size, uint8_t *buffer) = 0;

	virtual void            FSCloseFile(FILE *file) = 0;

	virtual uint64_t	FSDeleteFile(FSNode *node) = 0;

	virtual FSNode         *FSMakeDir(FSNode *node, const char *name, uint64_t uid, uint64_t gid, uint64_t mask) = 0;

	virtual FSNode         *FSReadDir(FSNode *node, uint64_t index) = 0;

	virtual FSNode         *FSFindDir(FSNode *node, const char *name) = 0;

	virtual uint64_t        FSGetDirElements(FSNode *node) = 0;

	virtual uint64_t        FSDeleteDir(FSNode *node) = 0;

	FSNode *rootNode;
private:
};

/* FSNode
 *  This permits an implementation-independent communcation between
 *  all the components of the VFS
 */
struct FSNode {
	FSDriver *driver;       // The filesystem driver

	char name[128];		// The name of the current object
	uint64_t mask;		// Permission mask
	uint64_t uid;		// User ID
	uint64_t gid;		// Group ID
	uint64_t flags;		// Node's flags (es: file, directory, symlink...)
	uint64_t inode;		// Implementation-driven inode for filesystem drivers
	uint64_t size;		// The node's size
	uint64_t impl;		// Free parameter for filesystem drivers
};

struct VFilesystem {
	FSNode *node;
	FSNode *mountdir;

	uint64_t flags;
};

namespace VFS {
	void ListDir(FSNode *dir);


	void Init(KInfo *info);

	FSNode *GetNode(VFilesystem *fs, char *path);

	VFilesystem *GetRootFS();
	VFilesystem *GetInitrdFS();

	VFilesystem *MountFS(FSNode *mountroot, FSDriver *fsdriver, uint64_t flags);
	uint64_t RemountFS(VFilesystem *fs, uint64_t flags);
	uint64_t UmountFS(VFilesystem *fs);
	FSNode *MakeFile(FSNode *node, const char *name, uint64_t uid, uint64_t gid, uint64_t mask);
	FILE *OpenFile(FSNode *node);
	uint64_t GetFileSize(FILE *file);
	uint64_t ReadFile(FILE *file, uint64_t offset, size_t size, uint8_t **buffer);
	uint64_t WriteFile(FILE *file, uint64_t offset, size_t size, uint8_t *buffer);
	void CloseFile(FILE *file);
	uint64_t DeleteFile(FSNode *node);
	FSNode *MakeDir(FSNode *node, const char *name, uint64_t uid, uint64_t gid, uint64_t mask);
	FSNode *ReadDir(FSNode *node, uint64_t index);
	FSNode *FindDir(FSNode *node, const char *name);
	uint64_t GetDirElements(FSNode *node);
	uint64_t DeleteDir(FSNode *node);
}
