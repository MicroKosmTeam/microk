#include <sys/ustar/ustar.hpp>
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
        void LoadArchive(uint8_t *archive, FSNode *node) {
		if(node == NULL) return;

                unsigned char *ptr = archive;

		TarFile *file = new TarFile;
                while (!memcmp(ptr + 257, "ustar", 5)) { // Until we have a valid header
                        int filesize = oct2bin(ptr + 0x7c, 11);

			// Should also add check for directories
			// Also, must add mode, gid, uid, ecc
			// Converting them from octal
			memcpy(file->filename, ptr, 100);
                        file->size = filesize;
                        file->data = new uint8_t[file->size];
                        memcpy(file->data, ptr + 512, filesize);

			node->driver->FSMakeFile(node, file->filename, 0, 0, 0);
			// Load file into fs and then close it
			// TODO: add vfs functions
			//FILE *file =

			delete[] file->data;

                        ptr += (((filesize + 511) / 512) + 1) * 512;
                }

		delete file;
        }
}
