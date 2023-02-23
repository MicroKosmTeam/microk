#include <mm/memory.h>
#include <cdefs.h>

uint64_t hhdm;

uint64_t get_memory_size(limine_memmap_entry **mMap, uint64_t mMapEntries) {
        static uint64_t memory_size_bytes = 0;

        if (memory_size_bytes > 0) {
                return memory_size_bytes;
        }

        for (int i = 0; i < mMapEntries; i++){
                limine_memmap_entry *desc = mMap[i];
                memory_size_bytes += desc->length;
        }

        return memory_size_bytes;
}

/*void *vmalloc(size_t size) {
        if (size % 0x1000) { // We can't allocate less that a page
                size -= size % 0x1000;
                size += 0x1000;
        }

        size_t pageCount = size / 0x1000;

        for (size_t i = 0; i < pageCount; i++) {
                GlobalPageTableManager.MapMemory(heapEnd, GlobalAllocator.RequestPage());
                heapEnd = (void*)((size_t)heapEnd + 0x1000);
        }
}*/


extern "C" int memcmp(const void* buf1, const void* buf2, size_t count) {
    if(!count)
        return(0);

    while(--count && *(char*)buf1 == *(char*)buf2 ) {
        buf1 = (char*)buf1 + 1;
        buf2 = (char*)buf2 + 1;
    }

    return(*((unsigned char*)buf1) - *((unsigned char*)buf2));
}

extern "C" void memset(void *start, uint8_t value, uint64_t num) {
#ifdef CONFIG_ARCH_x86_64 && CONFIG_HW_SSE
        int d0, d1;
        __asm__ __volatile__(
                "rep\n\t"
                "stosb"
                : "=&c" (d0), "=&D" (d1)
                :"a" (value),"1" (start),"0" (num)
                :"memory");
#else
        for (uint64_t i = 0; i < num; i++) {
                *(uint8_t*)((uint64_t)start + i) = value;
        }
#endif
}

extern "C" void memcpy(void *dest, void *src, size_t n) {
#ifdef CONFIG_ARCH_x86_64 && CONFIG_HW_SSE
        int i;
        for(i=0; i<n/16; i++) {
                __asm__ __volatile__ (
                                "movups (%0), %%xmm0\n"
                                "movntdq %%xmm0, (%1)\n"
                                ::"r"(src),
                                "r"(dest) : "memory");

                src += 16;
                dest += 16;
        }

        if(n & 7) {
                n = n&7;

                int d0, d1, d2;
                __asm__ __volatile__(
                        "rep ; movsl\n\t"
                        "testb $2,%b4\n\t"
                        "je 1f\n\t"
                        "movsw\n"
                        "1:\ttestb $1,%b4\n\t"
                        "je 2f\n\t"
                        "movsb\n"
                        "2:"
                        : "=&c" (d0), "=&D" (d1), "=&S" (d2)
                        :"0" (n/4), "q" (n),"1" ((long) dest),"2" ((long) src)
                        : "memory");
        }
#else
        char *csrc = (char *)src;
        char *cdest = (char *)dest;

        for (int i=0; i<n; i++) cdest[i] = csrc[i];
#endif
}
