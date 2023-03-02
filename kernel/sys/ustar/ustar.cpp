#include <sys/ustar/ustar.hpp>
#include <fs/vfs.hpp>
#include <sys/printk.hpp>
#include <mm/memory.hpp>
#include <mm/heap.hpp>
#include <mm/string.hpp>

#define PREFIX "[USTAR] "

static int oct2bin(unsigned char *str, int size) {
        int n = 0;
        unsigned char *c = str;
        while (size-- > 0) {
                n *= 8;
                n += *c - '0';
                c++;
        }
        return n;
}

namespace USTAR {
        static uint8_t total_files = 0;
        TarFile *firstFile;

        void LoadArchive(uint8_t *archive) {
                unsigned char *ptr = archive;
		TarFile *file = firstFile;

                while (!memcmp(ptr + 257, "ustar", 5)) { // Until we have a valid header
                        int filesize = oct2bin(ptr + 0x7c, 11);

			file = new TarFile;

                        memcpy(file->filename, ptr, 100);
                        file->size = filesize;
                        file->data = (uint8_t*)Malloc(file->size);
                        memcpy(file->data, ptr + 512, filesize);

			PRINTK::PrintK(" %s %d\n", file->filename, file->size);

			file = file->next;

                        ptr += (((filesize + 511) / 512) + 1) * 512;
                }
        }

        void ReadArchive() {
                PRINTK::PrintK("Files:\n");
		TarFile *file = firstFile;
		while(file) {
			PRINTK::PrintK(" %s %d\n", file->filename, file->size);
			file = file->next;
                }
        }

        bool GetFileSize(char *filename, size_t *size) {
		TarFile *file = firstFile;
		while(file) {
                        if(strcmp(filename, file->filename) != 0) {
				file = file->next;
				continue;
			}

                        *size = file->size;
                        return true;
                }

                return false;
        }

        bool ReadFile(char *filename, uint8_t **buffer, size_t size) {
		TarFile *file = firstFile;
		while(file) {
                        if(strcmp(filename, file->filename) != 0) {
				file = file->next;
				continue;
			}

                        PRINTK::PrintK("File found: %s %d\n", file->filename, file->size);

                        memcpy(*buffer, file->data, size > file->size ? file->size : size);

                        return true;
                }

                PRINTK::PrintK("No file named %s found.\n", filename);
                return false;
        }
}
