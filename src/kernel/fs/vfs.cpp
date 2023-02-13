#include <fs/vfs.h>
#include <sys/printk.h>
#include <stdio.h>
#include <mm/heap.h>
#include <mm/string.h>
#include <sys/panik.h>
#include <mm/memory.h>
#include <fs/ramfs/ramfs.h>
#include <sys/vector.h>

#define PREFIX "[VFS] "

FILE *stdout = NULL;
FILE *stdin = NULL;
FILE *stderr = NULL;
FILE *stdlog = NULL;

VFilesystem *rootfs;
VFilesystem *sysfs;
VFilesystem *devtmpfs;
VFilesystem *proc;

// TODO: This fails.
//Vector<VFilesystem*> drives;
VFilesystem *drives[128];
static uint64_t activeVFSs = 0;

static uint64_t currFileDescriptor = 128;

FILE *VFSOpen(VFilesystem *fs, FSNode *node, size_t bufferSize) {
	FILE *file = fs->driver->FSOpen(node, currFileDescriptor++);
	if (file == NULL) return NULL;
	file->bufferSize = bufferSize;
	file->buffer = new uint8_t[bufferSize];
	file->fsDescriptor = fs->descriptor;
	return file;
}

void VFSClose(VFilesystem *fs, FILE *file) {
	fs->driver->FSClose(file);
	if(file->bufferSize > 0) delete[] file->buffer;
	file = NULL;
}

uint64_t VFSRead(VFilesystem *fs, FILE *file, uint64_t offset, size_t size, uint8_t **buffer) {
	return fs->driver->FSRead(file, offset, size, buffer);
}

uint64_t VFSWrite(VFilesystem *fs, FILE *file, uint64_t offset, size_t size, uint8_t buffer) {
	return fs->driver->FSWrite(file, offset, size, buffer);
}

FSNode *VFSFindDir(VFilesystem *fs, FSNode *node, const char *name) {
	return fs->driver->FSFindDir(node, name);
}

FSNode *VFSReadDir(VFilesystem *fs, FSNode *node, uint64_t index) {
	return fs->driver->FSReadDir(node, index);
}

// Transfrom this from uint64_t to FSNode*
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
	// TODO: Deal with this later
	//fs->descriptor = activeVFSs++;
	return fs;
}

FILE *VFSMakeNode(VFilesystem *fs, FSNode *node, const char *name, uint64_t type, uint64_t nodeMajor, uint64_t nodeMinor) {
	FILE *file;
	VFSMakeFile(fs, node, name, 0, 0, 0);
	file->node = VFSFindDir(fs, node, name);
	file = VFSOpen(fs, file->node, 64 * 1024);

	file->node->flags |= type;

	// TODO: Major and minors have to wait for the driver redesign.
	// For now...
	// file->descriptor = nodeMinor;

	return file;
}

