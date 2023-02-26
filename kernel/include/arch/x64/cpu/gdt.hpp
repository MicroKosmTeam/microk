#pragma once
#include <stddef.h>
#include <stdint.h>

struct GDTPointer{
    uint16_t size;
    uint64_t offset;
} __attribute__((packed));

struct GDTEntry{
    uint16_t limit;
    uint16_t base_low16;
    uint8_t base_mid8;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high8;
} __attribute__((packed));

struct TSSEntry {
    uint16_t length;
    uint16_t base_low16;
    uint8_t base_mid8;
    uint8_t flags0;
    uint8_t flags1;
    uint8_t base_high8;
    uint32_t base_upper32;
    uint32_t reserved;
} __attribute__((packed));

struct TSS {
    uint32_t reserved0;
    uint64_t rsp[3];
    uint64_t reserved1;
    uint64_t ist[7];
    uint32_t reserved2;
    uint32_t reserved3;
    uint16_t reserved4;
    uint16_t iopb_offset;
} __attribute__((packed));

struct GDT {
    GDTEntry null;
    GDTEntry _16bit_code;
    GDTEntry _16bit_data;
    GDTEntry _32bit_code;
    GDTEntry _32bit_data;
    GDTEntry _64bit_code;
    GDTEntry _64bit_data;
    GDTEntry user_data;
    GDTEntry user_code;
    TSSEntry tss;
} __attribute__((packed));

extern "C" void FlushGDT(GDTPointer *pointer);
extern "C" void FlushTSS();

namespace x86_64 {
	void LoadGDT(uintptr_t stackPointer);
}
