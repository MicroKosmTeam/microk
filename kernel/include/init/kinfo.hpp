/*
   File: include/init/kinfo.hpp
*/

#pragma once
#include <stddef.h>
#include <stdint.h>
#include <limine.h>
#include <dev/uart/uart.hpp>

/*
   KInfo struct
    Contains some basic information to be passes between components of the kernel
    Sould only be allocated onjce in kernelStart()
*/
struct KInfo {
	/* Memory map information */
	limine_memmap_entry **mMap; /* Pointer to the Limine memory map */
	uint64_t mMapEntryCount; /* Number of memory map regions */
	uintptr_t higherHalf; /* Start of higher half mapping */
	uintptr_t kernelStack; /* Start of kernel stack */
	uint64_t kernelPhysicalBase; /* Start of the kernel in physical memory */
	uint64_t kernelVirtualBase; /* Start of the kernel in virtual memory */

	/* Module information */
	limine_file **modules; /* Pointer to the Limine modules */
	uint64_t moduleCount; /* Number of modules provided */

	/* Kernel serial device */
	UARTDevice *kernelPort; /* UART device used as serial port */

	/* ACPI information */
	void *rsdp; /* The pointer to ther RSDP */
};
