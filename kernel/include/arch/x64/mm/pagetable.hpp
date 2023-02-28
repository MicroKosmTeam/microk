#pragma once
#include <arch/x64/mm/paging.hpp>
#include <stdint.h>

class PageTableManager {
public:
        PageTableManager(PageTable *PML4Address);
        PageTable *PML4;
        void MapMemory(void *virtual_memory, void *physical_memory);
	void MapMemory(void* virtualMemory, void* physicalMemory, uint8_t flagNumber, PT_Flag flags[128], bool statuses[128]);
};

extern PageTableManager *GlobalPageTableManager;
