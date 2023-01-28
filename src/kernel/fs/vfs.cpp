#include <fs/vfs.h>
#include <sys/printk.h>
#include <stdio.h>
#include <mm/heap.h>

FILE *stdout = NULL;
FILE *stdlog = NULL;

void VFS_Init() {
	stdout = (FILE*)malloc(sizeof(stdout));
	stdlog = (FILE*)malloc(sizeof(stdlog));
	stdout->descriptor = VFS_FILE_STDOUT;
	stdlog->descriptor = VFS_FILE_STDLOG;
}

int VFS_WriteFile(fd_t file, uint8_t *data, size_t size) {
        switch (file) {
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
