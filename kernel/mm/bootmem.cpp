#include <mm/bootmem.hpp>

const uint32_t BOOTMEM_SIZE = 1024 * 1024;
uint8_t bootmemMemory[BOOTMEM_SIZE];
uint32_t lastPosition = 0;

namespace BOOTMEM {
	void *Malloc(size_t size) {
		if (lastPosition + size > BOOTMEM_SIZE) return NULL;

		void *seg = &bootmemMemory + lastPosition;
		lastPosition += size;

		return seg;
	}

	void Free(void *address) {
		return; // Bootmem can't free
	}

	uint32_t GetFree() {
		return BOOTMEM_SIZE - lastPosition;
	}

	uint32_t GetTotal() {
		return BOOTMEM_SIZE;
	}
}
