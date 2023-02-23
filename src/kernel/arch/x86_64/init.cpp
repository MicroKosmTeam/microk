#include <arch/x86_64/init.h>
#include <arch/x86_64/io/io.h>
#include <arch/x86_64/mm/vmm.h>
#include <arch/x86_64/cpu/cpu.h>
#include <arch/x86_64/cpu/gdt.h>
#include <arch/x86_64/interrupts/idt.h>
#include <arch/x86_64/interrupts/interrupts.h>
#include <mm/memory.h>
#include <mm/pmm.h>
#include <sys/printk.h>

namespace x86_64 {
IDTR idtr;

void SetIDTGate(void *handler, uint8_t entry_offset, uint8_t type_attr, uint8_t selector) {
        IDTDescEntry *interrupt = (IDTDescEntry*)(idtr.Offset + entry_offset * sizeof(IDTDescEntry));
        interrupt->SetOffset((uint64_t)handler);
        interrupt->type_attr = type_attr;
        interrupt->selector = selector;
}

void LoadGDT() {
	// Loading the GDT
	gdt_init();
	return;
}

void PrepareInterrupts(BootInfo *bootInfo) {
        // Preparing interrupts
        idtr.Limit = 0x0FFF;
        idtr.Offset = (uint64_t)PMM::RequestPage();

        // Insert interrupts
        SetIDTGate((void*)PageFault_handler, 0xE, IDT_TA_InterruptGate, 0x08);
        SetIDTGate((void*)DoubleFault_handler, 0x8, IDT_TA_InterruptGate, 0x08);
        SetIDTGate((void*)GPFault_handler, 0xD, IDT_TA_InterruptGate, 0x08);

        // Load the IDT
        asm("lidt %0" : : "m" (idtr));

        // Enabling maskable interrupts
        asm("sti");
}

void PreparePaging(BootInfo *bootInfo) {
	VMM::InitVirtualMemoryManager(bootInfo);
	return;
}
}
