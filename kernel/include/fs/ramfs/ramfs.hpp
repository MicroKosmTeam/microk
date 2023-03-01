#pragma once
#include <fs/vfs.hpp>

/******************
 * MICROK's RAMFS *
 ******************
 *
 * DIRECTORY STRUCTURE
 *
 * /-------\    /------\    /---\
 * |0. ROOT| -> |1. DEV| -> |3. FB0|
 * \-------/    \------/    \---/
 *                |           V
 *                |         /---\
 *                |         |5. SDA|
 *                |         \---/
 *                V
 *              /------\    /---\
 *              |2. SYS| -> |4. BUS| -> ...
 *              \------/    \---/
 *               |            V
 *               |          /-----\
 *               |          |6. BLOCK| -> ...
 *               |          \-----/
 *               |
 *               V
 *             /-----------\
 *             |3. BOOT.CFG|
 *             \-----------/
 *               V
 *              NULL
 *
 *
 * INODE TABLE
 *
 * /-+------------------------------\
 * | 0    | 1   | 2   | | maxInodes |
 * +----------------- | |-----------+
 * | ROOT | DEV | SYS | | NULL      |
 * \-+------------------------------/
 *
 *  The inode table is used to access an object at a specific inode. The indoes are assigned sequentially,
 *  the first object gets 0, the second 1, etc. This makes it fast to get to an element ( O(1), as it's basically a vector. ),
 *  Eliminating the need to trasversing the whole filesystem structure which (being a liked list), is quite slow.
 *  If a file is removed, the inode is immediatly reused, as the algorithm searches for the first element in the inode table
 *  that is free. So, the most recent file should have the greatest inode unless a file has been deleted.
 */

struct RAMFSObject {
	uint8_t magic;            // Magic number
	char name[128];           // The object's name
	uint64_t length;          // The total length of the object (0 for directories)
	bool isFile;              // Is it a file.
	RAMFSObject *firstObject; // If it's a directory.
	uint8_t *fileData;        // If it's a file.
	FSNode *node;             // The object's node

	RAMFSObject *nextObject;  // The next object in the chain
};

class RAMFSDriver : public FSDriver {
public:
	RAMFSDriver(FSNode *mountpoint, const uint64_t argMaxInodes) : maxInodes(argMaxInodes) {
		FSInit(mountpoint);
	}

	void            FSInit(FSNode *mountpoint) override;
	void		FSDelete() override;
	uint64_t        FSReadFile(FILE *file, uint64_t offset, size_t size, uint8_t **buffer) override;
	uint64_t        FSWriteFile(FILE *file, uint64_t offset, size_t size, uint8_t *buffer) override;
	FILE           *FSOpenFile(FSNode *node, uint64_t descriptor) override;
	void            FSCloseFile(FILE *file) override;
	uint64_t	FSDeleteFile(FSNode *node) override;
	FSNode         *FSReadDir(FSNode *node, uint64_t index) override;
	FSNode         *FSMakeDir(FSNode *node, const char *name, uint64_t uid, uint64_t gid, uint64_t mask) override;
	FSNode         *FSMakeFile(FSNode *node, const char *name, uint64_t uid, uint64_t gid, uint64_t mask) override;
	FSNode         *FSFindDir(FSNode *node, const char *name) override;
	uint64_t        FSGetDirElements(FSNode *node) override;
	uint64_t        FSDeleteDir(FSNode *node) override;
private:
	uint64_t currentInode;     // The first free inode
	const uint64_t maxInodes;  // The maximum number of inodes (has to be defined at the start)

	RAMFSObject *rootFile;     // The root file, that has the root node

	RAMFSObject **inodeTable;  // The inode table
};
