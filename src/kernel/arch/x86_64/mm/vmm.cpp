#include <arch/x86_64/mm/vmm.h>
#include <mm/pmm.h>
#include <sys/panik.h>
#include <sys/printk.h>

#define PREFIX "[x86_64 VMM] "

uint64_t *pageDir;

static uint64_t *LoadPageDir() {
	uint64_t *addr;
	// Stopgap measure
	// because otherwise something doesn't work.
	// and I have absolutely no idea why
	// and I don't want to spend any more time on paging
	// when I have so much to do.
	// おやすみなさい

	asm volatile("mov %%cr3, %0" : "=r"(addr) : :"memory");

	//addr = PMM::RequestPage();
	//memset(addr, 0, 0x1000);

	return addr;
}

static volatile uint64_t *GetNextLevel(uint64_t *pml, size_t index, bool allocate) {
	if ((pml[index] & PTE_PRESENT) != 0) {
		return (uint64_t *)(PTE_GET_ADDR(pml[index]) + hhdm);
	}

	if (!allocate) {
		return NULL;
	}

	void *nextLevel = PMM::RequestPage();
	if (nextLevel == NULL) {
		PANIK("Nomem.");
	}

	pml[index] = (uint64_t)nextLevel | PTE_PRESENT | PTE_READ_WRITE | PTE_USER_SUPERVISOR;
	return nextLevel + hhdm;
}

static volatile void MapPage(uint64_t *page_dir, uintptr_t physical_address, uintptr_t virtual_address, int flags) {
	size_t index4 = (size_t)(virtual_address & ((uintptr_t)0x1FF << 39)) >> 39;
	size_t index3 = (size_t)(virtual_address & ((uintptr_t)0x1FF << 30)) >> 30;
	size_t index2 = (size_t)(virtual_address & ((uintptr_t)0x1FF << 21)) >> 21;
	size_t index1 = (size_t)(virtual_address & ((uintptr_t)0x1FF << 12)) >> 12;

	uint64_t *pml3 = GetNextLevel(page_dir, index4, true);
	uint64_t *pml2 = GetNextLevel(pml3, index3, true);
	uint64_t *pml1 = GetNextLevel(pml2, index2, true);

	pml1[index1] = physical_address | flags;

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

#define PAGE_SIZE 0x1000

namespace VMM {
void InitVirtualMemoryManager(BootInfo *bootInfo) {
	printk(PREFIX "Initializing VMM...\n");
	hhdm = bootInfo->hhdmOffset;
	printk(PREFIX "Mapping at address: 0x%x\n", hhdm);

	pageDir = LoadPageDir();
	if (pageDir == NULL) {
		PANIK("VMM page dir isn't properly allocated");
	}

	uintptr_t textStart = text_start_addr,
		  rodataStart = rodata_start_addr,
		  dataStart = data_start_addr,
		  textEnd = text_end_addr,
		  rodataEnd = rodata_end_addr,
		  dataEnd = data_end_addr;

	for (size_t i = 0; i < 512; i++) {
		GetNextLevel(pageDir, i, true);
	}

	for (uintptr_t textAddr = textStart; textAddr < textEnd; textAddr += PAGE_SIZE) {
		uintptr_t phys = textAddr - bootInfo->virtualKernelOffset + bootInfo->physicalKernelOffset;
		MapPage(pageDir, phys, textAddr, PTE_PRESENT);
	}

	for (uintptr_t rodataAddr = rodataStart; rodataAddr < rodataEnd; rodataAddr += PAGE_SIZE) {
		uintptr_t phys = rodataAddr - bootInfo->virtualKernelOffset + bootInfo->physicalKernelOffset;
		MapPage(pageDir, phys, rodataAddr, PTE_PRESENT | PTE_UNAVAILABLE);
	}

	for (uintptr_t dataAddr = dataStart; dataAddr < dataEnd; dataAddr += PAGE_SIZE) {
		uintptr_t phys = dataAddr - bootInfo->virtualKernelOffset + bootInfo->physicalKernelOffset;
		MapPage(pageDir, phys, dataAddr, PTE_PRESENT | PTE_UNAVAILABLE | PTE_READ_WRITE);
	}

	for (size_t i = 0; i < bootInfo->mMapEntries - 1; i++) {
		limine_memmap_entry *entry = bootInfo->mMap[i];

		uintptr_t base = entry->base;
		uintptr_t top = entry->base + entry->length;

		for (uintptr_t j = base; j < top; j += PAGE_SIZE) {
			MapPage(pageDir, j, j, PTE_PRESENT | PTE_READ_WRITE);
			MapPage(pageDir, j + bootInfo->hhdmOffset, j, PTE_PRESENT | PTE_READ_WRITE | PTE_UNAVAILABLE);
		}
	}

	printk(PREFIX "Done mapping.\n");

	PushPageDir(pageDir);

	printk(PREFIX "VMM initialized\n");
}

void MapMemory(void *virtualMemory, void *physicalMemory) {
	MapPage(pageDir, physicalMemory, virtualMemory, PTE_PRESENT | PTE_READ_WRITE);
}
}
