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

FSNode *RAMFSDriver::FSReadDir(FSNode *node, uint64_t index) {
	if(node->inode > maxInodes) return 0;
	if(inodeTable[node->inode] == NULL) return 0;
	if(inodeTable[node->inode]->firstObject == NULL) return 0;
	if(inodeTable[node->inode]->isFile == true) return 0;

	RAMFSObject *directoryEntry = inodeTable[node->inode]->firstObject;

	for (int i = 0; i < index; i++) {
		if (directoryEntry->nextObject == NULL) return 0;
		directoryEntry = directoryEntry->nextObject;
	}

	FSNode *entry = (FSNode*)malloc(sizeof(FSNode)); // Remember to deallocate this!

	strcpy(entry->name, directoryEntry->name);
	entry->mask = directoryEntry->node->mask;
	entry->uid = directoryEntry->node->uid;
	entry->gid = directoryEntry->node->gid;
	entry->flags = directoryEntry->node->flags;
	entry->inode = directoryEntry->node->inode;
	entry->size = directoryEntry->node->size;
	entry->impl = directoryEntry->node->impl;

	return entry;
}

uint64_t RAMFSDriver::FSMakeDir(FSNode *node, const char *name, uint64_t uid, uint64_t gid, uint64_t mask) {
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
		inodeTable[node->inode]->firstObject->node->mask = mask;
		inodeTable[node->inode]->firstObject->node->uid = uid;
		inodeTable[node->inode]->firstObject->node->gid = gid;
		inodeTable[node->inode]->firstObject->node->size = inodeTable[node->inode]->firstObject->node->impl = 0;
		inodeTable[node->inode]->firstObject->node->flags = VFS_NODE_DIRECTORY;
		inodeTable[node->inode]->firstObject->node->inode = currentInode;
		inodeTable[node->inode]->firstObject->nextObject = NULL;
		inodeTable[currentInode++] = inodeTable[node->inode]->firstObject;

		return currentInode-1;
	} else {
		RAMFSObject *newDirectoryEntry;

		newDirectoryEntry = (RAMFSObject*)malloc(sizeof(RAMFSObject));
		newDirectoryEntry->magic = 0;
		strcpy(newDirectoryEntry->name, name);
		newDirectoryEntry->length = 0;
		newDirectoryEntry->isFile = false;
		newDirectoryEntry->firstObject = NULL;
		newDirectoryEntry->node = (FSNode*)malloc(sizeof(FSNode));
		strcpy(newDirectoryEntry->node->name, name);
		newDirectoryEntry->node->mask = mask;
		newDirectoryEntry->node->uid = uid;
		newDirectoryEntry->node->gid = gid;
		newDirectoryEntry->node->size = newDirectoryEntry->node->impl = 0;
		newDirectoryEntry->node->flags = VFS_NODE_DIRECTORY;
		newDirectoryEntry->node->inode = currentInode;
		newDirectoryEntry->nextObject = prevDirectoryEntry;

		inodeTable[currentInode++] = newDirectoryEntry;
		inodeTable[node->inode]->firstObject = newDirectoryEntry;

		return currentInode-1;
	}
}

