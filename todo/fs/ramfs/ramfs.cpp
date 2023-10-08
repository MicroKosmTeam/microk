#include <fs/ramfs/ramfs.hpp>
#include <mm/memory.hpp>
#include <mm/string.hpp>

void RAMFSDriver::FSInit(FSNode *mountpoint) {
	inodeTable = (RAMFSObject**)Malloc(sizeof(uint64_t) * maxInodes);
	currentInode = 0;

	for(int i = 0; i < maxInodes; i++) {
		inodeTable[i] = NULL;
	}

	rootFile = (RAMFSObject*)Malloc(sizeof(RAMFSObject));
	rootFile->magic = 0;
	if (mountpoint == NULL) strcpy(rootFile->name, "ramfs");
	else strcpy(rootFile->name, mountpoint->name);
	rootFile->length = 0;
	rootFile->isFile = false;
	rootFile->firstObject = NULL;
	rootNode = (FSNode*)Malloc(sizeof(FSNode));
	rootFile->node = rootNode;
	if (mountpoint == NULL) strcpy(rootFile->node->name, "ramfs");
	else strcpy(rootFile->node->name, mountpoint->name);
	rootFile->node->mask = rootFile->node->uid = rootFile->node->gid = rootFile->node->size = rootFile->node->impl = 0;
	rootFile->node->flags = VFS_NODE_DIRECTORY;
	rootFile->node->inode = currentInode;

	rootFile->nextObject = NULL;

	inodeTable[currentInode++] = rootFile;

	inodeTable[currentInode] = NULL;

}

void RAMFSDriver::FSDelete() {

}

uint64_t RAMFSDriver::FSReadFile(FILE *file, uint64_t offset, size_t size, uint8_t **buffer) {
	if(file->node == NULL) return 0;
	if(file->node->inode > maxInodes) return 0;
	if(inodeTable[file->node->inode] == NULL) return 0;
	if(inodeTable[file->node->inode]->isFile == false) return 0;

	if(offset > inodeTable[file->node->inode]->length) return 0;
	if(offset + size > inodeTable[file->node->inode]->length) size = inodeTable[file->node->inode]->length - offset;

	memcpy(*buffer + offset, inodeTable[file->node->inode]->fileData + offset, size);
	return size;
}

uint64_t RAMFSDriver::FSWriteFile(FILE *file, uint64_t offset, size_t size, uint8_t *buffer) {
	if(file->node->inode > maxInodes) return 0;
	if(inodeTable[file->node->inode] == NULL) return 0;
	if(inodeTable[file->node->inode]->isFile == false) return 0;

	if(offset + size > inodeTable[file->node->inode]->length) {
		if (inodeTable[file->node->inode]->length == 0) {
			inodeTable[file->node->inode]->fileData = new uint8_t[offset + size];
			memset(inodeTable[file->node->inode]->fileData, 0, inodeTable[file->node->inode]->length);

		} else {
			uint8_t *tempBuffer = new uint8_t[inodeTable[file->node->inode]->length];
			memcpy(tempBuffer, inodeTable[file->node->inode]->fileData, inodeTable[file->node->inode]->length);

			delete[] inodeTable[file->node->inode]->fileData;
			inodeTable[file->node->inode]->fileData = new uint8_t[offset+size];
			memset(inodeTable[file->node->inode]->fileData, 0, offset+size);

			memcpy(inodeTable[file->node->inode]->fileData, tempBuffer, inodeTable[file->node->inode]->length);
			//inodeTable[file->node->inode]->length = offset + size;

			delete[] tempBuffer;
		}
	}

	uint64_t length = (offset - inodeTable[file->node->inode]->length) > 0 ?
		(offset - inodeTable[file->node->inode]->length) :
		0;
	length += size;
	file->node->size = inodeTable[file->node->inode]->length = length;
	memcpy(inodeTable[file->node->inode]->fileData + offset, buffer, size);

	file->bufferPos = offset + size;

	return file->bufferPos;
}

FILE *RAMFSDriver::FSOpenFile(FSNode *node, uint64_t descriptor) {
	FILE *file = new FILE;
	file->node = node;
	file->descriptor = descriptor;
	return file;
}

void RAMFSDriver::FSCloseFile(FILE *file) {
	delete file;
}