void VFS_Init() {
	// Creating the ROOT driver
	FSDriver *rootfsDriver = new RAMFSDriver(100000); // A hundred thousand inodes should be fine (that's one hundred thousand files and folders).
	rootfs = VFSMountFS(NULL, rootfsDriver);

	VFSMakeDir(rootfs, rootfs->node, "dev", 0, 0, 0);
	VFSMakeDir(rootfs, rootfs->node, "sys", 0, 0, 0);
	VFSMakeDir(rootfs, rootfs->node, "proc", 0, 0, 0);

	FSDriver *sysfsDriver = new RAMFSDriver(10000);
	FSNode *sysdir = VFSFindDir(rootfs, rootfs->node, "sys");
	sysfs = VFSMountFS(sysdir, sysfsDriver);

	VFSMakeDir(sysfs, sysfs->node, "block", 0, 0, 0);
	VFSMakeDir(sysfs, sysfs->node, "bus", 0, 0, 0);
	VFSMakeDir(sysfs, sysfs->node, "dev", 0, 0, 0);
	VFSMakeDir(sysfs, sysfs->node, "fs", 0, 0, 0);
	VFSMakeDir(sysfs, sysfs->node, "kernel", 0, 0, 0);
	VFSMakeFile(sysfs, sysfs->node, "memory", 0, 0, 0);

	FSDriver *devtmpfsDriver = new RAMFSDriver(10000);
	FSNode *devdir = VFSFindDir(rootfs, rootfs->node, "dev");
	devtmpfs = VFSMountFS(devdir, devtmpfsDriver);

	VFSMakeDir(devtmpfs, devtmpfs->node, "fb", 0, 0, 0);
	VFSMakeFile(devtmpfs, devtmpfs->node, "tty0", 0, 0, 0);

	FSDriver *procDriver = new RAMFSDriver(10000);
	FSNode *procdir = VFSFindDir(rootfs, rootfs->node, "proc");
	proc = VFSMountFS(procdir, procDriver);

	VFSMakeFile(proc, proc->node, "mem", 0, 0, 0);

	drives[activeVFSs++] = rootfs;
	drives[activeVFSs++] = devtmpfs;
	drives[activeVFSs++] = sysfs;
	drives[activeVFSs++] = proc;
	drives[activeVFSs] = NULL;

	// TODO: Actually files don't work
	FSNode *node = VFSFindDir(devtmpfs, devtmpfs->node, "tty0");
	if (node == NULL) printk("File not found.\n");
	else printk("File found.\n");
	printk("Opening.\n");
	FILE *tty = VFSOpen(devtmpfs, node, 1024);
	if (tty == NULL) printk("File failed to open.\n");
	else printk("Done opening.\n");
	uint8_t *buffer = new uint8_t[128];
	memset(buffer, 'A', 128);
	printk("Write.\n");
	VFSWrite(devtmpfs, tty, 0, 128, buffer); // This part fails
	memset(buffer, 'B', 128);
	printk("Read.\n");
	VFSRead(devtmpfs, tty, 0, 128, &buffer); // Or this one
	printk("\nA: %c\n", buffer[0]);
	VFSClose(devtmpfs, tty);

	//while(true);



	/*
	drives.Push(devtmpfs, activeVFSs++);
	drives.Push(sysfs, activeVFSs++);
	drives.Push(rootfs, activeVFSs++);
	*/

	/*
	stdout = VFSMakeNode(devtmpfs, devtmpfs->node, "stdout", VFS_NODE_FIFO, VFS_NODE_MAJOR_STDIO, VFS_NODE_MINOR_STDOUT);
	stdin  = VFSMakeNode(devtmpfs, devtmpfs->node, "stdin" , VFS_NODE_FIFO, VFS_NODE_MAJOR_STDIO, VFS_NODE_MINOR_STDIN);
	stderr = VFSMakeNode(devtmpfs, devtmpfs->node, "stderr", VFS_NODE_FIFO, VFS_NODE_MAJOR_STDIO, VFS_NODE_MINOR_STDERR);
	stdlog = VFSMakeNode(devtmpfs, devtmpfs->node, "stdlog", VFS_NODE_FIFO, VFS_NODE_MAJOR_STDIO, VFS_NODE_MINOR_STDLOG);
	*/

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
			VFilesystem *drive = drives[i]; // drives.Get(i);
			if(drive->mountdir->impl == directory->impl &&
			   drive->mountdir->inode == directory->inode &&
			   strcmp(drive->mountdir->name, directory->name) == 0) {
			// This would be the right way, as there could be some files with the same inode, the same name and the same implementation number
			// But I will fix this later
			//if(drives[i]->mountdir == NULL || directory == NULL) continue;
			//if(drives[i]->mountdir == directory) {
				fs = drive;
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
			VFilesystem *drive = drives[i]; // drives.Get(i);
			if(drive->mountdir->impl == directory->impl &&
			   drive->mountdir->inode == directory->inode &&
			   strcmp(drive->mountdir->name, directory->name) == 0) {
			// This would be the right way, as there could be some files with the same inode, the same name and the same implementation number
			// But I will fix this later
			//if(drives[i]->mountdir == NULL || directory == NULL) continue;
			//if(drives[i]->mountdir == directory) {
				fs = drive;
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
			VFilesystem *drive = drives[i]; // drives.Get(i);
			if(drive->mountdir->impl == directory->impl &&
			   drive->mountdir->inode == directory->inode &&
			   strcmp(drive->mountdir->name, directory->name) == 0) {
			// This would be the right way, as there could be some files with the same inode, the same name and the same implementation number
			// But I will fix this later
			//if(drives[i]->mountdir == NULL || directory == NULL) continue;
			//if(drives[i]->mountdir == directory) {
				fs = drive;
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

// Use the 'descriptor' field to automatically deduce the relevant virtual filesystem.
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
