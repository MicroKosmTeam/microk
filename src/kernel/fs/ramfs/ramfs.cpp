#include <fs/ramfs/ramfs.h>
#include <mm/memory.h>
#include <mm/string.h>

void RAMFSDriver::FSInit() {
	inodeTable = (RAMFSObject**)malloc(sizeof(RAMFSObject) * maxInodes);
	currentInode = 0;

	for(int i = 0; i < maxInodes; i++) {
		inodeTable[i] = NULL;
	}

	rootFile = (RAMFSObject*)malloc(sizeof(RAMFSObject));
	rootFile->magic = 0;
	strcpy(rootFile->name, "ramfs");
	rootFile->length = 0;
	rootFile->isFile = false;
	rootFile->firstObject = NULL;
	rootNode = (FSNode*)malloc(sizeof(FSNode));
	rootFile->node = rootNode;
	strcpy(rootFile->node->name, "ramfs");
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

	rootFile->nextObject = NULL;

	inodeTable[currentInode++] = rootFile;

	inodeTable[currentInode] = NULL;
}

uint64_t RAMFSDriver::FSRead(FSNode *node, uint64_t offset, size_t size, uint8_t **buffer) {

}

uint64_t RAMFSDriver::FSWrite(FSNode *node, uint64_t offset, size_t size, uint8_t *buffer) {

}

void RAMFSDriver::FSOpen(FSNode *node) {

}

#include <sys/printk.h>
DirectoryEntry *RAMFSDriver::FSReadDir(FSNode *node, uint64_t index) {
	if(node->inode > maxInodes) return 0;
	if(inodeTable[node->inode] == NULL) return 0;
	if(inodeTable[node->inode]->firstObject == NULL) return 0;
	if(inodeTable[node->inode]->isFile == true) return 0;

	RAMFSObject *directoryEntry = inodeTable[node->inode]->firstObject;

	for (int i = 0; i < index; i++) {
		if (directoryEntry->nextObject == NULL) return 0;
		directoryEntry = directoryEntry->nextObject;
	}

	DirectoryEntry *entry = (DirectoryEntry*)malloc(sizeof(DirectoryEntry)); // Remember to deallocate this!

	strcpy(entry->name, directoryEntry->name);
	entry->inode = directoryEntry->node->inode;
	return entry;
}

uint64_t RAMFSDriver::FSMakeDir(FSNode *node, const char *name) {
	if(node->inode > maxInodes) return 0;
	if(inodeTable[node->inode] == NULL) return 0;
	if(inodeTable[node->inode]->isFile == true) return 0;

	RAMFSObject *prevDirectoryEntry = inodeTable[node->inode]->firstObject;

	if(prevDirectoryEntry == NULL) {
		inodeTable[node->inode]->firstObject = (RAMFSObject*)malloc(sizeof(RAMFSObject));
		inodeTable[node->inode]->firstObject->magic = 0;
		strcpy(inodeTable[node->inode]->firstObject->name, name);
		inodeTable[node->inode]->firstObject->length = 0;
		inodeTable[node->inode]->firstObject->isFile = false;
		inodeTable[node->inode]->firstObject->firstObject = NULL;
		inodeTable[node->inode]->firstObject->node = (FSNode*)malloc(sizeof(FSNode));
		strcpy(inodeTable[node->inode]->firstObject->node->name, name);
		inodeTable[node->inode]->firstObject->node->mask = inodeTable[node->inode]->firstObject->node->uid = inodeTable[node->inode]->firstObject->node->gid = inodeTable[node->inode]->firstObject->node->size = inodeTable[node->inode]->firstObject->node->impl = 0;
		inodeTable[node->inode]->firstObject->node->flags = VFS_NODE_DIRECTORY;
		inodeTable[node->inode]->firstObject->node->inode = currentInode;
		inodeTable[node->inode]->firstObject->node->read = NULL;
		inodeTable[node->inode]->firstObject->node->write = NULL;
		inodeTable[node->inode]->firstObject->node->open = NULL;
		inodeTable[node->inode]->firstObject->node->close = NULL;
		inodeTable[node->inode]->firstObject->node->readdir = (readdir_type_t)&this->FSReadDir;
		inodeTable[node->inode]->firstObject->node->finddir = (finddir_type_t)&this->FSFindDir;
		inodeTable[node->inode]->firstObject->node->ptr = NULL;
		inodeTable[node->inode]->firstObject->nextObject = NULL;
		inodeTable[currentInode++] = inodeTable[node->inode]->firstObject;

		return currentInode-1;
	} else {
		RAMFSObject *newDirectoryEntry = prevDirectoryEntry->nextObject;

		while (newDirectoryEntry->nextObject != NULL) {
			newDirectoryEntry = newDirectoryEntry->nextObject;
			prevDirectoryEntry = prevDirectoryEntry->nextObject;
		}

		newDirectoryEntry = (RAMFSObject*)malloc(sizeof(RAMFSObject));
		newDirectoryEntry->magic = 0;
		strcpy(newDirectoryEntry->name, name);
		newDirectoryEntry->length = 0;
		newDirectoryEntry->isFile = false;
		newDirectoryEntry->firstObject = NULL;
		newDirectoryEntry->node = (FSNode*)malloc(sizeof(FSNode));
		strcpy(newDirectoryEntry->node->name, name);
		newDirectoryEntry->node->mask = newDirectoryEntry->node->uid = newDirectoryEntry->node->gid = newDirectoryEntry->node->size = newDirectoryEntry->node->impl = 0;
		newDirectoryEntry->node->flags = VFS_NODE_DIRECTORY;
		newDirectoryEntry->node->inode = currentInode;
		newDirectoryEntry->node->read = NULL;
		newDirectoryEntry->node->write = NULL;
		newDirectoryEntry->node->open = NULL;
		newDirectoryEntry->node->close = NULL;
		newDirectoryEntry->node->readdir = (readdir_type_t)&this->FSReadDir;
		newDirectoryEntry->node->finddir = (finddir_type_t)&this->FSFindDir;
		newDirectoryEntry->node->ptr = NULL;
		newDirectoryEntry->nextObject = NULL;

		inodeTable[currentInode++] = newDirectoryEntry;
		prevDirectoryEntry->nextObject = newDirectoryEntry;

		return currentInode-1;
	}
}

FSNode *RAMFSDriver::FSFindDir(FSNode *node, const char *name) {

}