uint64_t RAMFSDriver::FSDeleteFile(FSNode *node) {

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

	FSNode *entry = (FSNode*)Malloc(sizeof(FSNode)); // Remember to deallocate this!

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

FSNode *RAMFSDriver::FSMakeDir(FSNode *node, const char *name, uint64_t uid, uint64_t gid, uint64_t mask) {
	if(node->inode > maxInodes) return 0;
	if(inodeTable[node->inode] == NULL) return 0;
	if(inodeTable[node->inode]->isFile == true) return 0;

	RAMFSObject *prevDirectoryEntry = inodeTable[node->inode]->firstObject;

	if(prevDirectoryEntry == NULL) {
		inodeTable[node->inode]->firstObject = (RAMFSObject*)Malloc(sizeof(RAMFSObject));
		inodeTable[node->inode]->firstObject->magic = 0;
		strcpy(inodeTable[node->inode]->firstObject->name, name);
		inodeTable[node->inode]->firstObject->length = 0;
		inodeTable[node->inode]->firstObject->isFile = false;
		inodeTable[node->inode]->firstObject->firstObject = NULL;
		inodeTable[node->inode]->firstObject->node = (FSNode*)Malloc(sizeof(FSNode));
		strcpy(inodeTable[node->inode]->firstObject->node->name, name);
		inodeTable[node->inode]->firstObject->node->mask = mask;
		inodeTable[node->inode]->firstObject->node->uid = uid;
		inodeTable[node->inode]->firstObject->node->gid = gid;
		inodeTable[node->inode]->firstObject->node->size = inodeTable[node->inode]->firstObject->node->impl = 0;
		inodeTable[node->inode]->firstObject->node->flags = VFS_NODE_DIRECTORY;
		inodeTable[node->inode]->firstObject->node->inode = currentInode;
		inodeTable[node->inode]->firstObject->nextObject = NULL;
		inodeTable[currentInode++] = inodeTable[node->inode]->firstObject;

		return inodeTable[currentInode-1]->node;
	} else {
		RAMFSObject *newDirectoryEntry;

		newDirectoryEntry = (RAMFSObject*)Malloc(sizeof(RAMFSObject));
		newDirectoryEntry->magic = 0;
		strcpy(newDirectoryEntry->name, name);
		newDirectoryEntry->length = 0;
		newDirectoryEntry->isFile = false;
		newDirectoryEntry->firstObject = NULL;
		newDirectoryEntry->node = (FSNode*)Malloc(sizeof(FSNode));
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

		return inodeTable[currentInode-1]->node;
	}
}

FSNode *RAMFSDriver::FSMakeFile(FSNode *node, const char *name, uint64_t uid, uint64_t gid, uint64_t mask) {
	if(node->inode > maxInodes) return 0;
	if(inodeTable[node->inode] == NULL) return 0;
	if(inodeTable[node->inode]->isFile == true) return 0;

	RAMFSObject *prevDirectoryEntry = inodeTable[node->inode]->firstObject;

	if(prevDirectoryEntry == NULL) {
		inodeTable[node->inode]->firstObject = (RAMFSObject*)Malloc(sizeof(RAMFSObject));
		inodeTable[node->inode]->firstObject->magic = 0;
		strcpy(inodeTable[node->inode]->firstObject->name, name);
		inodeTable[node->inode]->firstObject->length = 0;
		inodeTable[node->inode]->firstObject->isFile = true;
		inodeTable[node->inode]->firstObject->fileData = NULL;
		inodeTable[node->inode]->firstObject->node = (FSNode*)Malloc(sizeof(FSNode));
		strcpy(inodeTable[node->inode]->firstObject->node->name, name);
		inodeTable[node->inode]->firstObject->node->driver = this;
		inodeTable[node->inode]->firstObject->node->mask = mask;
		inodeTable[node->inode]->firstObject->node->uid = uid;
		inodeTable[node->inode]->firstObject->node->gid = gid;
		inodeTable[node->inode]->firstObject->node->size = inodeTable[node->inode]->firstObject->node->impl = 0;
		inodeTable[node->inode]->firstObject->node->flags = VFS_NODE_FILE;
		inodeTable[node->inode]->firstObject->node->inode = currentInode;
		inodeTable[node->inode]->firstObject->nextObject = NULL;
		inodeTable[currentInode++] = inodeTable[node->inode]->firstObject;

		return inodeTable[currentInode-1]->node;
	} else {
		RAMFSObject *newDirectoryEntry;

		newDirectoryEntry = (RAMFSObject*)Malloc(sizeof(RAMFSObject));
		newDirectoryEntry->magic = 0;
		strcpy(newDirectoryEntry->name, name);
		newDirectoryEntry->length = 0;
		newDirectoryEntry->isFile = true;
		inodeTable[node->inode]->firstObject->fileData = NULL;
		newDirectoryEntry->node = (FSNode*)Malloc(sizeof(FSNode));
		strcpy(newDirectoryEntry->node->name, name);
		newDirectoryEntry->node->mask = mask;
		newDirectoryEntry->node->driver = this;
		newDirectoryEntry->node->uid = uid;
		newDirectoryEntry->node->gid = gid;
		newDirectoryEntry->node->size = newDirectoryEntry->node->impl = 0;
		newDirectoryEntry->node->flags = VFS_NODE_FILE;
		newDirectoryEntry->node->inode = currentInode;
		newDirectoryEntry->nextObject = prevDirectoryEntry;

		inodeTable[currentInode++] = newDirectoryEntry;
		inodeTable[node->inode]->firstObject = newDirectoryEntry;

		return inodeTable[currentInode-1]->node;
	}
}

FSNode *RAMFSDriver::FSFindDir(FSNode *node, const char *name) {
	// This function is really sketchy. I will have to rewrite it.
	if(node->inode > maxInodes) return 0;
	if(inodeTable[node->inode] == NULL) return 0;
	if(inodeTable[node->inode]->firstObject == NULL) return 0;
	if(inodeTable[node->inode]->isFile == true) return 0;

	RAMFSObject *directoryEntry = inodeTable[node->inode]->firstObject;
	FSNode *entry = new FSNode; // Remember to deallocate this!

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

	delete entry;

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

uint64_t RAMFSDriver::FSDeleteDir(FSNode *node) {

}
