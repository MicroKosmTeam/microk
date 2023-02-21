#include <mm/pmm.h>
#include <sys/printk.h>

#define LIMINE_MEMMAP_USABLE                 0
#define LIMINE_MEMMAP_RESERVED               1
#define LIMINE_MEMMAP_ACPI_RECLAIMABLE       2
#define LIMINE_MEMMAP_ACPI_NVS               3
#define LIMINE_MEMMAP_BAD_MEMORY             4
#define LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE 5
#define LIMINE_MEMMAP_KERNEL_AND_MODULES     6
#define LIMINE_MEMMAP_FRAMEBUFFER            7

#define PREFIX "[PMM] "

uint64_t free_memory;
uint64_t reserved_memory;
uint64_t used_memory;
bool initialized = false;

PageFrameAllocator GlobalAllocator;

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

void PageFrameAllocator::ReadMemoryMap(limine_memmap_entry **mMap, size_t mMapEntries) {
        if (initialized) return;

        initialized = true;

        void *largest_free = NULL;
        size_t largest_free_size = 0;

	printk(PREFIX "Physical memory map:\n");
        for (int i = 0; i < mMapEntries; i++){
                limine_memmap_entry *desc = mMap[i];
		printk(PREFIX "[mem 0x%x - 0x%x] %s\n", desc->base, desc->base + desc->length, GetMemoryType(desc->type));
                if (desc->type == LIMINE_MEMMAP_USABLE) {
                        if(desc->length > largest_free_size) {
                                largest_free = desc->base;
                                largest_free_size = desc->length;
                        }
                }
        }


        uint64_t memory_size = get_memory_size(mMap, mMapEntries);
        free_memory = memory_size;
        uint64_t bitmap_size = memory_size / 4096 / 8 + 1;

        // Initialize bitmap
        InitBitmap(bitmap_size, largest_free);

        // Reserve all pages
        ReservePages(0, memory_size / 4096 + 1);

        // Unreserve usable pages (we do it because the mmap can have holes in it)
        for (int i = 0; i < mMapEntries; i++){
                limine_memmap_entry *desc = mMap[i];
                if (desc->type == LIMINE_MEMMAP_USABLE) {
                        UnreservePages(desc->base, desc->length / 4096);
                }
        }

        // Locking the memory below 1MB
        ReservePages(0, 0x100);

        // Locking the page bitmap
        ReservePages(page_bitmap.buffer, page_bitmap.size / 4096 + 1);
}

void PageFrameAllocator::InitBitmap(size_t bitmap_size, void *buffer_address) {
        page_bitmap.size = bitmap_size;
        page_bitmap.buffer = (uint8_t*)buffer_address;
        for (int i = 0; i < bitmap_size; i++) {
                *(uint8_t*)(page_bitmap.buffer + i) = 0;
        }
}

uint64_t page_bitmap_index = 0; // Last page searched
void *PageFrameAllocator::RequestPage() {
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

void *PageFrameAllocator::RequestPages(size_t pages) {
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

void PageFrameAllocator::FreePage(void *address) {
        uint64_t index = (uint64_t)address / 4096;
        if(page_bitmap[index] == false) return;
        if(page_bitmap.set(index, false)) {
                free_memory += 4096;
                used_memory -= 4096;

                if(page_bitmap_index > index) {
                        page_bitmap_index = index; // Making sure that we don't miss free pages
                }
        }
}

void PageFrameAllocator::LockPage(void *address) {
        uint64_t index = (uint64_t)address / 4096;
        if(page_bitmap[index] == true) return;
        if(page_bitmap.set(index, true)) {
                free_memory -= 4096;
                used_memory += 4096;
        }
}

void PageFrameAllocator::UnreservePage(void *address) {
        uint64_t index = (uint64_t)address / 4096;
        if(page_bitmap[index] == false) return;
        if(page_bitmap.set(index, false)) {
                free_memory += 4096;
                reserved_memory -= 4096;
                if(page_bitmap_index > index) {
                        page_bitmap_index = index; // Making sure that we don't miss free pages
                }
        }
}

void PageFrameAllocator::ReservePage(void *address) {
        uint64_t index = (uint64_t)address / 4096;
        if(page_bitmap[index] == true) return;
        if(page_bitmap.set(index, true)) {
                free_memory -= 4096;
                reserved_memory += 4096;
        }
}

void PageFrameAllocator::FreePages(void *address, uint64_t page_count) {
        for (int i = 0; i < page_count; i++) {
                FreePage((void*)(uint64_t)address + (i * 4096));
        }
}

void PageFrameAllocator::LockPages(void *address, uint64_t page_count) {
        for (int i = 0; i < page_count; i++) {
                LockPage((void*)(uint64_t)address + (i * 4096));
        }
}

void PageFrameAllocator::UnreservePages(void *address, uint64_t page_count) {
        for (int i = 0; i < page_count; i++) {
                UnreservePage((void*)(uint64_t)address + (i * 4096));
        }
	printk(PREFIX "Change: [mem 0x%x - 0x%x] %s -> %s\n", address, address + page_count * 0x1000, GetMemoryType(LIMINE_MEMMAP_RESERVED), GetMemoryType(LIMINE_MEMMAP_USABLE));
}

void PageFrameAllocator::ReservePages(void *address, uint64_t page_count) {
        for (int i = 0; i < page_count; i++) {
                ReservePage((void*)(uint64_t)address + (i * 4096));
        }

	printk(PREFIX "Change: [mem 0x%x - 0x%x] %s -> %s\n", address, address + page_count * 0x1000, GetMemoryType(LIMINE_MEMMAP_USABLE), GetMemoryType(LIMINE_MEMMAP_RESERVED));
}

uint64_t PageFrameAllocator::GetFreeMem() {
        return free_memory;
}

uint64_t PageFrameAllocator::GetUsedMem() {
        return used_memory;
}

uint64_t PageFrameAllocator::GetReservedMem() {
        return reserved_memory;
}
