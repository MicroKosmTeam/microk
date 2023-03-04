#include <init/modules.hpp>
#include <limine.h>
#include <sys/panic.hpp>
#include <sys/printk.hpp>
#include <mm/memory.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <sys/ustar/ustar.hpp>
#include <mm/string.hpp>
#include <sys/elf.hpp>
#include <fs/vfs.hpp>
#include <cdefs.h>

static volatile limine_module_request moduleRequest {
	.id = LIMINE_MODULE_REQUEST,
	.revision = 0
};

uint64_t *KRNLSYMTABLE;

namespace MODULE {
void Init(KInfo *info) {
	// Kernel symbol table
	for (int i = 0; i < CONFIG_SYMBOL_TABLE_PAGES * 0x1000; i+=0x1000) {
		VMM::MapMemory(CONFIG_SYMBOL_TABLE_BASE + i, PMM::RequestPage());
	}

	KRNLSYMTABLE = CONFIG_SYMBOL_TABLE_BASE;
	memset(KRNLSYMTABLE, 0, 0x1000);

	KRNLSYMTABLE[0] = &PRINTK::PrintK;
	KRNLSYMTABLE[1] = &Malloc;
	KRNLSYMTABLE[2] = &Free;

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
			PRINTK::PrintK("Initrd: [ %s %d ] %s\r\n",
					info->modules[i]->path,
					info->modules[i]->size,
					info->modules[i]->cmdline);

			PRINTK::PrintK("Unpacking initrd in /initrd.\r\n");
			USTAR::LoadArchive(info->modules[i]->address, VFS::GetInitrdFS()->node);

			VFS::ListDir(VFS::GetInitrdFS()->node);
			VFS::ListDir(VFS::GetRootFS()->node);
		} else {
			PRINTK::PrintK("Unknown module: [ %s %d ] %s\r\n",
					info->modules[i]->path,
					info->modules[i]->size,
					info->modules[i]->cmdline);
		}

	}
}
}
