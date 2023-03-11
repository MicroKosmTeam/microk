/*
   File: arch/x64/interrupts/idt.cpp
*/

#include <sys/panic.hpp>
#include <arch/x64/interrupts/idt.hpp>

/* Setting the kernel offset in the GDT (5th entry) */
#define GDT_OFFSET_KERNEL_CODE (0x08 * 5)
/* Max number of interrupts in x86_64 is 256 */
#define IDT_MAX_DESCRIPTORS 256

/* Create the IDT, aligned for maximum performance */
__attribute__((aligned(0x10))) IDTEntry idt[IDT_MAX_DESCRIPTORS];
/* Create the IDTR */
IDTR idtr;

/* Function to set a descriptor in the GDT */
static void IDTSetDescriptor(uint8_t vector, void* isr, uint8_t flags) {
	/* Create new descriptor */
	IDTEntry* descriptor = &idt[vector];

	/* Setting parameters */
	descriptor->isrLow = (uint64_t)isr & 0xFFFF;
	descriptor->kernelCs = GDT_OFFSET_KERNEL_CODE;
	descriptor->ist = 0;
	descriptor->attributes = flags;
	descriptor->isrMid = ((uint64_t)isr >> 16) & 0xFFFF;
	descriptor->isrHigh = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
	descriptor->reserved = 0;
}

/* ISR stub table, declared in assembly code */
extern void *isrStubTable[];

namespace x86_64 {
/* Function to initialize the IDT */
void IDTInit() {
	/* Set base and size in the pointer */
	idtr.base = (uintptr_t)&idt[0];
	idtr.limit = (uint16_t)sizeof(IDTEntry) * IDT_MAX_DESCRIPTORS - 1;

	/* Fill in the 32 exception handlers */
	for (uint8_t vector = 0; vector < 32; vector++) {
		IDTSetDescriptor(vector, isrStubTable[vector], 0x8E);
	}

	/* Load the new IDT */
	asm volatile ("lidt %0" : : "m"(idtr));
	/* Set the interrupt flag */
	asm volatile ("sti");
}
}

/* Special page fault hanlder */
extern "C" void pageFaultHandler() {
	PANIC("Page fault!");

	/* Completely hangs the computer */
	while (true) {
		asm volatile ("cli; hlt");
	}
}

/* Stub exception handler */
__attribute__((noreturn))
extern "C" void exceptionHandler() {
	PANIC("Interrupt");

	/* Completely hangs the computer */
	while (true) {
		asm volatile ("cli; hlt");
	}
}
