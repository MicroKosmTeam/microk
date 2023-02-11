#include <fs/vfs.h>
#include <sys/printk.h>
#include <stdio.h>
#include <mm/heap.h>
#include <mm/string.h>
#include <sys/panik.h>
#include <mm/memory.h>
#include <fs/ramfs/ramfs.h>

#define PREFIX "[VFS] "

FILE *stdout = NULL;
FILE *stdin = NULL;
FILE *stderr = NULL;
FILE *stdlog = NULL;

VFilesystem *rootfs;
VFilesystem *sysfs;
VFilesystem *devtmpfs;
VFilesystem *proc;

VFilesystem *drivers[128];
static uint64_t activeVFSs = 0;

FSNode *VFSFindDir(VFilesystem *fs, FSNode *node, const char *name) {
	return fs->driver->FSFindDir(node, name);
}

FSNode *VFSReadDir(VFilesystem *fs, FSNode *node, uint64_t index) {
	return fs->driver->FSReadDir(node, index);
}

uint64_t VFSMakeDir(VFilesystem *fs, FSNode *node, const char *name) {
	return fs->driver->FSMakeDir(node, name);
}

FSNode **VFSListDir(VFilesystem *fs, FSNode *node) {
	FSNode **entries = (FSNode**)malloc(sizeof(uint64_t) * fs->driver->FSGetDirElements(node));

	for (int i = 0;; i++) {
		if ((entries[i] = fs->driver->FSReadDir(node, i)) == NULL) break;
	}

	return entries;
}

VFilesystem *VFSMountFS(FSNode *mountroot, FSDriver *fsdriver) {
	VFilesystem *fs = new VFilesystem;
	fs->driver = fsdriver;
	fs->node = fsdriver->rootNode;
	fs->mountdir = mountroot;
	return fs;
}

void VFS_Init() {
	// Creating IO buffers
	stdout = (FILE*)malloc(sizeof(FILE) + 1);
	stdout->descriptor = VFS_FILE_STDOUT;
	stdout->bufferSize = 1024 * 64; // 64kb
	stdout->buffer = (uint8_t*)malloc(stdout->bufferSize);

	stdin= (FILE*)malloc(sizeof(FILE) + 1);
	stdin->descriptor = VFS_FILE_STDIN;
	stdin->bufferSize = 1024 * 64; // 64kb
	stdin->buffer = (uint8_t*)malloc(stdout->bufferSize);

	stderr = (FILE*)malloc(sizeof(FILE) + 1);
	stderr->descriptor = VFS_FILE_STDERR;
	stderr->bufferSize = 1024 * 64; // 64kb
	stderr->buffer = (uint8_t*)malloc(stdout->bufferSize);

	stdlog = (FILE*)malloc(sizeof(FILE) + 1);
	stdlog->descriptor = VFS_FILE_STDLOG;
	stdlog->bufferSize = 1024 * 64; // 64kb
	stdlog->buffer = (uint8_t*)malloc(stdlog->bufferSize);

	// Creating the ROOT driver
	FSDriver *rootfsDriver = new RAMFSDriver(100000); // A hundred thousand inodes should be fine (that's one hundred thousand files and folders).
	rootfs = VFSMountFS(NULL, rootfsDriver);

	VFSMakeDir(rootfs, rootfs->node, "dev");
	VFSMakeDir(rootfs, rootfs->node, "sys");
	VFSMakeDir(rootfs, rootfs->node, "proc");

	printf("/\n");

	FSNode **entries = VFSListDir(rootfs, rootfs->node);
	for (int i = 0;; i++) {
		FSNode *result = entries[i];
		if (result == NULL) break;
		printf("  /%s\n", result->name, result->inode);
		free(result);
	}
	free(entries);

	FSDriver *sysfsDriver = new RAMFSDriver(100000); // A hundred thousand inodes should be fine (that's one hundred thousand files and folders).
	FSNode *sysdir = VFSFindDir(rootfs, rootfs->node, "sys");
	sysfs = VFSMountFS(sysdir, sysfsDriver);

	printf("/sys\n");
	VFSMakeDir(sysfs, sysfs->node, "block");
	VFSMakeDir(sysfs, sysfs->node, "bus");
	VFSMakeDir(sysfs, sysfs->node, "dev");
	VFSMakeDir(sysfs, sysfs->node, "fs");
	VFSMakeDir(sysfs, sysfs->node, "kernel");
	entries = VFSListDir(sysfs, sysfs->node);
	for (int i = 0;; i++) {
		FSNode *result = entries[i];
		if (result == NULL) break;
		printf("  /sys/%s\n", result->name, result->inode);
		free(result);
	}
	free(entries);
	free(sysdir);

	FSDriver *devtmpfsDriver = new RAMFSDriver(100000); // A hundred thousand inodes should be fine (that's one hundred thousand files and folders).
	FSNode *devdir = VFSFindDir(rootfs, rootfs->node, "dev");
	devtmpfs = VFSMountFS(devdir, devtmpfsDriver);

	printf("/dev\n");
	VFSMakeDir(devtmpfs, devtmpfs->node, "fb");
	entries = VFSListDir(devtmpfs, devtmpfs->node);
	for (int i = 0;; i++) {
		FSNode *result = entries[i];
		if (result == NULL) break;
		printf("  /dev/%s\n", result->name, result->inode);
		free(result);
	}
	free(entries);
	free(devdir);
}

