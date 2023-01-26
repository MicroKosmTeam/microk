#include <demo.h>
#include <fs/ustar/ustar.h>
#include <sys/printk.h>
#include <mm/memory.h>
#include <mm/heap.h>
#include <dev/timer/pit/pit.h>

namespace DEMO {

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
                for (int i = 0; i<30; i++) printk("\n");
        #endif

        for (int i = 0; i<files; i++) {
                size_t size;
                USTAR::GetFileSize(filenames[i], &size);
                uint8_t *buffer = (uint8_t*)malloc(size);
                memset(buffer, 0, size);
                if(USTAR::ReadFile(filenames[i], &buffer, size)) {
                        print_image(buffer);
                }

                //PIT::Sleep(50);

                free(buffer);
        }
}
}
