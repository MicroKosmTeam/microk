#pragma once
#include <stdint.h>
#include <fs/node.h>
#include <fs/file.h>

/* FSDriver
 *  This is the basic class for any filesystem driver
 *
 */
class FSDriver {
public:
	virtual void            FSInit() = 0;

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

	virtual uint64_t       *FSDeleteDir(FSNode *node);

	FSNode *rootNode;
private:
};

