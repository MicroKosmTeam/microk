#pragma once
#include <init/kutil.h>

namespace x86_64 {
	void LoadGDT();
	void PrepareInterrupts(BootInfo *bootInfo);
	void PreparePaging(limine_memmap_entry **mMap, uint64_t mMapEntries, uint64_t offset);
}

