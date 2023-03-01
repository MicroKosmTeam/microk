#include <fs/vfs.hpp>
#include <sys/printk.hpp>

VFilesystem *rootfs;

namespace VFS {
void Init(KInfo *info) {
	PRINTK::PrintK("Starting the VFS.\r\n");
}
}
