#include <arch/x64/interrupts/interrupts.hpp>
#include <mm/memory.hpp>
#include <sys/printk.hpp>
#include <sys/panic.hpp>

#define IDT_INTERRUPT 0xE
#define IDT_DPL0 0x0
#define IDT_PRESENT 0x80

#define NUM_INTERRUPTS 256

struct IDT {
	uint16_t baseLow;
	uint16_t cs;
	uint8_t ist;
	uint8_t flags;
	uint16_t baseMedium;
	uint32_t baseHigh;
	uint32_t reserved0;
}__attribute__((packed)) idt[NUM_INTERRUPTS];

struct {
	uint16_t length;
	struct IDT *address;
} __attribute__((packed)) idtr;

extern uintptr_t isrTable[];
IntHandler intHandlers[NUM_INTERRUPTS];

static void idtSetGate(uint32_t num, uintptr_t vector, uint16_t cs, uint8_t ist, uint8_t flags) {
	idt[num].baseLow = vector & 0xFFFF;
	idt[num].baseMedium = (vector >> 16) & 0xFFFF;
	idt[num].baseHigh = (vector >> 32) & 0xFFFFFFFF;
	idt[num].cs = cs;
	idt[num].ist = ist;
	idt[num].flags = flags;
}

extern "C" intHandler(Registers *reg) {
	if(intHandlers[reg->intNo]) { return intHandlers[reg->intNo](reg); }

	PANIC("Unhandled interrupt occurred");

	asm volatile ("cli");
	while (true) {
		asm volatile("hlt");
	}
}

extern "C" void loadIDT(void *idtr);
namespace x86_64 {
void InterruptInit() {
	memset(idt, 0, sizeof(idt));
	memset(intHandlers, 0, sizeof(intHandlers));

	for(int i=0; i < NUM_INTERRUPTS; i++) {
		idtSetGate(i, isrTable[i], 0x8, 0, IDT_PRESENT | IDT_DPL0 | IDT_INTERRUPT);
	}

	idtr.address = idt;
	idtr.length = sizeof(idt)-1;
	PRINTK::PrintK("IDT initialized.\r\n");
	loadIDT(&idtr);
}

IntHandler BindInterrupt(uint32_t num, IntHandler fn) {
	IntHandler old = intHandlers[num];
	intHandlers[num] = fn;
	return old;
}
}
