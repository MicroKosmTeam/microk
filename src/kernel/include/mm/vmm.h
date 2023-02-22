#pragma once
#include <cdefs.h>

#ifdef CONFIG_ARCH_x86_64
#include <arch/x86_64/mm/vmm.h>
#endif

namespace VMM {
	void InitVirtualMemoryManager(limine_memmap_entry **mMap, uint64_t mMapEntries, uint64_t offset);
	void MapMemory(void *virtualMemory, void *physicalMemory);
}

