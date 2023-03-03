#include <arch/x64/interrupts/idt.hpp>

#define GDT_OFFSET_KERNEL_CODE (0x08 * 5) // It's the 5th entry of the GDT
#define IDT_MAX_DESCRIPTORS 256

__attribute__((aligned(0x10)))
IDTEntry idt[256]; // Create an array of IDT entries; aligned for performance
IDTR idtr;

static void IDTSetDescriptor(uint8_t vector, void* isr, uint8_t flags) {
	IDTEntry* descriptor = &idt[vector];

	descriptor->isrLow = (uint64_t)isr & 0xFFFF;
	descriptor->kernelCs = GDT_OFFSET_KERNEL_CODE;
	descriptor->ist = 0;
	descriptor->attributes = flags;
	descriptor->isrMid = ((uint64_t)isr >> 16) & 0xFFFF;
	descriptor->isrHigh = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
	descriptor->reserved = 0;
}

extern void* isr_stub_table[];

namespace x86_64 {
void IDTInit() {
	idtr.base = (uintptr_t)&idt[0];
	idtr.limit = (uint16_t)sizeof(IDTEntry) * IDT_MAX_DESCRIPTORS - 1;

	for (uint8_t vector = 0; vector < 32; vector++) {
		IDTSetDescriptor(vector, isr_stub_table[vector], 0x8E);
		//vectors[vector] = true;
	}

	asm volatile ("lidt %0" : : "m"(idtr)); // load the new IDT
	asm volatile ("sti"); // set the interrupt flag
}
}

#include <sys/panic.hpp>
__attribute__((noreturn))
extern "C" void exceptionHandler() {
	PANIC("Interrupt");

	while (true) {
		asm volatile ("cli; hlt"); // Completely hangs the computer
	}
}
