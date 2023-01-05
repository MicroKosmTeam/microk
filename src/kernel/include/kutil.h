#pragma once
#include <stdint.h>
#include <mm/efimem.h>
#include <dev/fb/gop.h>
#include <sys/printk.h>
#include <sys/cstr.h>
#include <mm/memory.h>
#include <mm/pageframe.h>
#include <mm/bitmap.h>
#include <mm/pageindexer.h>
#include <mm/pagetable.h>
#include <mm/paging.h>
#include <mm/heap.h>
#include <io/io.h>
#include <cpu/gdt.h>
#include <cpu/interrupts/idt.h>
#include <cpu/interrupts/interrupts.h>
#include <dev/8259/pic.h>
#include <dev/tty/gpm/gpm.h>
#include <dev/acpi/acpi.h>
#include <dev/pci/pci.h>

extern uint64_t kernel_start, kernel_end;

struct BootInfo {
	Framebuffer* framebuffer;
	PSF1_FONT* psf1_Font;
	EFI_MEMORY_DESCRIPTOR* mMap;
	uint64_t mMapSize;
	uint64_t mMapDescSize;
        ACPI::RSDP2 *rsdp;
};

struct KernelInfo {
        uint64_t kernel_size;
};

KernelInfo kinit(BootInfo *bootInfo);
