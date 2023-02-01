#include <cpu/gdt.h>
#include <mm/memory.h>

__attribute((aligned(0x1000))) GDT DefaultGDT = {
        {0, 0, 0, 0x00, 0x00, 0}, // Null
        {0, 0, 0, 0x9a, 0xa0, 0}, // Kcode
        {0, 0, 0, 0x92, 0xa0, 0}, // Kdata
        {0, 0, 0, 0x00, 0x00, 0}, // UNull
        {0, 0, 0, 0x9a, 0xa0, 0}, // Ucode
        {0, 0, 0, 0x92, 0xa0, 0}, // Udata
        {0, 0, 0, 0x00, 0x00, 0}, // TSS
};

tss_entry tss;
void init_tss() {
        DefaultGDT.TSS.limit0 = sizeof(tss);
        DefaultGDT.TSS.base0 = 0;
        DefaultGDT.TSS.base1 = 0;
        DefaultGDT.TSS.AccessByte = 0x89;
        DefaultGDT.TSS.limit1_Flags = 0x00;
        DefaultGDT.TSS.base2 = 0;
        DefaultGDT.TSS.base3 = (uint64_t)&tss;
        memset(&tss, 0, sizeof(tss));

//        tss.ss0
//        tss.esp0
}
