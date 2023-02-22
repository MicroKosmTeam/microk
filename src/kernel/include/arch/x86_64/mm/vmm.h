#pragma once
#include <limine.h>
#include <stdint.h>
#include <stddef.h>
#include <arch/x86_64/mm/paging.h>

#define PTE_PRESENT (1 << 0)
#define PTE_READ_WRITE (1 << 1)
#define PTE_USER_SUPERVISOR (1 << 2)
#define PTE_WRITE_THROUGH (1 << 3)
#define PTE_CACHE_DISABLED (1 << 4)
#define PTE_ACCESSED (1 << 5)
#define PTE_DIRTY (1 << 6)
#define PTE_PAT (1 << 7)
#define PTE_GLOBAL (1 << 8)
#define PTE_UNAVAILABLE (1 << 63)

namespace VMM {
	void InitVirtualMemoryManager(limine_memmap_entry **mMap, uint64_t mMapEntries, uint64_t offset);
	void MapMemory(void *virtualMemory, void *physicalMemory);
}

