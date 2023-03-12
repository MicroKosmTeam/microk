#include <sys/ustar/ustar.hpp>
#include <sys/printk.hpp>
#include <mm/memory.hpp>
#include <mm/heap.hpp>
#include <mm/string.hpp>
#include <sys/panic.hpp>

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

#include <sys/elf.hpp>

namespace USTAR {
        void LoadArchive(uint8_t *archive, FSNode *node) {
		if(node == NULL) return;

                unsigned char *ptr = archive;

		TarFile *tarFile = new TarFile;
                while (!memcmp(ptr + 257, "ustar", 5)) { // Until we have a valid header
                        int filesize = oct2bin(ptr + 0x7c, 11);

			// Should also add check for directories
			// Also, must add mode, gid, uid, ecc
			// Converting them from octal
			memcpy(tarFile->filename, ptr, 100);
                        tarFile->size = filesize;
                        tarFile->data = new uint8_t[tarFile->size];
                        memcpy(tarFile->data, ptr + 512, filesize);

			if (memcmp(tarFile->filename, "module/", 7) == 0 && tarFile-> size > 0) {
				LoadELF(tarFile->data);
			}

/*
			FSNode *fileNode = VFS::MakeFile(node, tarFile->filename, 0, 0, 0);
			if (fileNode == NULL) PANIC("Could not create file from initrd");
			FILE *file = VFS::OpenFile(fileNode);
			if (file == NULL) PANIC("Could not open file from initrd");

			VFS::WriteFile(file, 0, tarFile->size, tarFile->data);

			VFS::CloseFile(file);
*/

			delete[] tarFile->data;

                        ptr += (((filesize + 511) / 512) + 1) * 512;
                }

		delete tarFile;
        }
}
