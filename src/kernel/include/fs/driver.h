#pragma once
#include <stdint.h>
#include <stddef.h>

struct FSNode;

typedef uint64_t (*read_type_t)(struct FSNode*,uint64_t,uint64_t,uint8_t**);
typedef uint64_t (*write_type_t)(struct FSNode*,uint64_t,uint64_t,uint8_t*);
typedef void (*open_type_t)(struct FSNode*);
typedef void (*close_type_t)(struct FSNode*);
typedef struct DirectoryEntry * (*readdir_type_t)(struct FSNode*,uint64_t);
typedef struct FSNode * (*finddir_type_t)(struct FSNode*,char *name);

struct FSNode {
	char name[128];
	uint64_t mask;
	uint64_t uid;
	uint64_t gid;
	uint64_t flags;
	uint64_t inode;
	uint64_t size;
	uint64_t impl;

	read_type_t read;
	write_type_t write;
	open_type_t open;
	close_type_t close;
	readdir_type_t readdir;
	finddir_type_t finddir;

	struct FSNode *ptr;
};

class FSDriver {
public:
	virtual void            FSInit() = 0;
	virtual uint64_t        FSRead(FSNode *node, uint64_t offset, size_t size, uint8_t **buffer) = 0;
	virtual uint64_t        FSWrite(FSNode *node, uint64_t offset, size_t size, uint8_t *buffer) = 0;
	virtual void            FSOpen(FSNode *node) = 0;
	virtual DirectoryEntry *FSReadDir(FSNode *node, uint64_t index) = 0;
	virtual uint64_t        FSMakeDir(FSNode *node, const char *name) = 0;
	virtual FSNode         *FSFindDir(FSNode *node, const char *name) = 0;
private:

};

