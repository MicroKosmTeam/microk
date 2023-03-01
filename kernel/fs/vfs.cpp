#include <fs/vfs.hpp>
#include <sys/printk.hpp>
#include <mm/memory.hpp>
#include <fs/ramfs/ramfs.hpp>

VFilesystem *rootfs;

namespace VFS {
VFilesystem *VFSMountFS(FSNode *mountroot, FSDriver *fsdriver) {
	if (mountroot != NULL) PRINTK::PrintK("Mounting fs in %s.\r\n", mountroot->name);
	VFilesystem *fs = new VFilesystem;
	fs->node = fsdriver->rootNode;
	fs->node->driver = fsdriver;
	fs->mountdir = mountroot;
	return fs;
}

void Init(KInfo *info) {
	PRINTK::PrintK("Starting the VFS.\r\n");

	FSDriver *rootfsDriver = new RAMFSDriver(10000);
	rootfs = VFSMountFS(NULL, rootfsDriver);

	PRINTK::PrintK("The VFS has been initialized.\r\n");
}
}