static void std_print_buf(FILE *file, void (*print)(char ch)) {
	for (int i = 0; i < file->bufferPos; i++) {
		(*print)(file->buffer[i]);
	}
}

int VFS_Write(FILE *file, uint8_t *data, size_t size) {
	if (file->bufferPos + 1 >= file->bufferSize) {

	}

        switch (file->descriptor) {
                case VFS_FILE_STDOUT: {
                        char ch = data[0];
			switch (ch) {
				case '\f':
					std_print_buf(stdout, &GOP_print_char);
					memset(file->buffer, 0, file->bufferSize);
					file->bufferPos = 0;
					break;
				case '\n':
					file->buffer[file->bufferPos++] = ch;
					std_print_buf(stdout, &GOP_print_char);
					memset(file->buffer, 0, file->bufferSize);
					file->bufferPos = 0;
					break;
				default:
					file->buffer[file->bufferPos++] = ch;
					break;
			}
			}
                        return 0;
                case VFS_FILE_STDIN: {
                        char ch = data[0];
			switch (ch) {
				case '\f': // Free stdin
					memset(file->buffer, 0, file->bufferSize);
					file->bufferPos = 0;
					break;
				default:
					file->buffer[file->bufferPos++] = ch;
					break;
			}
			}
                        return 0;
                case VFS_FILE_STDERR:
                        return 0;
                case VFS_FILE_STDLOG: {
                        char ch = data[0];
			switch (ch) {
				case '\f':
					std_print_buf(stdlog, &serial_print_char);
					memset(file->buffer, 0, file->bufferSize);
					file->bufferPos = 0;
					break;
				case '\n':
					file->buffer[file->bufferPos++] = ch;
					std_print_buf(stdlog, &serial_print_char);
					memset(file->buffer, 0, file->bufferSize);
					file->bufferPos = 0;
					break;
				default:
					file->buffer[file->bufferPos++] = ch;
					break;
			}
                        }
                        return 0;
		default:
			return 0;
        }
}

int VFS_Read(FILE *file, uint8_t **buffer, size_t size) {
	switch (file->descriptor) {
                case VFS_FILE_STDOUT:
                        return 1;
                case VFS_FILE_STDIN: {
			/*if (size > file->bufferSize) {
				memcpy(*buffer, file->buffer, file->bufferSize);
				memset(file->buffer, 0, file->bufferSize);
				file->bufferPos = 0;
			} else {
				memcpy(*buffer, file->buffer, size);
				memcpy(file->buffer, file->buffer + size, file->bufferSize - size);
				file->bufferPos = file->bufferSize - size;
			}*/

			// TMP fix
			memcpy(*buffer, file->buffer, size);
			memset(file->buffer, '\0', file->bufferSize);
			file->bufferPos = 0;
			}
                        return 0;
                case VFS_FILE_STDERR:
                        return 1;
                case VFS_FILE_STDLOG:
                        return 1;
		default:
			return 0;

	}
}

FILE *fopen(const char *filename, const char *mode) {

}

FILE *freopen(const char *filename, const char *mode, FILE *stream) {

}

int fclose(FILE *stream) {

}
