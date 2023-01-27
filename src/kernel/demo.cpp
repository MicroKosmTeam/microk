#include <demo.h>
#include <stdio.h>
#include <fs/ustar/ustar.h>
#include <sys/printk.h>
#include <mm/memory.h>
#include <mm/heap.h>
#include <dev/timer/pit/pit.h>

namespace DEMO {
void Stress() {
        uint32_t malloc_count = 100;
        uint32_t memcpy_count = 100;
        uint32_t memset_count = 100;
        uint32_t memcmp_count = 100;
        uint8_t total_sizes = 14;
        uint32_t sizes[] = {
                512,
                1024,
                2048,
                4096,
                8192,
                16384,
                32768,
                65536,
                131072,
                262144,
                524288,
                1048576,
                2097152,
                4194304
        };

        printf("1. Malloc and free stressing.\n");
        printf("Starting %d mallocs of sizes from 512 to 4M.\n", malloc_count);
        for (int i = 0; i < total_sizes; i++) {
                printf(" -> Size: %d ", sizes[i]);
                for (int j = 0; j < malloc_count; j++) {
                        uint8_t *data = (uint8_t*)malloc(sizes[i]);
                        memset(data, 0, sizes[i]);
                        free(data);
                }
                printf("  Done.\n");
        }

        printf("2. Memcpy stressing.\n");
        printf("Starting %d memcpys of sizes from 512 to 4M.\n", memcpy_count);
        for (int i = 0; i < total_sizes; i++) {
                printf(" -> Size: %d ", sizes[i]);
                for (int j = 0; j < memcpy_count; j++) {
                        uint8_t *src = (uint8_t*)malloc(sizes[i]);
                        uint8_t *dest = (uint8_t*)malloc(sizes[i]);
                        memcpy(dest, src, sizes[i]);
                        free(src);
                        free(dest);
                }
                printf("  Done.\n");
        }

        printf("2. Memset stressing.\n");
        printf("Starting %d memsets of sizes from 512 to 4M.\n", memset_count);
        for (int i = 0; i < total_sizes; i++) {
                uint8_t *data = (uint8_t*)malloc(sizes[i]);
                printf(" -> Size: %d ", sizes[i]);
                for (int j = 0; j < memset_count; j++) {
                        memset(data, 0, sizes[i]);
                }
                printf("  Done.\n");
                free(data);
        }

        printf("3. Memcmp stressing.\n");
        printf("Starting %d memcmps of sizes from 512 to 4M.\n", memcmp_count);
        for (int i = 0; i < total_sizes; i++) {
                uint8_t *src = (uint8_t*)malloc(sizes[i]);
                uint8_t *dest = (uint8_t*)malloc(sizes[i]);
                memset(src, 0, sizes[i]);
                memset(dest, 0, sizes[i]);
                printf(" -> Size: %d ", sizes[i]);
                for (int j = 0; j < memcmp_count; j++) {
                        memcmp(dest, src, sizes[i]);
                }
                printf("  Done.\n");
                free(src);
                free(dest);
        }
}

void Doom() {
}

void BadApple() {
        uint16_t files = 30;
        char *filenames[] = {
                "001.ppm\0",
                "002.ppm\0",
                "003.ppm\0",
                "004.ppm\0",
                "005.ppm\0",
                "006.ppm\0",
                "007.ppm\0",
                "008.ppm\0",
                "009.ppm\0",
                "010.ppm\0",
                "011.ppm\0",
                "012.ppm\0",
                "013.ppm\0",
                "014.ppm\0",
                "015.ppm\0",
                "016.ppm\0",
                "017.ppm\0",
                "018.ppm\0",
                "019.ppm\0",
                "020.ppm\0",
                "021.ppm\0",
                "022.ppm\0",
                "023.ppm\0",
                "024.ppm\0",
                "025.ppm\0",
                "026.ppm\0",
                "027.ppm\0",
                "028.ppm\0",
                "029.ppm\0",
                "030.ppm\0",
        };

        #ifdef KCONSOLE_GOP
                GlobalRenderer.print_clear();
                for (int i = 0; i<30; i++) printf("\n");
        #endif

        for (int i = 0; i<files; i++) {
                size_t size;
                USTAR::GetFileSize(filenames[i], &size);
                uint8_t *buffer = (uint8_t*)malloc(size);
                memset(buffer, 0, size);
                if(USTAR::ReadFile(filenames[i], &buffer, size)) {
                        print_image(buffer);
                }

                free(buffer);
        }
}
}
