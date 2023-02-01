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
	uint64_t prev_tss; // The previous TSS - with hardware task switching these form a kind of backward linked list.
	uint64_t rsp0;     // The stack pointer to load when changing to kernel mode.
	uint64_t ss0;      // The stack segment to load when changing to kernel mode.
}__attribute__((packed));

extern GDT DefaultGDT;

extern "C" void LoadGDT(GDTDescriptor *gdtDescriptor);

void init_tss();

