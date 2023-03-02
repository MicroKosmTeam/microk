#include <init/modules.hpp>
#include <limine.h>
#include <sys/panic.hpp>
#include <sys/printk.hpp>
#include <mm/memory.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <sys/ustar/ustar.hpp>
#include <mm/string.hpp>

#define SYMBOL_NUMBER 2

static volatile limine_module_request moduleRequest {
	.id = LIMINE_MODULE_REQUEST,
	.revision = 0
};

uint64_t *KRNLSYMTABLE;

namespace MODULE {
void Init(KInfo *info) {
	// Kernel symbol table
	KRNLSYMTABLE = PMM::RequestPage();
	memset(KRNLSYMTABLE, 0, 0x1000);

	KRNLSYMTABLE[0] = &PRINTK::PrintK;
	KRNLSYMTABLE[1] = &Malloc;
	KRNLSYMTABLE[2] = &Free;

	VMM::MapMemory(0xffffffffffff0000, KRNLSYMTABLE);

	void (*testPrintk)(char *format, ...) = KRNLSYMTABLE[0];
	void* (*testMalloc)(size_t size) = KRNLSYMTABLE[1];
	void (*testFree)(void *p) = KRNLSYMTABLE[2];

	uint64_t *test = testMalloc(10);
	*test = 0x69;
	testPrintk("Test: 0x%x\r\n", *test);
	testFree(test);

	// Loading initrd
	if (moduleRequest.response == NULL) PANIC("Module request not answered by Limine");

	info->modules = moduleRequest.response->modules;
	info->moduleCount = moduleRequest.response->module_count;

	PRINTK::PrintK("Available modules:\r\n");

	for (int i = 0; i < info->moduleCount; i++) {

		if (strcmp(info->modules[i]->cmdline, "INITRD") == 0) {
			PRINTK::PrintK("Initrd found!\r\n");
			USTAR::LoadArchive(info->modules[i]->address);
		} else {
			PRINTK::PrintK("Unknown module: [ %s %d ] %s\r\n",
					info->modules[i]->path,
					info->modules[i]->size,
					info->modules[i]->cmdline);
		}

	}


}
}
