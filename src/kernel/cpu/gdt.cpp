#include <cpu/gdt.h>
#include <mm/memory.h>
#include <mm/pageframe.h>

__attribute((aligned(0x1000))) GDT DefaultGDT = {
        {0, 0, 0, 0x00, 0x00, 0}, // Null
        {0, 0, 0, 0x9a, 0xa0, 0}, // Kernel code
        {0, 0, 0, 0x92, 0xa0, 0}, // Kernel data
        {0, 0, 0, 0x00, 0x00, 0}, // Null
        {0, 0, 0, 0x9a, 0xa0, 0}, // User code
        {0, 0, 0, 0x92, 0xa0, 0}, // User data
        {0, 0, 0, 0x00, 0x00, 0}, // TSS
};

tss_entry *tss;
void init_tss() {
	tss = GlobalAllocator.RequestPage();
        DefaultGDT.TSS.limit0 = sizeof(tss_entry);
        DefaultGDT.TSS.base0 = 0;
        DefaultGDT.TSS.base1 = 0;
        DefaultGDT.TSS.AccessByte = 0x89;
        DefaultGDT.TSS.limit1_Flags = 0x00;
        DefaultGDT.TSS.base2 = 0;
        DefaultGDT.TSS.base3 = (uint64_t)tss;
        memset(tss, 0, sizeof(tss));
}

extern "C" void interrupt_stack(void *rsp0) {
	tss->rsp0 = (uint64_t)rsp0;
}

extern "C" int test_user_function() {
	return 69;
}
