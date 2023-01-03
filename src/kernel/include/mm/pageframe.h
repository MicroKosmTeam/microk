#pragma once
#include <mm/efimem.h>
#include <stdint.h>
#include <stddef.h>
#include <mm/bitmap.h>
#include <mm/memory.h>

class PageFrameAllocator {
public:
        void ReadEFIMemoryMap(EFI_MEMORY_DESCRIPTOR *mMap, size_t mMapSize, size_t mMapDescSize);
        Bitmap page_bitmap;

        void FreePage(void *address);
        void FreePages(void *address, uint64_t page_count);
        void LockPage(void *address);
        void LockPages(void *address, uint64_t page_count);

        void *RequestPage();

        uint64_t GetFreeMem();
        uint64_t GetUsedMem();
        uint64_t GetReservedMem();

private:
        void InitBitmap(size_t bitmap_size, void *buffer_address);
        void UnreservePage(void *address);
        void UnreservePages(void *address, uint64_t page_count);
        void ReservePage(void *address);
        void ReservePages(void *address, uint64_t page_count);
};

extern PageFrameAllocator GlobalAllocator;
