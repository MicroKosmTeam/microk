#pragma once
#include <stdint.h>
#include <stddef.h>

namespace BOOTMEM {
	void *Malloc(size_t size);
	void Free(void *address);

	uint32_t GetFree();
	uint32_t GetTotal();

	bool DeactivateBootmem();
	bool BootmemIsActive();
}
