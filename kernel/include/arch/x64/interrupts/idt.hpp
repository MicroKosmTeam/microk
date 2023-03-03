#pragma once
#include <stdint.h>

struct IDTEntry {
	uint16_t    isrLow;      // The lower 16 bits of the ISR's address
	uint16_t    kernelCs;    // The GDT segment selector that the CPU will load into CS before calling the ISR
	uint8_t	    ist;          // The IST in the TSS that the CPU will load into RSP; set to zero for now
	uint8_t     attributes;   // Type and attributes; see the IDT page
	uint16_t    isrMid;      // The higher 16 bits of the lower 32 bits of the ISR's address
	uint32_t    isrHigh;     // The higher 32 bits of the ISR's address
	uint32_t    reserved;     // Set to zero
} __attribute__((packed));

struct IDTR {
	uint16_t	limit;
	uint64_t	base;
} __attribute__((packed));


extern "C" void exceptionHandler();
extern "C" void pageFaultHandler();

namespace x86_64 {
	void IDTInit();
}
