#include <fs/vfs.hpp>
#include <sys/printk.hpp>
#include <mm/memory.hpp>
#include <fs/ramfs/ramfs.hpp>
#include <mm/string.hpp>

VFilesystem *rootfs;
VFilesystem *sysfs;
VFilesystem *procfs;
VFilesystem *initrdfs;

namespace VFS {
void ListDir(FSNode *dir) {
	if (dir == NULL) return;
	uint64_t dirElements = dir->driver->FSGetDirElements(dir);
	if (dirElements == 0) return;

	PRINTK::PrintK("Listing %s...\r\n", dir->name);
	for (int i = 0; i < dirElements; i++) {
		FSNode *element = dir->driver->FSReadDir(dir, i);
		PRINTK::PrintK(" - %s\r\n", element->name);
	}
}


void Init(KInfo *info) {
	rootfs = sysfs = procfs = initrdfs = NULL;
	PRINTK::PrintK("Starting the VFS.\r\n");

	FSDriver *rootfsDriver = new RAMFSDriver(NULL, 10000);
	rootfs = MountFS(NULL, rootfsDriver, 0);

	FSNode *sysDir = MakeDir(rootfs->node, "sys", 0, 0, 0);
	FSDriver *sysfsDriver = new RAMFSDriver(sysDir, 10000);
	sysfs = MountFS(sysDir, sysfsDriver, 0);

	FSNode *procDir = MakeDir(rootfs->node, "proc", 0, 0, 0);
	FSDriver *procDriver = new RAMFSDriver(procDir, 10000);
	procfs = MountFS(procDir, procDriver, 0);

	FSNode *initrdDir = MakeDir(rootfs->node, "initrd", 0, 0, 0);
	FSDriver *initrdDriver = new RAMFSDriver(initrdDir, 10000);
	initrdfs = MountFS(initrdDir, initrdDriver, 0);

	PRINTK::PrintK("The VFS has been initialized.\r\n");
}

VFilesystem *GetRootFS() {
	return rootfs;
}

VFilesystem *GetInitrdFS() {
	return initrdfs;
}

FSNode *GetNode(VFilesystem *fs, char *path) {
	FSNode *node = fs->node;

	char *ptr = strtok(path, "/");

	if (ptr == NULL) return node;

	do {
		node = FindDir(node, ptr);

		if (node == NULL) break;
	} while (ptr = strtok(NULL, " "));

	return node;
}

VFilesystem *MountFS(FSNode *mountroot, FSDriver *fsdriver, uint64_t flags) {
	// Remember, mountroot can be null
	if (mountroot == NULL) {
		// Only if the rootfs is free
		if (rootfs != NULL) return NULL;
	}

	if (fsdriver == NULL) return NULL;

	// TODO: Check if already mounted

	VFilesystem *fs = new VFilesystem;
	fs->node = fsdriver->rootNode;
	fs->node->driver = fsdriver;
	fs->mountdir = mountroot;
	fs->flags = flags;

	return fs;
}

uint64_t RemountFS(VFilesystem *fs, uint64_t flags) {
	if (fs == NULL) return 1;

	// TODO: Check for open files and such

	fs->flags = flags;

	// TODO: Refresh the VFS buffers?
	return 0;
}

uint64_t UmountFS(VFilesystem *fs) {
	if (fs == NULL) return 1;

	// TODO: Check for open files and such
	fs->node->driver->FSDelete();

	// The driver automatically destroys the node
	delete fs->node->driver;
	delete fs;
}

FSNode *MakeFile(FSNode *node, const char *name, uint64_t uid, uint64_t gid, uint64_t mask) {
	if (node == NULL) return NULL;
	if (node->driver == NULL) return NULL;

	return node->driver->FSMakeFile(node, name, uid, gid, mask);
}

FILE *OpenFile(FSNode *node) {
	if (node == NULL) return NULL;
	if (node->driver == NULL) return NULL;

	uint64_t descriptor = 0; // TODO: Get file descriptors
	FILE *file = node->driver->FSOpenFile(node, descriptor);

	// TODO: Do some stuff with the file

	return file;
}

uint64_t GetFileSize(FILE *file) {
	if (file == NULL) return NULL;
	if (file->node == NULL) return NULL;
	return file->node->size;
}

uint64_t ReadFile(FILE *file, uint64_t offset, size_t size, uint8_t **buffer) {
	if (file == NULL) return NULL;
	if (file->node == NULL) return NULL;
	if (file->node->driver == NULL) return NULL;

	// TODO: Do some caching or leave it to the FS?

	return file->node->driver->FSReadFile(file, offset, size, buffer);
}

uint64_t WriteFile(FILE *file, uint64_t offset, size_t size, uint8_t *buffer) {
	if (file == NULL) return NULL;
	if (file->node == NULL) return NULL;
	if (file->node->driver == NULL) return NULL;

	// TODO: Do some caching or leave it to the FS?

	return file->node->driver->FSWriteFile(file, offset, size, buffer);
}

void CloseFile(FILE *file) {
	if (file == NULL) return NULL;
	if (file->node == NULL) return NULL;
	if (file->node->driver == NULL) return NULL;

	// TODO: Dismantle eventual remainders in the VFS

	return file->node->driver->FSCloseFile(file);
}

uint64_t DeleteFile(FSNode *node) {
	if (node == NULL) return NULL;
	if (node->driver == NULL) return NULL;

	return node->driver->FSDeleteFile(node);
}

FSNode *MakeDir(FSNode *node, const char *name, uint64_t uid, uint64_t gid, uint64_t mask) {
	if (node == NULL) return NULL;
	if (node->driver == NULL) return NULL;

	return node->driver->FSMakeDir(node, name, uid, gid, mask);
}

FSNode *ReadDir(FSNode *node, uint64_t index) {
	if (node == NULL) return NULL;
	if (node->driver == NULL) return NULL;

	return node->driver->FSReadDir(node, index);
}

FSNode *FindDir(FSNode *node, const char *name) {
	if (node == NULL) return NULL;
	if (node->driver == NULL) return NULL;

	return node->driver->FSFindDir(node, name);
}

uint64_t GetDirElements(FSNode *node) {
	if (node == NULL) return NULL;
	if (node->driver == NULL) return NULL;

	return node->driver->FSGetDirElements(node);
}

uint64_t DeleteDir(FSNode *node) {
	if (node == NULL) return NULL;
	if (node->driver == NULL) return NULL;

	return node->driver->FSDeleteDir(node);
}
}
