#include <mm/memory.h>

uint64_t get_memory_size(EFI_MEMORY_DESCRIPTOR *mMap, uint64_t mMapEntries, uint64_t mMapDescSize) {
        static uint64_t memory_size_bytes = 0;
        
        if (memory_size_bytes > 0) {
                return memory_size_bytes;
        }

        for (int i = 0; i < mMapEntries; i++){
                EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)mMap + (i * mMapDescSize));
                memory_size_bytes += desc->numPages * 4096;
        }

        return memory_size_bytes;
}
