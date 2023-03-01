#pragma once
#include <stddef.h>
#include <stdint.h>
#include <limine.h>
#include <dev/uart/uart.hpp>

struct KInfo {
	limine_memmap_entry **mMap;
	uint64_t mMapEntryCount;
	uintptr_t higherHalf;

	limine_file **modules;
	uint64_t moduleCount;

	uintptr_t kernelStack;
	uint64_t kernelPhysicalBase;
	uint64_t kernelVirtualBase;

	UARTDevice *kernelPort;
};
