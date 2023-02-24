#pragma once
#include <stdint.h>
#include <limine.h>
#include <mm/pmm.h>
#include <dev/acpi/acpi.h>

extern uint64_t kernel_start, kernel_end;
extern uintptr_t text_start_addr,
		  rodata_start_addr,
		  data_start_addr,
		  text_end_addr,
		  rodata_end_addr,
		  data_end_addr;
extern uintptr_t kernelStack;

struct BootInfo {
	limine_framebuffer **framebuffers;
	uint64_t framebufferCount;

	limine_memmap_entry **mMap;
	uint64_t mMapEntries;
	uint64_t hhdmOffset;
	uint64_t physicalKernelOffset;
	uint64_t virtualKernelOffset;

	limine_file **modules;
	uint64_t moduleCount;

	ACPI::RSDP2 *rsdp;

	bool smp;
	limine_smp_info **cpus;
	uint64_t cpuCount;
	uint32_t bspLapicID;

	char *cmdline;
};

void startKernel(BootInfo *bootInfo);
void smpInit(limine_smp_info *info);
void restInit();

