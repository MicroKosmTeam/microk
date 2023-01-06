#pragma once
#include <stdint.h>

struct GDTDescriptor {
        uint16_t size;
        uint64_t offset;
}__attribute__((packed));

struct GDTEntry {
        uint16_t limit0;
        uint16_t base0;
        uint8_t base1;
        uint8_t AccessByte;
        uint8_t limit1_Flags;
        uint8_t base2;
}__attribute__((packed));

struct GDTEntryTSS {
        uint16_t limit0;
        uint16_t base0;
        uint8_t base1;
        uint8_t AccessByte;
        uint8_t limit1_Flags;
        uint8_t base2;
         uint64_t base3;
}__attribute__((packed));

struct GDT {
        GDTEntry Null;       // 0x00
        GDTEntry KernelCode; // 0x08
        GDTEntry KernelData; // 0x10
        GDTEntry UserNull;
        GDTEntry UserCode;
        GDTEntry UserData;
        GDTEntryTSS TSS;
}__attribute__((packed)) __attribute__((aligned(0x1000)));

struct tss_entry {
	uint32_t prev_tss; // The previous TSS - with hardware task switching these form a kind of backward linked list.
	uint32_t esp0;     // The stack pointer to load when changing to kernel mode.
	uint32_t ss0;      // The stack segment to load when changing to kernel mode.
	// Everything below here is unused.
	uint32_t esp1; // esp and ss 1 and 2 would be used when switching to rings 1 or 2.
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint16_t trap;
	uint16_t iomap_base;
}__attribute__((packed));

extern GDT DefaultGDT;

extern "C" void LoadGDT(GDTDescriptor *gdtDescriptor);

void init_tss();
