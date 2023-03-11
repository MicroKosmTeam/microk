#include <arch/x64/mm/vmm.hpp>
#include <limine.h>
#include <sys/panic.hpp>
#include <mm/pmm.hpp>
#include <sys/printk.hpp>
#include <mm/memory.hpp>
#include <mm/bootmem.hpp>
#include <arch/x64/mm/pagetable.hpp>

#define PAGE_SIZE 0x1000

bool initialized = false;

static volatile limine_kernel_address_request kAddrRequest {
	.id = LIMINE_KERNEL_ADDRESS_REQUEST,
	.revision = 0
};

extern volatile uintptr_t text_start_addr, text_end_addr, rodata_start_addr, rodata_end_addr, data_start_addr, data_end_addr;

PageTable *PML4;

uint64_t higherHalf;
uint64_t kVirtualBase;

namespace x86_64 {
void InitVMM(KInfo *info) {
	if(initialized) return;

	initialized = true;

	if(kAddrRequest.response == NULL || hhdmRequest.response == NULL) PANIC("Virtual memory request not answered by Limine");

	higherHalf = info->higherHalf = hhdmRequest.response->offset;
	info->kernelPhysicalBase = kAddrRequest.response->physical_base;
	kVirtualBase = info->kernelVirtualBase = kAddrRequest.response->virtual_base;

	PML4 = (PageTable*)PMM::RequestPage();
	memset(PML4, 0, PAGE_SIZE);

	// Copy the bootloader page table into ours, so we know it's right
	PageTable *OldPML4;
	asm volatile("mov %%cr3, %0" : "=r"(OldPML4) : :"memory");
	for (int i = 0; i < PAGE_SIZE; i++) {
		PML4[i] = OldPML4[i];
	}

	GlobalPageTableManager = new PageTableManager(PML4);
	PRINTK::PrintK("Kernel page table initialized.\r\n");

	/*
	PT_Flag flags[128];
	bool flagStatus[128];
	uint64_t flagNumber;

	for (uint64_t t = text_start_addr; t < text_end_addr; t++) {
		uint64_t phys = t - info->kernelVirtualBase + info->kernelPhysicalBase;
		GlobalPageTableManager->MapMemory((void*)t , (void*)phys);
	}

	flagNumber = 1;
	flags[0] = PT_Flag::NX;
	flagStatus[0] = true;
	for (uint64_t t = rodata_start_addr; t < rodata_end_addr; t++) {
		uint64_t phys = t - info->kernelVirtualBase + info->kernelPhysicalBase;
		GlobalPageTableManager->MapMemory((void*)t , (void*)phys, flagNumber, flags, flagStatus);
	}

	flagNumber = 2;
	flags[0] = PT_Flag::NX;
	flagStatus[0] = true;
	flags[1] = PT_Flag::ReadWrite;
	flagStatus[1] = true;
	for (uint64_t t = data_start_addr; t < data_end_addr; t++) {
		uint64_t phys = t - info->kernelVirtualBase + info->kernelPhysicalBase;
		GlobalPageTableManager->MapMemory((void*)t , (void*)phys, flagNumber, flags, flagStatus);
	}


	PT_Flag flags[16];
	bool flagStatus[16];
	uint64_t flagNumber;
	flagNumber = 1;
	flags[0] = PT_Flag::ReadWrite;
	flagStatus[0] = true;

	for (uint64_t i = 0; i < info->mMapEntryCount; i++) {
		limine_memmap_entry *entry = info->mMap[i];

		uint64_t base = entry->base;
		uint64_t top = base + entry->length;

		for (uint64_t t = base; t < top; t += 4096){
			GlobalPageTableManager->MapMemory((void*)t, (void*)t, flagNumber, flags, flagStatus);
			GlobalPageTableManager->MapMemory((void*)t + info->higherHalf, (void*)t, flagNumber, flags, flagStatus);
		}
	}

	*/
	PRINTK::PrintK("Done mapping.\r\n");

	asm volatile ("mov %0, %%cr3" : : "r" (PML4));
}
}
