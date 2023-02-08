#include <fs/ramfs/ramfs.h>
#include <mm/memory.h>

void RAMFSDriver::FSInit() {
	inodeTable = (RAMFSObject**)malloc(sizeof(RAMFSObject) * maxInodes);
	currentInode = 0;

	for(int i = 0; i < maxInodes; i++) {
		inodeTable[i] = NULL;
	}

	rootFile = (RAMFSObject*)malloc(sizeof(RAMFSObject));
	rootFile->magic = 0;
	memcpy(rootFile->name, "ramfs\0", 6);
	rootFile->length = 0;
	rootFile->isFile = false;
	rootFile->firstObject = NULL;
	rootFile->node = (FSNode*)malloc(sizeof(FSNode));
	memcpy(rootFile->node->name, "ramfs\0", 6);
	rootFile->node->mask = rootFile->node->uid = rootFile->node->gid = rootFile->node->size = rootFile->node->impl = 0;
	rootFile->node->flags = VFS_NODE_DIRECTORY;
	rootFile->node->inode = currentInode;
	rootFile->node->read = NULL;
	rootFile->node->write = NULL;
	rootFile->node->open = NULL;
	rootFile->node->close = NULL;
	rootFile->node->readdir = (readdir_type_t)&this->FSReadDir;
	rootFile->node->finddir = (finddir_type_t)&this->FSFindDir;
	rootFile->node->ptr = NULL;

	rootFile->nextObject;

	inodeTable[currentInode] = rootFile;
	inodeTable[currentInode++] = NULL;
}

uint64_t RAMFSDriver::FSRead(FSNode *node, uint64_t offset, size_t size, uint8_t **buffer) {

}

uint64_t RAMFSDriver::FSWrite(FSNode *node, uint64_t offset, size_t size, uint8_t *buffer) {

}

void RAMFSDriver::FSOpen(FSNode *node) {

}

DirectoryEntry *RAMFSDriver::FSReadDir(FSNode *node, uint64_t index) {

}

uint64_t RAMFSDriver::FSMakeDir(FSNode *node, const char *name) {

}

FSNode *RAMFSDriver::FSFindDir(FSNode *node, const char *name) {

}
