#include <mm/pageframe.h>

uint64_t free_memory;
uint64_t reserved_memory;
uint64_t used_memory;
bool initialized = false;

PageFrameAllocator GlobalAllocator;

void PageFrameAllocator::ReadEFIMemoryMap(EFI_MEMORY_DESCRIPTOR *mMap, size_t mMapSize, size_t mMapDescSize) {
        if (initialized) return;

        initialized = true;

        uint64_t mMapEntries = mMapSize / mMapDescSize;
        void *largest_free = NULL;
        size_t largest_free_size = 0;
        
        for (int i = 0; i < mMapEntries; i++){
                EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)mMap + (i * mMapDescSize));
                if (desc->type == 7) {
                        if( desc->numPages * 4096 > largest_free_size) {
                                largest_free = desc->physAddr;
                                largest_free_size = desc->numPages * 4096;
                        }
                }
        }

        uint64_t memory_size = get_memory_size(mMap, mMapEntries, mMapDescSize);
        free_memory = memory_size;
        uint64_t bitmap_size = memory_size / 4096 / 8 + 1;

        // Initialize bitmap
        InitBitmap(bitmap_size, largest_free);

        // Lock pages of bitmap
        LockPages(page_bitmap.buffer, page_bitmap.size / 4096 + 1);

        // Reserve unusable/reserved pages
        for (int i = 0; i < mMapEntries; i++){
                EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)mMap + (i * mMapDescSize));
                if (desc->type != 7) {
                        ReservePages(desc->physAddr, desc->numPages);
                }
        }
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
}

void PageFrameAllocator::ReservePages(void *address, uint64_t page_count) {
        for (int i = 0; i < page_count; i++) {
                ReservePage((void*)(uint64_t)address + (i * 4096));
        }
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
