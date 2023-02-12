#pragma once
#include <stdint.h>
#include <stddef.h>
#include <cdefs.h>

struct FSNode {
	char name[VFS_FILE_MAX_NAME_LENGTH];
	uint64_t mask;
	uint64_t uid;
	uint64_t gid;
	uint64_t flags;
	uint64_t inode;
	uint64_t size;
	uint64_t impl;
};

struct FILE {
	uint64_t descriptor;
	uint64_t fsDescriptor;
	FSNode *node;
	uint8_t *buffer;
	uint64_t bufferSize;
	uint64_t bufferPos = 0;
};

class FSDriver {
public:
	virtual void            FSInit() = 0;
	virtual uint64_t        FSRead(FILE *file, uint64_t offset, size_t size, uint8_t **buffer) = 0;
	virtual uint64_t        FSWrite(FILE *file, uint64_t offset, size_t size, uint8_t *buffer) = 0;
	virtual FILE           *FSOpen(FSNode *node, uint64_t descriptor) = 0;
	virtual void            FSClose(FILE *file) = 0;
	virtual FSNode         *FSReadDir(FSNode *node, uint64_t index) = 0;
	virtual uint64_t        FSMakeDir(FSNode *node, const char *name, uint64_t uid, uint64_t gid, uint64_t mask) = 0;
	virtual uint64_t        FSMakeFile(FSNode *node, const char *name, uint64_t uid, uint64_t gid, uint64_t mask) = 0;
	virtual FSNode         *FSFindDir(FSNode *node, const char *name) = 0;
	virtual uint64_t        FSGetDirElements(FSNode *node) = 0;
	FSNode *rootNode;
private:

};

