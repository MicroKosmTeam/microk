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
#include<mm/pagetable.h>
#include <mm/paging.h>
#include <io/io.h>
#include <cpu/gdt.h>
#include <cpu/interrupts/idt.h>
#include <cpu/interrupts/interrupts.h>

extern uint64_t kernel_start, kernel_end;

struct BootInfo {
	Framebuffer* framebuffer;
	PSF1_FONT* psf1_Font;
	EFI_MEMORY_DESCRIPTOR* mMap;
	uint64_t mMapSize;
	uint64_t mMapDescSize;
};

struct KernelInfo {
        uint64_t kernel_size;
        PageTableManager *pageTableManager;
};

KernelInfo kinit(BootInfo *bootInfo);
