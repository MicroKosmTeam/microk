#include <mm/memory.hpp>
#include <sys/printk.hpp>
#include <sys/panic.hpp>

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

namespace MEM {
void Init(KInfo *info) {
	if (mMapRequest.response == NULL) PANIC("Memory map request not answered by Limine");

	info->mMap = mMapRequest.response->entries;
	info->mMapEntryCount = mMapRequest.response->entry_count;

	PRINTK::PrintK("Memory map:\r\n");

	for (int i = 0; i < info->mMapEntryCount; i++) {
		PRINTK::PrintK("[0x%x - 0x%x] -> %s\r\n",
				info->mMap[i]->base,
				info->mMap[i]->base + info->mMap[i]->length,
				memTypeStrings[info->mMap[i]->type]);
	}
}
}
