#pragma once
#include <stddef.h>
#include <stdint.h>
#include <limine.h>

struct KInfo {
	limine_memmap_entry **mMap;
	uint64_t mMapEntryCount;

	limine_file **modules;
	uint64_t moduleCount;

	uintptr_t kernelStack;
};
