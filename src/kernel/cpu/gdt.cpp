#include <cpu/gdt.h>

__attribute((aligned(0x1000))) GDT DefaultGDT = {
        {0, 0, 0, 0x00, 0x00, 0}, // Null
        {0, 0, 0, 0x9a, 0xa0, 0}, // Kcode
        {0, 0, 0, 0x92, 0xa0, 0}, // Kdata
        {0, 0, 0, 0x00, 0x00, 0}, // Unull
        {0, 0, 0, 0x9a, 0xa0, 0}, // Ucode
        {0, 0, 0, 0x92, 0xa0, 0}, // Udata
};
