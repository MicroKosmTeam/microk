#include <arch/x64/mm/vmm.hpp>
#include <limine.h>
#include <sys/panic.hpp>
#include <mm/pmm.hpp>
#include <sys/printk.hpp>
#include <mm/memory.hpp>
#include <sys/symbol.hpp>

#define PAGE_SIZE 0x1000

bool initialized = false;

static volatile limine_kernel_address_request kAddrRequest {
	.id = LIMINE_KERNEL_ADDRESS_REQUEST,
	.revision = 0
};

Pagemap *kernelPagemap;

uint64_t higherHalf;
uint64_t kPhysicalBase;
uint64_t kVirtualBase;

Symbol *textStart, *textEnd;
Symbol *rodataStart, *rodataEnd;
Symbol *dataStart, *dataEnd;

static uint64_t *GetNextLevel(uint64_t *topLevel, size_t index, bool allocate) {
	if ((topLevel[index] & PTE_PRESENT) != 0) {
		return (uint64_t *)(PTE_GET_ADDR(topLevel[index]) + higherHalf);
	}

	if (!allocate) {
		return NULL;
	}

	void *nextLevel = PMM::RequestPage();
	memset(nextLevel, 0, PAGE_SIZE);
	if (nextLevel == NULL) {
		return NULL;
	}

	topLevel[index] = (uint64_t)nextLevel | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
	return nextLevel + higherHalf;
}

