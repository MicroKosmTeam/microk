#include <mm/pmm.hpp>
#include <sys/printk.hpp>
#include <limine.h>

#define LIMINE_MEMMAP_USABLE		 0
#define LIMINE_MEMMAP_RESERVED	       1
#define LIMINE_MEMMAP_ACPI_RECLAIMABLE       2
#define LIMINE_MEMMAP_ACPI_NVS	       3
#define LIMINE_MEMMAP_BAD_MEMORY	     4
#define LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE 5
#define LIMINE_MEMMAP_KERNEL_AND_MODULES     6
#define LIMINE_MEMMAP_FRAMEBUFFER	    7

static uint64_t free_memory;
static uint64_t reserved_memory;
static uint64_t used_memory;
static bool initialized = false;
static Bitmap page_bitmap;
static uint64_t page_bitmap_index = 0; // Last page searched

static void InitBitmap(size_t bitmap_size, void *buffer_address);
static void UnreservePage(void *address);
static void UnreservePages(void *address, uint64_t page_count);
static void ReservePage(void *address);
static void ReservePages(void *address, uint64_t page_count);

static char *GetMemoryType(int type) {
	switch (type) {
		case LIMINE_MEMMAP_USABLE:
			return "Usable";
		case LIMINE_MEMMAP_RESERVED:
			return "Reserved";
		case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
			return "ACPI reclaimable";
		case LIMINE_MEMMAP_ACPI_NVS:
			return "ACPI NVS";
		case LIMINE_MEMMAP_BAD_MEMORY:
			return "Bad";
		case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
			return "Bootloader reclaimable";
		case LIMINE_MEMMAP_KERNEL_AND_MODULES:
			return "Kernel and modules";
		case LIMINE_MEMMAP_FRAMEBUFFER:
			return "Framebuffer";
	}
}

static void InitBitmap(size_t bitmap_size, void *buffer_address) {
	page_bitmap.size = bitmap_size;
	page_bitmap.buffer = (uint8_t*)buffer_address;
	for (int i = 0; i < bitmap_size; i++) {
		*(uint8_t*)(page_bitmap.buffer + i) = 0;
	}
}
static void UnreservePage(void *address) {
	uint64_t index = (uint64_t)address / 4096;
	if(page_bitmap[index] == false) return;
	if(page_bitmap.Set(index, false)) {
		free_memory += 4096;
		reserved_memory -= 4096;
		if(page_bitmap_index > index) {
			page_bitmap_index = index; // Making sure that we don't miss free pages
		}
	}
}

static void ReservePage(void *address) {
	uint64_t index = (uint64_t)address / 4096;
	if(page_bitmap[index] == true) return;
	if(page_bitmap.Set(index, true)) {
		free_memory -= 4096;
		reserved_memory += 4096;
	}
}
static void UnreservePages(void *address, uint64_t page_count) {
	for (int i = 0; i < page_count; i++) {
		UnreservePage((void*)(uint64_t)address + (i * 4096));
	}
}

static void ReservePages(void *address, uint64_t page_count) {
	for (int i = 0; i < page_count; i++) {
		ReservePage((void*)(uint64_t)address + (i * 4096));
	}
}


namespace PMM {
void InitPageFrameAllocator(KInfo *info) {
	if (initialized) return;

	initialized = true;

	void *largestFree = NULL;
	size_t largestFreeSize = 0;

	uint64_t memory_size = 0;
	for (int i = 0; i < info->mMapEntryCount; i++) {
		limine_memmap_entry *desc = info->mMap[i];
		if (desc->type == LIMINE_MEMMAP_USABLE) {
			if(desc->length > largestFreeSize) {
				largestFree = (void*)desc->base;
				largestFreeSize = desc->length;
			}
		}

		if (desc->type != LIMINE_MEMMAP_RESERVED) memory_size += desc->length;
	}

	uint64_t bitmap_size = memory_size / 4096 / 8 + 1;

	free_memory = memory_size;

	// Initialize bitmap
	InitBitmap(bitmap_size, largestFree);

	// Reserve all pages
	ReservePages(0, memory_size / 4096 + 1);

	// Unreserve usable pages (we do it because the mmap can have holes in it)
	for (int i = 0; i < info->mMapEntryCount; i++){
		limine_memmap_entry *desc = info->mMap[i];
		if (desc->type == LIMINE_MEMMAP_USABLE) {
			UnreservePages((void*)desc->base, desc->length / 4096);

		}
	}

	// Locking the memory below 1MB
	ReservePages(0, 0x100);

	// Locking the page bitmap
	ReservePages(page_bitmap.buffer, page_bitmap.size / 4096 + 1);

	PRINTK::PrintK("Bitmap allocator started: %dMb free out of %dMb\r\n", free_memory / 1024 / 1024, memory_size / 1024 / 1024);
}

void *RequestPage() {
	for (; page_bitmap_index < page_bitmap.size * 8; page_bitmap_index++) {
		if(page_bitmap[page_bitmap_index] == true) continue;
		LockPage((void*)(page_bitmap_index * 4096));

		return (void*)(page_bitmap_index * 4096);
	}

	// Try again (because RequestPages might leave holes)
	page_bitmap_index = 0;
	for (; page_bitmap_index < page_bitmap.size * 8; page_bitmap_index++) {
		if(page_bitmap[page_bitmap_index] == true) continue;
		LockPage((void*)(page_bitmap_index * 4096));

		return (void*)(page_bitmap_index * 4096);
	}

	// No more memory
	return NULL;
}

void *RequestPages(size_t pages) {
	uint64_t old_page_index = page_bitmap_index;
	for (; page_bitmap_index < (page_bitmap.size - pages)* 8; page_bitmap_index++) {
		bool free = true;
		for (size_t i; i < pages; i++) {
			if(page_bitmap[page_bitmap_index + i]) { free = false; break; };
		}

		if (free) {
			LockPages((void*)(page_bitmap_index * 4096), pages);
			return (void*)(page_bitmap_index * 4096);
		}

	}

	page_bitmap_index = old_page_index;

	// No more memory
	return NULL;
}

void FreePage(void *address) {
	uint64_t index = (uint64_t)address / 4096;
	if(page_bitmap[index] == false) return;
	if(page_bitmap.Set(index, false)) {
		free_memory += 4096;
		used_memory -= 4096;

		if(page_bitmap_index > index) {
			page_bitmap_index = index; // Making sure that we don't miss free pages
		}
	}
}

void LockPage(void *address) {
	uint64_t index = (uint64_t)address / 4096;
	if(page_bitmap[index] == true) return;
	if(page_bitmap.Set(index, true)) {
		free_memory -= 4096;
		used_memory += 4096;
	}
}



void FreePages(void *address, uint64_t page_count) {
	for (int i = 0; i < page_count; i++) {
		FreePage((void*)(uint64_t)address + (i * 4096));
	}
}

void LockPages(void *address, uint64_t page_count) {
	for (int i = 0; i < page_count; i++) {
		LockPage((void*)(uint64_t)address + (i * 4096));
	}
}

uint64_t GetFreeMem() {
	return free_memory;
}

uint64_t GetUsedMem() {
	return used_memory;
}

uint64_t GetReservedMem() {
	return reserved_memory;
}
}
