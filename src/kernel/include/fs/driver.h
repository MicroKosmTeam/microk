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

class FSDriver {
public:
	virtual void            FSInit() = 0;
	virtual uint64_t        FSRead(FSNode *node, uint64_t offset, size_t size, uint8_t **buffer) = 0;
	virtual uint64_t        FSWrite(FSNode *node, uint64_t offset, size_t size, uint8_t *buffer) = 0;
	virtual void            FSOpen(FSNode *node) = 0;
	virtual FSNode         *FSReadDir(FSNode *node, uint64_t index) = 0;
	virtual uint64_t        FSMakeDir(FSNode *node, const char *name, uint64_t uid, uint64_t gid, uint64_t mask) = 0;
	virtual uint64_t        FSMakeFile(FSNode *node, const char *name, uint64_t uid, uint64_t gid, uint64_t mask) = 0;
	virtual FSNode         *FSFindDir(FSNode *node, const char *name) = 0;
	virtual uint64_t        FSGetDirElements(FSNode *node) = 0;
	FSNode *rootNode;
private:

};

