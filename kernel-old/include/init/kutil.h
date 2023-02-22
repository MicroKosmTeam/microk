#pragma once
#include <stdint.h>
#include <dev/fb/gop.h>
#include <dev/acpi/acpi.h>
#include <limine.h>

extern uint64_t kernel_start, kernel_end;
extern uintptr_t kernelStack;

struct BootInfo {
	limine_framebuffer **framebuffers;
	uint64_t framebufferCount;
	//Framebuffer* framebuffer;
	//PSF1_FONT* psf1_Font;

	limine_memmap_entry **mMap;
	uint64_t mMapEntries;
	uint64_t hhdmOffset;

	limine_file **modules;
	uint64_t moduleCount;

        ACPI::RSDP2 *rsdp;

	void *initrdData;
	uint64_t initrdSize;

};

struct KernelInfo {
        uint64_t kernelSize;
	void *initrd;
	uint64_t initrdSize;
};

extern KernelInfo kInfo;

void kinit(BootInfo *bootInfo);
void restInit();

