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

uint64_t VFS_ReadNode(FSNode *node, uint64_t offset, uint64_t size, uint8_t **buffer) {
	if (node->read != 0)
		return node->read(node, offset, size, buffer);
	else
		return 0;
}


uint64_t VFS_WriteNode(FSNode *node, uint64_t offset, uint64_t size, uint8_t *buffer) {
	if (node->write != 0)
		return node->write(node, offset, size, buffer);
	else
		return 0;
}
void VFS_OpenNode(FSNode *node, uint8_t read, uint8_t write) {
	if (node->open != 0)
		return node->open(node);
	else
		return 0;
}

void VFS_CloseNode(FSNode *node) {
	if (node->close != 0)
		return node->close(node);
	else
		return 0;
}

DirectoryEntry *VFS_ReadDirNode(FSNode *node, uint64_t index) {
	if (node->read != 0) {
		if ((node->flags&0x7) == VFS_NODE_DIRECTORY && node->readdir != 0 )
			return node->readdir(node, index);
		else
			return 0;
	} else
		return 0;
}

FSNode *VFS_FindDirNode(FSNode *node, char *name) {
	if (node->read != 0) {
		if ((node->flags&0x7) == VFS_NODE_DIRECTORY && node->readdir != 0 )
			return node->finddir(node, name);
		else
			return 0;
	} else
		return 0;
}

void VFS_Init() {
	rootfs->driver = new RAMFSDriver(1000000); // A million inodes should be fine.

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
