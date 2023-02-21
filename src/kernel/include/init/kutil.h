#pragma once
#include <stdint.h>
#include <limine.h>
#include <mm/pmm.h>

extern uint64_t kernel_start, kernel_end;
extern uintptr_t kernelStack;

struct BootInfo {
	limine_framebuffer **framebuffers;
	uint64_t framebufferCount;

	limine_memmap_entry **mMap;
	uint64_t mMapEntries;
	uint64_t hhdmOffset;

	limine_file **modules;
	uint64_t moduleCount;

	void *rsdp;

	bool smp;
	limine_smp_info **cpus;
	uint64_t cpuCount;
	uint32_t bspLapicID;

	char *cmdline;
};

void kinit(BootInfo *bootInfo);
void smpInit(limine_smp_info *info);
void restInit();

