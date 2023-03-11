#include <mm/memory.hpp>
#include <sys/printk.hpp>
#include <sys/panic.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <mm/bootmem.hpp>
#include <mm/heap.hpp>
#include <arch/x64/mm/pageindexer.hpp>

volatile limine_hhdm_request hhdmRequest {
	.id = LIMINE_HHDM_REQUEST,
	.revision = 0
};

static volatile limine_memmap_request mMapRequest {
	.id = LIMINE_MEMMAP_REQUEST,
	.revision = 0
};

static char *memTypeStrings[] = {
	"Usable",
	"Reserved",
	"ACPI Reclaimable",
	"ACPI NVS",
	"Bad",
	"Bootloader reclaimable",
	"Kernel and modules",
	"Framebuffer"
};

void *Malloc(size_t size) {
	if(HEAP::IsHeapActive()) return HEAP::Malloc(size);
	if(BOOTMEM::BootmemIsActive()) return BOOTMEM::Malloc(size);
	else return NULL;
}

void *VMalloc(void *address, size_t size) {
	if(size % 0x1000) {
		size = (size / 0x1000 + 1) * 0x1000;
	}

	for (int i = 0; i < size; i+= 0x1000) {
		void *page = PMM::RequestPage();
		VMM::MapMemory(address + size, page);
	}

	return address;
}

void Free(void *p) {
	HEAP::Free(p);
}

// Doesn't actually work, we need to get the physicalAddress
void VFree(void *address, size_t size) {
	if(size % 0x1000) {
		size = (size / 0x1000 + 1) * 0x1000;
	}

	uint64_t physicalAddress = 0;

	for (int i = 0; i < size; i+= 0x1000) {
		PMM::FreePage(physicalAddress + i);
		VMM::MapMemory(physicalAddress + i, physicalAddress + i);
	}

}

void *operator new(size_t size) {
	if(HEAP::IsHeapActive()) return HEAP::Malloc(size);
	if(BOOTMEM::BootmemIsActive()) return BOOTMEM::Malloc(size);
	else return NULL;
}

void *operator new[](size_t size) {
	if(HEAP::IsHeapActive()) return HEAP::Malloc(size);
	if(BOOTMEM::BootmemIsActive()) return BOOTMEM::Malloc(size);
	else return NULL;
}

void operator delete(void* p) {
	// Now, here comes the problem in deciding who allocated this block
	// We should assume that someone that allocs on BOOTMEM
	// will not call free

	HEAP::Free(p);
}

void operator delete(void* p, size_t size) {
	// Now, here comes the problem in deciding who allocated this block
	// We should assume that someone that allocs on BOOTMEM
	// will not call free

	HEAP::Free(p);
}

void operator delete[](void* p) {
	// Now, here comes the problem in deciding who allocated this block
	// We should assume that someone that allocs on BOOTMEM
	// will not call free

	HEAP::Free(p);
}

void operator delete[](void* p, size_t size) {
	// Now, here comes the problem in deciding who allocated this block
	// We should assume that someone that allocs on BOOTMEM
	// will not call free

	HEAP::Free(p);
}

namespace MEM {
void Init(KInfo *info) {
	if (mMapRequest.response == NULL) PANIC("Memory map request not answered by Limine");

	info->mMap = mMapRequest.response->entries;
	info->mMapEntryCount = mMapRequest.response->entry_count;

	PMM::InitPageFrameAllocator(info);

	PRINTK::PrintK("Memory map:\r\n");

	for (int i = 0; i < info->mMapEntryCount; i++) {
		PRINTK::PrintK("[0x%x - 0x%x] -> %s\r\n",
				info->mMap[i]->base,
				info->mMap[i]->base + info->mMap[i]->length,
				memTypeStrings[info->mMap[i]->type]);
	}

	VMM::InitVMM(info);

	PRINTK::PrintK("Done.\r\n");
}
}
