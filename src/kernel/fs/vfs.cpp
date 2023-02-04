#include <fs/vfs.h>
#include <sys/printk.h>
#include <stdio.h>
#include <mm/heap.h>
#include <mm/string.h>
#include <sys/panik.h>
#include <mm/memory.h>

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
FILE *stdin = NULL;
FILE *stderr = NULL;
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
