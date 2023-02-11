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

VFilesystem *drives[128];
static uint64_t activeVFSs = 0;

FSNode *VFSFindDir(VFilesystem *fs, FSNode *node, const char *name) {
	return fs->driver->FSFindDir(node, name);
}

FSNode *VFSReadDir(VFilesystem *fs, FSNode *node, uint64_t index) {
	return fs->driver->FSReadDir(node, index);
}

uint64_t VFSMakeDir(VFilesystem *fs, FSNode *node, const char *name, uint64_t mask, uint64_t uid, uint64_t gid) {
	return fs->driver->FSMakeDir(node, name, mask, uid, gid);
}

uint64_t VFSMakeFile(VFilesystem *fs, FSNode *node, const char *name, uint64_t mask, uint64_t uid, uint64_t gid) {
	return fs->driver->FSMakeFile(node, name, mask, uid, gid);
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
	stdout = new FILE;
	stdout->descriptor = VFS_FILE_STDOUT;
	stdout->bufferSize = 1024 * 64; // 64kb
	stdout->buffer = (uint8_t*)malloc(stdout->bufferSize);

	stdin = new FILE;
	stdin->descriptor = VFS_FILE_STDIN;
	stdin->bufferSize = 1024 * 64; // 64kb
	stdin->buffer = (uint8_t*)malloc(stdout->bufferSize);

	stderr = new FILE;
	stderr->descriptor = VFS_FILE_STDERR;
	stderr->bufferSize = 1024 * 64; // 64kb
	stderr->buffer = (uint8_t*)malloc(stdout->bufferSize);

	stdlog = new FILE;
	stdlog->descriptor = VFS_FILE_STDLOG;
	stdlog->bufferSize = 1024 * 64; // 64kb
	stdlog->buffer = (uint8_t*)malloc(stdlog->bufferSize);

	// Creating the ROOT driver
	FSDriver *rootfsDriver = new RAMFSDriver(100000); // A hundred thousand inodes should be fine (that's one hundred thousand files and folders).
	rootfs = VFSMountFS(NULL, rootfsDriver);

	VFSMakeDir(rootfs, rootfs->node, "dev", 0, 0, 0);
	VFSMakeDir(rootfs, rootfs->node, "sys", 0, 0, 0);
	VFSMakeDir(rootfs, rootfs->node, "proc", 0, 0, 0);

	FSDriver *devtmpfsDriver = new RAMFSDriver(10000);
	FSNode *devdir = VFSFindDir(rootfs, rootfs->node, "dev");
	devtmpfs = VFSMountFS(devdir, devtmpfsDriver);

	VFSMakeDir(devtmpfs, devtmpfs->node, "fb", 0, 0, 0);
	VFSMakeFile(devtmpfs, devtmpfs->node, "tty0", 0, 0, 0);

	FSDriver *sysfsDriver = new RAMFSDriver(10000);
	FSNode *sysdir = VFSFindDir(rootfs, rootfs->node, "sys");
	sysfs = VFSMountFS(sysdir, sysfsDriver);

	VFSMakeDir(sysfs, sysfs->node, "block", 0, 0, 0);
	VFSMakeDir(sysfs, sysfs->node, "bus", 0, 0, 0);
	VFSMakeDir(sysfs, sysfs->node, "dev", 0, 0, 0);
	VFSMakeDir(sysfs, sysfs->node, "fs", 0, 0, 0);
	VFSMakeDir(sysfs, sysfs->node, "kernel", 0, 0, 0);
	VFSMakeFile(sysfs, VFSFindDir(sysfs, sysfs->node, "block"), "test", 0, 0, 0);

	drives[activeVFSs++] = rootfs;
	drives[activeVFSs++] = sysfs;
	drives[activeVFSs++] = devtmpfs;
	drives[activeVFSs] = NULL;
}

