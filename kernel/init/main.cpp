#include <cdefs.h>
#include <stdint.h>
#include <stddef.h>
#include <limine.h>
#include <sys/panic.hpp>
#include <init/kinfo.hpp>
#include <sys/printk.hpp>
#include <mm/memory.hpp>
#include <mm/bootmem.hpp>
#include <init/modules.hpp>
#include <arch/x64/main.hpp>
#include <dev/acpi/acpi.hpp>
#include <proc/scheduler.hpp>
#include <mm/heap.hpp>
#include <mm/pmm.hpp>
#include <fs/vfs.hpp>
#include <sys/ustar/ustar.hpp>

static volatile limine_stack_size_request stackRequest {
	.id = LIMINE_STACK_SIZE_REQUEST,
	.revision = 0,
	.stack_size = CONFIG_STACK_SIZE
};

void restInit(KInfo *info);
KInfo *info;

extern "C" void kernelStart(void) {
	info = (KInfo*)BOOTMEM::Malloc(sizeof(KInfo) + 1);

	PRINTK::EarlyInit(info);

	if (stackRequest.response == NULL) PANIC("Stack size request not answered by Limine");

	x86_64::Init(info);

	MEM::Init(info);

	HEAP::InitializeHeap(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE / 0x1000);
	BOOTMEM::DeactivateBootmem();
	PRINTK::PrintK("Free bootmem memory: %dkb out of %dkb.\r\n", BOOTMEM::GetFree() / 1024, BOOTMEM::GetTotal() / 1024);

	VFS::Init(info);

	// TODO: Fix initrd loading
	MODULE::Init(info);

	ACPI::Init(info);

	PRINTK::PrintK(" __  __  _                _  __\r\n"
		       "|  \\/  |(_) __  _ _  ___ | |/ /\r\n"
		       "| |\\/| || |/ _|| '_|/ _ \\|   < \r\n"
		       "|_|  |_||_|\\__||_|  \\___/|_|\\_\\\r\n"
		       "The operating system for the future...at your fingertips.\r\n");
	PRINTK::PrintK("MicroK Started.\r\n");
	PRINTK::PrintK("Free memory: %dkb out of %dkb (%d%% free).\r\n",
			PMM::GetFreeMem() / 1024,
			(PMM::GetFreeMem() + PMM::GetUsedMem()) / 1024,
			(PMM::GetFreeMem() + PMM::GetUsedMem()) / PMM::GetFreeMem() * 100);

	restInit(info);
}

void restInit(KInfo *info) {
	PRINTK::PrintK("Kernel is now resting...\r\n");

	while (true) {
		asm volatile ("hlt");
	}
}
