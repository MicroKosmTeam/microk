/*
   File: arch/x64/cpu/gdt.cpp
*/

#include <mm/memory.hpp>
#include <arch/x64/cpu/gdt.hpp>

/* This is the GDT. It must be aligned, otherwise it won't work.
   This follows the recomended GDT by Limine */
__attribute__((aligned(0x1000))) GDT gdt = {
	{0, 0, 0, 0, 0, 0}, /* Required null gdt segment */
	{0xffff, 0, 0, 0x9a, 0x80, 0}, /* 16-bit code */
	{0xffff, 0, 0, 0x92, 0x80, 0}, /* 16-bit data */
	{0xffff, 0, 0, 0x9a, 0xcf, 0}, /* 32-bit code */
	{0xffff, 0, 0, 0x92, 0xcf, 0}, /* 32-bit data */
	{0, 0, 0, 0x9a, 0xa2, 0}, /* 64-bit code */
	{0, 0, 0, 0x92, 0xa0, 0}, /* 64-bit data */
	{0, 0, 0, 0xF2, 0, 0}, /* User data */
	{0, 0, 0, 0xFA, 0x20, 0}, /* User code */
	{0x68, 0, 0, 0x89, 0x20, 0, 0, 0} /* TSS */
};

/* Aligned GDT Pointer */
__attribute__((aligned(0x1000))) GDTPointer gdtPointer;
/* The TSS must be aligned */
__attribute__((aligned(0x1000))) TSS tss;

namespace x86_64 {
/*
   Function that initializes the TSS given the current kernel stack
*/
void TSSInit(uintptr_t stackPointer) {
	/* Cleaning the TSS struct */
	memset(&tss, 0, sizeof(tss));

	/* Initializing the stack pointer */
	tss.rsp[0] = stackPointer;
	tss.ist[1] = 0;
	/* Giving TSS size */
	tss.iopb_offset = sizeof(tss);

	/* Setting TSS parameters in the GDT */
	gdt.tss.base_low16 = (uintptr_t)&tss & 0xFFFF;
	gdt.tss.base_mid8 = ((uintptr_t)&tss >> 16) & 0xFF;
	gdt.tss.base_high8 = ((uintptr_t)&tss >> 24) & 0xFF;
	gdt.tss.base_upper32 = (uintptr_t)&tss >> 32;
}

/*
   Function that loads the GDT to the CPU
*/
void LoadGDT(uintptr_t stackPointer) {
	/* Initialization of the TSS */
	TSSInit(stackPointer);

	/* Setting GDT pointer size and offset */
	gdtPointer.size = sizeof(gdt) - 1;
	gdtPointer.offset = (uint64_t)&gdt;

	/* Call to asm functions that load the GDT and TSS */
	FlushGDT(&gdtPointer);
	FlushTSS();
}
}
