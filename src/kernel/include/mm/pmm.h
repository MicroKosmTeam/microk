#pragma once
#include <limine.h>
#include <stdint.h>
#include <stddef.h>
#include <mm/bitmap.h>
#include <mm/memory.h>

namespace PMM {
	void InitPageFrameAllocator(limine_memmap_entry **mMap, size_t mMapEntries);

	void *RequestPage();
	void *RequestPages(size_t pages);

	void LockPage(void *address);
	void LockPages(void *address, uint64_t page_count);

	void FreePage(void *address);
	void FreePages(void *address, uint64_t page_count);

	uint64_t GetFreeMem();
	uint64_t GetUsedMem();
	uint64_t GetReservedMem();
}

