#include <fs/ustar/ustar.h>
#include <fs/vfs.h>
#include <stdio.h>
#include <mm/memory.h>
#include <mm/heap.h>
#include <mm/string.h>

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
        TarFile files[128];

        void LoadArchive(uint8_t *archive) {
                unsigned char *ptr = archive;

                while (!memcmp(ptr + 257, "ustar", 5)) { // Until we have a valid header
                        int filesize = oct2bin(ptr + 0x7c, 11);

                        TarFile file;
                        memcpy(file.filename, ptr, 100);
                        file.size = filesize;
                        file.data = (uint8_t*)malloc(file.size);
                        memcpy(file.data, ptr + 512, filesize);
//                        file.data = (uint8_t*)ptr + 512;

                        dprintf(PREFIX " %s %d\n", file.filename, file.size); 
                        files[total_files++] = file;
                        ptr += (((filesize + 511) / 512) + 1) * 512; // Next header
                        //ptr += 512 + filesize;
                }
        }

        void ReadArchive() {
                dprintf(PREFIX "Files:\n");
                for (int i = 0; i < total_files; i++) {
                        dprintf(PREFIX " %s %d\n", files[i].filename, files[i].size); 
                }
        }

        bool GetFileSize(char *filename, size_t *size) {
                for (int i = 0; i < total_files; i++) {
                        if(strcmp(filename, files[i].filename) != 0) continue;
                        
                        *size = files[i].size;
                        return true;
                }
                
                return false;
        }

        bool ReadFile(char *filename) {
                for (int i = 0; i < total_files; i++) {
                        if(strcmp(filename, files[i].filename) != 0) continue;
                        
                        dprintf(PREFIX "File found: %s %d\n", files[i].filename, files[i].size); 

                        dprintf(PREFIX "Data:\n");
                        for (int j = 0; j < files[i].size; j++) {
                                dprintf("%c", files[i].data[j]);
                        }
                        dprintf("\n");

                        return true;
                }
                

                dprintf(PREFIX "No file named %s found.\n", filename);
                return false;
        }

        bool ReadFile(char *filename, uint8_t **buffer, size_t size) {
                for (int i = 0; i < total_files; i++) {
                        if(strcmp(filename, files[i].filename) != 0) continue;
                        
                        dprintf(PREFIX "File found: %s %d\n", files[i].filename, files[i].size); 

                        memcpy(*buffer, files[i].data, size > files[i].size ? files[i].size : size);

                        return true;
                }

                dprintf(PREFIX "No file named %s found.\n", filename);
                return false;
        }
}
