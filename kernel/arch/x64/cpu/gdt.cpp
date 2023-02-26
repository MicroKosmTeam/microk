#include <arch/x64/cpu/gdt.hpp>
#include <mm/memory.hpp>

__attribute__((aligned(0x1000))) GDT gdt = {
	{0, 0, 0, 0, 0, 0}, // null
	{0xffff, 0, 0, 0x9a, 0x80, 0}, // 16-bit code
	{0xffff, 0, 0, 0x92, 0x80, 0}, // 16-bit data
	{0xffff, 0, 0, 0x9a, 0xcf, 0}, // 32-bit code
	{0xffff, 0, 0, 0x92, 0xcf, 0}, // 32-bit data
	{0, 0, 0, 0x9a, 0xa2, 0}, // 64-bit code
	{0, 0, 0, 0x92, 0xa0, 0}, // 64-bit data
	{0, 0, 0, 0xF2, 0, 0}, // user data
	{0, 0, 0, 0xFA, 0x20, 0}, // user code
	{0x68, 0, 0, 0x89, 0x20, 0, 0, 0} // tss
};

__attribute__((aligned(0x1000))) GDTPointer gdtPointer;
__attribute__((aligned(0x1000))) TSS tss;

namespace x86_64 {
void TSSInit(uintptr_t stackPointer) {
	memset(&tss, 0, sizeof(tss));

	tss.rsp[0] = stackPointer;
	tss.ist[1] = 0;

	tss.iopb_offset = sizeof(tss);

	gdt.tss.base_low16 = (uintptr_t)&tss & 0xFFFF;
	gdt.tss.base_mid8 = ((uintptr_t)&tss >> 16) & 0xFF;
	gdt.tss.base_high8 = ((uintptr_t)&tss >> 24) & 0xFF;
	gdt.tss.base_upper32 = (uintptr_t)&tss >> 32;
}

void LoadGDT(uintptr_t stackPointer) {
	TSSInit(stackPointer);


	gdtPointer.size = sizeof(gdt) - 1;
	gdtPointer.offset = (uint64_t)&gdt;

	FlushGDT(&gdtPointer);
	FlushTSS();
}
}