uint64_t RAMFSDriver::FSMakeFile(FSNode *node, const char *name, uint64_t uid, uint64_t gid, uint64_t mask) {
	if(node->inode > maxInodes) return 0;
	if(inodeTable[node->inode] == NULL) return 0;
	if(inodeTable[node->inode]->isFile == true) return 0;

	RAMFSObject *prevDirectoryEntry = inodeTable[node->inode]->firstObject;

	if(prevDirectoryEntry == NULL) {
		inodeTable[node->inode]->firstObject = (RAMFSObject*)malloc(sizeof(RAMFSObject));
		inodeTable[node->inode]->firstObject->magic = 0;
		strcpy(inodeTable[node->inode]->firstObject->name, name);
		inodeTable[node->inode]->firstObject->length = 0;
		inodeTable[node->inode]->firstObject->isFile = true;
		inodeTable[node->inode]->firstObject->fileData = NULL;
		inodeTable[node->inode]->firstObject->node = (FSNode*)malloc(sizeof(FSNode));
		strcpy(inodeTable[node->inode]->firstObject->node->name, name);
		inodeTable[node->inode]->firstObject->node->mask = mask;
		inodeTable[node->inode]->firstObject->node->uid = uid;
		inodeTable[node->inode]->firstObject->node->gid = gid;
		inodeTable[node->inode]->firstObject->node->size = inodeTable[node->inode]->firstObject->node->impl = 0;
		inodeTable[node->inode]->firstObject->node->flags = VFS_NODE_FILE;
		inodeTable[node->inode]->firstObject->node->inode = currentInode;
		inodeTable[node->inode]->firstObject->nextObject = NULL;
		inodeTable[currentInode++] = inodeTable[node->inode]->firstObject;

		return currentInode-1;
	} else {
		RAMFSObject *newDirectoryEntry;

		newDirectoryEntry = (RAMFSObject*)malloc(sizeof(RAMFSObject));
		newDirectoryEntry->magic = 0;
		strcpy(newDirectoryEntry->name, name);
		newDirectoryEntry->length = 0;
		newDirectoryEntry->isFile = true;
		inodeTable[node->inode]->firstObject->fileData = NULL;
		newDirectoryEntry->node = (FSNode*)malloc(sizeof(FSNode));
		strcpy(newDirectoryEntry->node->name, name);
		newDirectoryEntry->node->mask = mask;
		newDirectoryEntry->node->uid = uid;
		newDirectoryEntry->node->gid = gid;
		newDirectoryEntry->node->size = newDirectoryEntry->node->impl = 0;
		newDirectoryEntry->node->flags = VFS_NODE_FILE;
		newDirectoryEntry->node->inode = currentInode;
		newDirectoryEntry->nextObject = prevDirectoryEntry;

		inodeTable[currentInode++] = newDirectoryEntry;
		inodeTable[node->inode]->firstObject = newDirectoryEntry;

		return currentInode-1;
	}
}

FSNode *RAMFSDriver::FSFindDir(FSNode *node, const char *name) {
	if(node->inode > maxInodes) return 0;
	if(inodeTable[node->inode] == NULL) return 0;
	if(inodeTable[node->inode]->firstObject == NULL) return 0;
	if(inodeTable[node->inode]->isFile == true) return 0;

	RAMFSObject *directoryEntry = inodeTable[node->inode]->firstObject;
	FSNode *entry = (FSNode*)malloc(sizeof(FSNode)); // Remember to deallocate this!

	while (directoryEntry != NULL) {
		if (strcmp(directoryEntry->name, name) == 0) {
			strcpy(entry->name, directoryEntry->name);
			entry->mask = directoryEntry->node->mask;
			entry->uid = directoryEntry->node->uid;
			entry->gid = directoryEntry->node->gid;
			entry->flags = directoryEntry->node->flags;
			entry->inode = directoryEntry->node->inode;
			entry->size = directoryEntry->node->size;
			entry->impl = directoryEntry->node->impl;

			return entry;
		}
		directoryEntry = directoryEntry->nextObject;
	}

	free(entry);

	return 0;
}

uint64_t RAMFSDriver::FSGetDirElements(FSNode *node) {
	if(node->inode > maxInodes) return 0;
	if(inodeTable[node->inode] == NULL) return 0;
	if(inodeTable[node->inode]->firstObject == NULL) return 0;
	if(inodeTable[node->inode]->isFile == true) return 0;

	RAMFSObject *directoryEntry = inodeTable[node->inode]->firstObject;

	uint64_t elements = 0;

	while (directoryEntry != NULL) {
		elements++;
		directoryEntry = directoryEntry->nextObject;
	}

	return elements;
}
