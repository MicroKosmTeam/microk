#pragma once
#include <stdint.h>
#include <stddef.h>
#include <fs/vfs.hpp>

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
		uint64_t size;
		uint64_t mode;
		uint64_t uid;
		uint64_t gid;
		uint8_t type;
                uint8_t *data;
        };

        void LoadArchive(uint8_t *archive, FSNode *node);
}
