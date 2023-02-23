#pragma once
#include <limine.h>
#include <stdint.h>
#include <stddef.h>
#include <arch/x86_64/mm/paging.h>
#include <init/kutil.h>

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

#define PTE_ADDR_MASK 0x000ffffffffff000
#define PTE_GET_ADDR(VALUE) ((VALUE) & PTE_ADDR_MASK)
#define PTE_GET_FLAGS(VALUE) ((VALUE) & ~PTE_ADDR_MASK)

namespace VMM {
	void InitVirtualMemoryManager(BootInfo *bootInfo);
	void MapMemory(void *virtualMemory, void *physicalMemory);
}

