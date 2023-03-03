#pragma once
#include <stdint.h>
#include <stddef.h>

namespace USTAR {
        struct tar_header {
                char filename[100];
                char mode[8];
                char uid[8];
                char gid[8];
                char size[12];
                char mtime[12];
                char chksum[8];
                char typeflag[1];
        }__attribute__((packed));

        struct TarFile {
                char filename[100];
                uint32_t size;
                uint8_t *data;
        };

        extern TarFile files[128];

        void LoadArchive(uint8_t *archive);
        void ReadArchive();
        bool GetFileSize(char *filename, size_t *size);
        bool ReadFile(char *filename);
        bool ReadFile(char *filename, uint8_t **buffer, size_t size);
}
