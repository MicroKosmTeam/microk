#pragma once
#include <stdint.h>
#include <mm/efimem.h>
#include <dev/fb/gop.h>
#include <dev/acpi/acpi.h>

extern uint64_t kernel_start, kernel_end;

struct BootInfo {
	Framebuffer* framebuffer;
	PSF1_FONT* psf1_Font;
	EFI_MEMORY_DESCRIPTOR* mMap;
	uint64_t mMapSize;
	uint64_t mMapDescSize;
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

