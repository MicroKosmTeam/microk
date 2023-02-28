#include <mm/bootmem.hpp>
#include <cdefs.h>

const uint32_t BOOTMEM_SIZE = CONFIG_BOOTMEM_SIZE;
uint8_t bootmemMemory[BOOTMEM_SIZE];
uint32_t lastPosition = 0;
bool bootmemStatus = true;

namespace BOOTMEM {
	bool DeactivateBootmem() {
		bootmemStatus = false;
	}

	bool BootmemIsActive() {
		return bootmemStatus;
	}

	void *Malloc(size_t size) {
		if (lastPosition + size > BOOTMEM_SIZE) {
			DeactivateBootmem();
			return NULL;
		}

		void *seg = bootmemMemory + lastPosition;
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
