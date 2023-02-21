#include <mm/memory.h>
#include <mm/pageframe.h>
#include <mm/pagetable.h>
#include <stddef.h>

void *vmallocAddress = (void*)0xfff0000000000000;

void *vmalloc(size_t size) {
	if (size % 0x1000 > 0)  { // At least the size of a page (4kb)
		size -= (size % 0x1000);
		size += 0x1000;
	}

	if (size == 0) return NULL;

        void *pos = vmallocAddress;

        for (size_t i = 0; i < size / 0x1000; i++) {
                GlobalPageTableManager.MapMemory(pos, GlobalAllocator.RequestPage());
                pos = (void*)((size_t)pos + 0x1000); // Advancing
        }

	vmallocAddress += size;
}
