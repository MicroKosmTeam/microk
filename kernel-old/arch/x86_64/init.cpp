#include <arch/x86_64/init.h>
#include <arch/x86_64/cpu.h>
#include <mm/memory.h>
#include <dev/8259/pic.h>
#include <dev/tty/gpm/gpm.h>
#include <arch/x86_64/gdt.h>
#include <arch/x86_64/interrupts/idt.h>
#include <arch/x86_64/interrupts/interrupts.h>
#include <io/io.h>
#include <mm/pageframe.h>
#include <mm/bitmap.h>
#include <mm/pageindexer.h>
#include <mm/pagetable.h>
#include <mm/paging.h>
#include <mm/heap.h>
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

void SetupPaging(BootInfo *bootInfo) {
        // New page table
        PageTable *PML4 = (PageTable*)GlobalAllocator.RequestPage();
        memset(PML4, 0, 0x1000);
        PageTableManager newPageTableManager = PageTableManager(PML4);
        GlobalPageTableManager = newPageTableManager;

        // Identity mapping
        uint64_t memSize = get_memory_size(bootInfo->mMap, bootInfo->mMapEntries);
        for (uint64_t i = 0; i < memSize; i+= 0x1000) {
                GlobalPageTableManager.MapMemory((void*)i, (void*)i);
        }

        // Loading the page table
        asm ("mov %0, %%cr3" : : "r" (PML4));
	return;
}

void PrepareInterrupts(BootInfo *bootInfo) {
        // Preparing interrupts
        idtr.Limit = 0x0FFF;
        idtr.Offset = (uint64_t)GlobalAllocator.RequestPage();

        // Insert interrupts
        SetIDTGate((void*)PageFault_handler, 0xE, IDT_TA_InterruptGate, 0x08);
        SetIDTGate((void*)DoubleFault_handler, 0x8, IDT_TA_InterruptGate, 0x08);
        SetIDTGate((void*)GPFault_handler, 0xD, IDT_TA_InterruptGate, 0x08);
        SetIDTGate((void*)KeyboardInt_handler, 0x21, IDT_TA_InterruptGate, 0x08);
        SetIDTGate((void*)MouseInt_handler, 0x2C, IDT_TA_InterruptGate, 0x08);
        SetIDTGate((void*)PITInt_handler, 0x20, IDT_TA_InterruptGate, 0x08);

        // Load the IDT
        asm("lidt %0" : : "m" (idtr));

        // Starting the mouse
        InitPS2Mouse();

        // Setting up the PIC for basic early input and the PIT
        RemapPIC();
        outb(PIC1_DATA, 0b11111000);
        outb(PIC2_DATA, 0b11101111);

        // Enabling maskable interrupts
        asm("sti");
}
}
