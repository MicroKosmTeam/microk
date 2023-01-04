#pragma once
#include <mm/paging.h>


class PageTableManager {
public:
        PageTableManager(PageTable *PML4Address);
        PageTable *PML4;
        void MapMemory(void *virtual_memory, void *physical_memory);
};

extern PageTableManager GlobalPageTableManager;
