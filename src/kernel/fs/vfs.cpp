#include <fs/vfs.h>
#include <sys/printk.h>
#include <stdio.h>
#include <mm/heap.h>
#include <mm/string.h>
#include <sys/panik.h>

#define VFS_FILE_STDOUT 0x0000
#define VFS_FILE_STDIN  0x0001
#define VFS_FILE_STDERR 0x0002
#define VFS_FILE_STDLOG 0x0003

#define VFS_FLAG_ROOT   0x0001
#define VFS_FLAG_RAMFS  0x0002
#define VFS_FLAG_DEVFS  0x0004
#define VFS_FLAG_SYSFS  0x0008
#define VFS_FLAG_PROC   0x0010

FILE *stdout = NULL;
FILE *stdlog = NULL;

VFilesystem *rootfs = NULL;

void VFS_Tree() {
	printf("Root mounts: %d:\n", rootfs->mountpoints);
	int i = 0;
	while(i < rootfs->mountpoints) {
		printf(" - %s\n", rootfs->up[i]->mountpoint);
		i++;
	}
}

void VFS_Init() {
	if(VFS_Mount(NULL, "/", NULL, VFS_FLAG_RAMFS | VFS_FLAG_ROOT)) {
		PANIK("Failed to mount root");
	} else {
		VFS_Mount(rootfs, "/dev", NULL, VFS_FLAG_RAMFS | VFS_FLAG_DEVFS);
		VFS_Mount(rootfs, "/sys", NULL, VFS_FLAG_RAMFS | VFS_FLAG_SYSFS);
		VFS_Mount(rootfs, "/proc", NULL, VFS_FLAG_RAMFS | VFS_FLAG_PROC);
	}

	stdout = (FILE*)malloc(sizeof(FILE) + 1);
	stdlog = (FILE*)malloc(sizeof(FILE) + 1);
	stdout->descriptor = VFS_FILE_STDOUT;
	stdlog->descriptor = VFS_FILE_STDLOG;
}

int VFS_Mount(VFilesystem *base, char *path, char *drive, uint64_t flags) {
	if(base == NULL && (flags & VFS_FLAG_ROOT)) {
		if (rootfs != NULL) return -1;

		rootfs = (VFilesystem*)malloc(sizeof(VFilesystem) + 1);
		rootfs->mountpoints=0;
		rootfs->mountpoint = (char*)malloc((strlen("/\0") + 1) * sizeof(char));
		strcpy(rootfs->mountpoint, "/\0");

		rootfs->drive= (char*)malloc((strlen(drive) + 1) * sizeof(char));
		strcpy(rootfs->drive, drive); // This has to be overhauled

		rootfs->flags = flags;
		return 0;
	} else if (base != NULL) {
		base->up[base->mountpoints] = (VFilesystem*)malloc(sizeof(VFilesystem) + 1);
		base->up[base->mountpoints]->mountpoints = 0;
		base->up[base->mountpoints]->mountpoint = (char*)malloc((strlen(path) + 1) * sizeof(char));
		strcpy(base->up[base->mountpoints]->mountpoint, path);
		base->up[base->mountpoints]->drive= (char*)malloc((strlen(drive) + 1) * sizeof(char));
		strcpy(base->up[base->mountpoints]->drive, drive); // This has to be overhauled

		base->up[base->mountpoints]->flags = flags;
		base->mountpoints++;

		return 0;
	} else {
		return-1;
	}
}

int VFS_Write(FILE *file, uint8_t *data, size_t size) {
        switch (file->descriptor) {
                case VFS_FILE_STDOUT: {
                        char ch = data[0];
                        GOP_print_char(ch);
                        }
                        return 0;
                case VFS_FILE_STDIN:
                        return 0;
                case VFS_FILE_STDERR:
                        return 0;
                case VFS_FILE_STDLOG: {
                        char ch = data[0];
                        serial_print_char(ch);
                        }
                        return 0;
        }
}

FILE *fopen(const char *filename, const char *mode) {

}

FILE *freopen(const char *filename, const char *mode, FILE *stream) {

}

int fclose(FILE *stream) {

}
