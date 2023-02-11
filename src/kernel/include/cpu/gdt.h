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
	uint32_t r1;
	uint64_t rsp0;
	uint64_t rsp1;
	uint64_t rsp2;
	uint64_t r2;
	uint64_t ist1;
	uint64_t ist2;
	uint64_t ist3;
	uint64_t ist4;
	uint64_t ist5;
	uint64_t ist6;
	uint64_t ist7;
	uint64_t r3;
	uint16_t r4;
	uint16_t io_mba;
}__attribute__((packed));

extern GDT DefaultGDT;

extern "C" void LoadGDT(GDTDescriptor *gdtDescriptor);

void init_tss();

extern "C" void interrupt_stack(void *rsp0);
extern "C" void jump_usermode();
extern "C" int test_user_function();

