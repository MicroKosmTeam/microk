#include <fs/vfs.hpp>
#include <sys/printk.hpp>
#include <mm/memory.hpp>
#include <fs/ramfs/ramfs.hpp>

VFilesystem *rootfs;
VFilesystem *devtmpfs;
VFilesystem *proc;

namespace VFS {
VFilesystem *VFSMountFS(FSNode *mountroot, FSDriver *fsdriver) {
	if (mountroot != NULL) PRINTK::PrintK("Mounting fs in %s.\r\n", mountroot->name);
	VFilesystem *fs = new VFilesystem;
	fs->node = fsdriver->rootNode;
	fs->node->driver = fsdriver;
	fs->mountdir = mountroot;
	return fs;
}

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
	PRINTK::PrintK("Starting the VFS.\r\n");

	FSDriver *rootfsDriver = new RAMFSDriver(NULL, 10000);
	rootfs = VFSMountFS(NULL, rootfsDriver);
	FSNode *devdir = rootfs->node->driver->FSMakeDir(rootfs->node, "dev", 0, 0, 0);
	FSNode *procdir = rootfs->node->driver->FSMakeDir(rootfs->node, "proc", 0, 0, 0);
	FSDriver *devtmpfsDriver = new RAMFSDriver(devdir, 10000);
	FSDriver *procDriver = new RAMFSDriver(procdir, 10000);
	devtmpfs = VFSMountFS(devdir, devtmpfsDriver);
	devtmpfs->node->driver->FSMakeDir(devtmpfs->node, "tty", 0, 0, 0);
	proc = VFSMountFS(procdir, procDriver);
	proc->node->driver->FSMakeDir(proc->node, "1", 0, 0, 0);

	ListDir(rootfs->node);
	ListDir(devtmpfs->node);

	PRINTK::PrintK("The VFS has been initialized.\r\n");
}
}