#include <mm/string.h>
void VFS_LS(char *path) {
	VFilesystem *fs = rootfs;
	FSNode *directory = rootfs->node;

        if (path[0] == '/' && strlen(path) == 1) {
		FSNode **entries = VFSListDir(fs, directory);
		for (int i = 0; i < fs->driver->FSGetDirElements(directory); i++) {
			FSNode *result = entries[i];
			if (result == NULL) break;
			printf("%d %d %d %s\n", result->mask, result->uid, result->gid, result->name);
			free(result);
		}
		free(entries);

		return;
	}

	printf("  %s\n", path);

	char *dirName = path;
	dirName = strtok(dirName, "/");

	do {
		directory = VFSFindDir(fs, directory, dirName);

		if (directory == NULL) return;

		for (int i = 0; i < activeVFSs; i++) {
			if(drives[i]->mountdir->impl == directory->impl &&
			   drives[i]->mountdir->inode == directory->inode &&
			   strcmp(drives[i]->mountdir->name, directory->name) == 0) {
			// This would be the right way, as there could be some files with the same inode, the same name and the same implementation number
			// But I will fix this later
			//if(drives[i]->mountdir == NULL || directory == NULL) continue;
			//if(drives[i]->mountdir == directory) {
				fs = drives[i];
				directory = fs->node;
				break;
			}
		}
	} while ((dirName = strtok(NULL, "/")) != NULL);

	int elements = fs->driver->FSGetDirElements(directory);
	if(elements <= 0) return;

	FSNode **entries = VFSListDir(fs, directory);
	for (int i = 0; i < elements; i++) {
		FSNode *result = entries[i];
		if (result == NULL) break;
			printf("%d %d %d %s\n", result->mask, result->uid, result->gid, result->name);
		free(result);
	}
	free(entries);
}

void VFS_Mkdir(char *path, char *name) {
	VFilesystem *fs = rootfs;
	FSNode *directory = rootfs->node;

        if (path[0] == '/' && strlen(path) == 1) {
		VFSMakeDir(fs, directory, name, 0, 0, 0);
		return;
	}

	printf("  %s\n", path);

	char *dirName = path;
	dirName = strtok(dirName, "/");

	do {
		directory = VFSFindDir(fs, directory, dirName);

		if (directory == NULL) return;

		for (int i = 0; i < activeVFSs; i++) {
			if(drives[i]->mountdir->impl == directory->impl &&
			   drives[i]->mountdir->inode == directory->inode &&
			   strcmp(drives[i]->mountdir->name, directory->name) == 0) {
			// This would be the right way, as there could be some files with the same inode, the same name and the same implementation number
			// But I will fix this later
			//if(drives[i]->mountdir == NULL || directory == NULL) continue;
			//if(drives[i]->mountdir == directory) {
				fs = drives[i];
				directory = fs->node;
				break;
			}
		}
	} while ((dirName = strtok(NULL, "/")) != NULL);

	VFSMakeDir(fs, directory, name, 0, 0, 0);
}

void VFS_Touch(char *path, char *name) {
	VFilesystem *fs = rootfs;
	FSNode *directory = rootfs->node;

        if (path[0] == '/' && strlen(path) == 1) {
		VFSMakeFile(fs, directory, name, 0, 0, 0);
		return;
	}

	printf("  %s\n", path);

	char *dirName = path;
	dirName = strtok(dirName, "/");

	do {
		directory = VFSFindDir(fs, directory, dirName);

		if (directory == NULL) return;

		for (int i = 0; i < activeVFSs; i++) {
			if(drives[i]->mountdir->impl == directory->impl &&
			   drives[i]->mountdir->inode == directory->inode &&
			   strcmp(drives[i]->mountdir->name, directory->name) == 0) {
			// This would be the right way, as there could be some files with the same inode, the same name and the same implementation number
			// But I will fix this later
			//if(drives[i]->mountdir == NULL || directory == NULL) continue;
			//if(drives[i]->mountdir == directory) {
				fs = drives[i];
				directory = fs->node;
				break;
			}
		}
	} while ((dirName = strtok(NULL, "/")) != NULL);

	VFSMakeFile(fs, directory, name, 0, 0, 0);
}

void VFS_Print(VFilesystem *fs) {
	printf("  /%s\n", fs->mountdir->name == NULL ? "" : fs->mountdir->name);

	FSNode **entries = VFSListDir(fs, fs->node);
	for (int i = 0; i < fs->driver->FSGetDirElements(fs->node); i++) {
		FSNode *result = entries[i];
		if (result == NULL) break;
		printf("  |--> %s\n", result->name);
		free(result);
	}
	free(entries);
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
