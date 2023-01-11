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

extern "C" void memset(void *start, uint8_t value, uint64_t num) {
        for (uint64_t i = 0; i < num; i++) {
                *(uint8_t*)((uint64_t)start + i) = value;
        }
}

extern "C" void memcpy(void *dest, void *src, size_t n){
        char *csrc = (char *)src; 
        char *cdest = (char *)dest; 
  
        for (int i=0; i<n; i++) 
                cdest[i] = csrc[i]; 
} 