namespace x86_64 {
uint64_t MapPage(Pagemap *pageMap, uintptr_t physAddr, uintptr_t virtAddr, uint64_t flags) {
	size_t pml4Entry = (virtAddr & (0x1ffull << 39)) >> 39;
	size_t pml3Entry = (virtAddr & (0x1ffull << 30)) >> 30;
	size_t pml2Entry = (virtAddr & (0x1ffull << 21)) >> 21;
	size_t pml1Entry = (virtAddr & (0x1ffull << 12)) >> 12;

	uint64_t *pml4 = pageMap->topLevel;
	uint64_t *pml3 = GetNextLevel(pml4, pml4Entry, false);
	if (pml3 == NULL) {
	//tlb_shootdown(pagemap);
		return 4;
	}
	uint64_t *pml2 = GetNextLevel(pml3, pml3Entry, false);
	if (pml2 == NULL) {
	//tlb_shootdown(pagemap);
		return 3;
	}
	uint64_t *pml1 = GetNextLevel(pml2, pml2Entry, false);
	if (pml1 == NULL) {
	//tlb_shootdown(pagemap);
		return 2;
	}

	if ((pml1[pml1Entry] & PTE_PRESENT) == 0) {
	//tlb_shootdown(pagemap);
		return 1;
	}

	pml1[pml1Entry] = PTE_GET_ADDR(pml1[pml1Entry]) | flags;

	//tlb_shootdown(pagemap);

	return 0;
}

void SwitchToPagemap(Pagemap *pageMap) {
	asm volatile (
			"mov %0, %%cr3"
			:
			: "r" ((void *)pageMap->topLevel - higherHalf)
			: "memory"
			);
}

void InitVMM(KInfo *info) {
	if(initialized) return;

	initialized = true;

	if(kAddrRequest.response == NULL || hhdmRequest.response == NULL) PANIC("Virtual memory request not answered by Limine");

	higherHalf = info->higherHalf = hhdmRequest.response->offset;
	kPhysicalBase = info->kernelPhysicalBase = kAddrRequest.response->physical_base;
	kVirtualBase = info->kernelVirtualBase = kAddrRequest.response->virtual_base;

	kernelPagemap = new Pagemap;
	kernelPagemap->topLevel = PMM::RequestPage();
	memset(kernelPagemap->topLevel, 0, PAGE_SIZE);
	kernelPagemap->topLevel = (void*) kernelPagemap->topLevel + higherHalf;

	for (size_t i = 256; i < 512; i++) {
		if(!GetNextLevel(kernelPagemap->topLevel, i, true)) PANIC("GetNextLevel returned NULL in initialization");
	}

	textStart = LookupSymbol("textStartAddr");
	textEnd = LookupSymbol("textEndAddr");
	rodataStart = LookupSymbol("rodataStartAddr");
	rodataEnd = LookupSymbol("rodataEndAddr");
	dataStart = LookupSymbol("dataStartAddr");
	dataEnd = LookupSymbol("dataEndAddr");

	PRINTK::PrintK("Mapping the kernel sections...\r\n");

	PRINTK::PrintK("Text start:    0x%x\r\n", textStart->addr);
	PRINTK::PrintK("Text end:      0x%x\r\n", textEnd->addr);
	for (uintptr_t textAddr = textStart->addr - (textStart->addr % PAGE_SIZE); textAddr < textEnd->addr + (PAGE_SIZE - textEnd->addr % PAGE_SIZE); textAddr += PAGE_SIZE) {
		uintptr_t phys = textAddr - kVirtualBase + kPhysicalBase;
		if(MapPage(kernelPagemap, phys, textAddr, PTE_PRESENT)) PANIC("Text page does not exist");
	}

	PRINTK::PrintK("Rodata start:  0x%x\r\n", rodataStart->addr);
	PRINTK::PrintK("Rodata end:    0x%x\r\n", rodataEnd->addr);
	for (uintptr_t rodataAddr = rodataStart->addr - (rodataStart->addr % PAGE_SIZE); rodataAddr < rodataEnd->addr + (PAGE_SIZE - rodataEnd->addr % PAGE_SIZE); rodataAddr += PAGE_SIZE) {
		uintptr_t phys = rodataAddr - kVirtualBase + kPhysicalBase;
		if(MapPage(kernelPagemap, phys, rodataAddr, PTE_PRESENT | PTE_NX)) PANIC("Rodata page does not exist");
	}

	PRINTK::PrintK("Data start:    0x%x\r\n", dataStart->addr);
	PRINTK::PrintK("Data end:      0x%x\r\n", dataEnd->addr);
	for (uintptr_t dataAddr = dataStart->addr - (dataStart->addr % PAGE_SIZE); dataAddr < dataEnd->addr + (PAGE_SIZE - dataEnd->addr % PAGE_SIZE); dataAddr += PAGE_SIZE) {
		uintptr_t phys = dataAddr - kVirtualBase + kPhysicalBase;
		if(MapPage(kernelPagemap, phys, dataAddr, PTE_PRESENT | PTE_WRITABLE | PTE_NX)) PANIC("Data page does not exist");
	}

	PRINTK::PrintK("Mapping lower 4GB\r\n");
	for (uintptr_t addr = 0x1000; addr < 0x100000000; addr += PAGE_SIZE) {
		if(MapPage(kernelPagemap, addr, addr, PTE_PRESENT | PTE_WRITABLE)) PANIC("4G page does not exist");
		if(MapPage(kernelPagemap, addr, addr + higherHalf, PTE_PRESENT | PTE_WRITABLE | PTE_NX)) PANIC("Page does not exist");
	}

	PRINTK::PrintK("Mapping available memory over 4GB\r\n");
	for (size_t i = 0; i < info->mMapEntryCount; i++) {
		limine_memmap_entry *entry = info->mMap[i];

		uintptr_t base = entry->base - (entry->base % PAGE_SIZE);
		uintptr_t top = base + entry->length + (PAGE_SIZE - entry->length % PAGE_SIZE);

		if (top <= 0x100000000) continue;

		PRINTK::PrintK("Section start: 0x%x\r\n", base);
		PRINTK::PrintK("Section end:   0x%x\r\n", top);

		for (uintptr_t j = base; j < top; j += PAGE_SIZE) {
			if (j < 0x100000000) continue;

			if(MapPage(kernelPagemap, j, j, PTE_PRESENT | PTE_WRITABLE)) PANIC("Page does not exist");
			if(MapPage(kernelPagemap, j, j + higherHalf, PTE_PRESENT | PTE_WRITABLE | PTE_NX)) PANIC("Page does not exist");
		}
	}

	PRINTK::PrintK("Switching to new pagemap (at 0x%x)...\r\n", (void*) kernelPagemap->topLevel - higherHalf);
	SwitchToPagemap(kernelPagemap);
}
}
