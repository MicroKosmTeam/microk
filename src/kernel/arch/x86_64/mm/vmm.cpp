#include <arch/x86_64/mm/vmm.h>
#include <mm/pmm.h>
#include <sys/panik.h>
#include <sys/printk.h>

#define PREFIX "[x86_64 VMM] "

volatile uint64_t *pageDir;

static uint64_t *LoadPageDir() {
	uint64_t *addr;
	asm volatile("mov %%cr3, %0" : "=r"(addr) :);
	return addr;
}

static volatile uint64_t *GetNextLevel(uint64_t *pml, size_t index, int flags) {
	if (!(pml[index] & 1)) {
		pml[index] = (uint64_t)PMM::RequestPage();
		pml[index] |= flags;
	}

	return (pml[index] & ~(0x1ff));
}

static volatile void MapPage(uint64_t *page_dir, uintptr_t physical_address, uintptr_t virtual_address, int flags) {
	size_t index4 = (size_t)(virtual_address & ((uintptr_t)0x1FF << 39)) >> 39;
	size_t index3 = (size_t)(virtual_address & ((uintptr_t)0x1FF << 30)) >> 30;
	size_t index2 = (size_t)(virtual_address & ((uintptr_t)0x1FF << 21)) >> 21;
	size_t index1 = (size_t)(virtual_address & ((uintptr_t)0x1FF << 12)) >> 12;

	uint64_t *pml3 = GetNextLevel(page_dir, index4, flags);
	uint64_t *pml2 = GetNextLevel(pml3, index3, flags);
	uint64_t *pml1 = GetNextLevel(pml2, index2, flags);

	pml1[index1] = physical_address | flags;

	flush_tlb(virtual_address);
}

static volatile void UnmapPage(uint64_t *page_dir, uintptr_t virtual_address, int flags) {
	size_t index4 = (size_t)(virtual_address & ((uintptr_t)0x1FF << 39)) >> 39;
	size_t index3 = (size_t)(virtual_address & ((uintptr_t)0x1FF << 30)) >> 30;
	size_t index2 = (size_t)(virtual_address & ((uintptr_t)0x1FF << 21)) >> 21;
	size_t index1 = (size_t)(virtual_address & ((uintptr_t)0x1FF << 12)) >> 12;

	uint64_t *pml3 = GetNextLevel(page_dir, index4, flags);
	uint64_t *pml2 = GetNextLevel(pml3, index3, flags);
	uint64_t *pml1 = GetNextLevel(pml2, index2, flags);

	pml1[index1] = 0;

	flush_tlb(virtual_address);
}

#define LIMINE_MEMMAP_USABLE                 0
#define LIMINE_MEMMAP_RESERVED               1
#define LIMINE_MEMMAP_ACPI_RECLAIMABLE       2
#define LIMINE_MEMMAP_ACPI_NVS               3
#define LIMINE_MEMMAP_BAD_MEMORY             4
#define LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE 5
#define LIMINE_MEMMAP_KERNEL_AND_MODULES     6
#define LIMINE_MEMMAP_FRAMEBUFFER            7

namespace VMM {
void InitVirtualMemoryManager(limine_memmap_entry **mMap, uint64_t mMapEntries, uint64_t offset) {
	printk(PREFIX "Initializing VMM...\n");
	printk(PREFIX "Mapping at address: 0x%x\n", offset);

	pageDir = LoadPageDir();
	if (pageDir == NULL) {
		PANIK("VMM page dir isn't properly allocated");
	}

	uint64_t memorySize = get_memory_size(mMap, mMapEntries);
	for (uint64_t i = 0; i < mMapEntries; i++) {
		limine_memmap_entry *entry = mMap[i];
/*
		for (uint64_t j = entry->base; j < entry->base + entry->length && j < memorySize; j += 0x1000) {
			//printk("Mapping 0x%x - 0x%x to 0x%x - 0x%x\n", j, j + 0x1000, j + offset, j + 0x1000 + offset);
			MapPage(pageDir, j, j, PTE_PRESENT | PTE_READ_WRITE | PTE_USER_SUPERVISOR);
			MapPage(pageDir, j, j + offset, PTE_PRESENT | PTE_READ_WRITE | PTE_USER_SUPERVISOR);
		}
*/
	}

	printk(PREFIX "Done mapping.\n");

	PushPageDir(pageDir);

	printk(PREFIX "VMM initialized\n");
}

void MapMemory(void *virtualMemory, void *physicalMemory) {
	MapPage(pageDir, physicalMemory, virtualMemory, PTE_PRESENT | PTE_READ_WRITE);
}
}
