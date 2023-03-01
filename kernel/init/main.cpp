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
#include <fs/vfs.hpp>

static volatile limine_stack_size_request stackRequest {
	.id = LIMINE_STACK_SIZE_REQUEST,
	.revision = 0,
	.stack_size = CONFIG_STACK_SIZE
};

void restInit(uint64_t tmp);
KInfo *info;

extern "C" void isrCommon(void) {
}

extern "C" void kernelStart(void) {
	info = (KInfo*)BOOTMEM::Malloc(sizeof(KInfo) + 1);

	PRINTK::EarlyInit(info);

	if (stackRequest.response == NULL) PANIC("Stack size request not answered by Limine");

	x86_64::Init(info);


	MEM::Init(info);

	HEAP::InitializeHeap(info->kernelVirtualBase + 0x1FF00000, 0x1000);
	BOOTMEM::DeactivateBootmem();
	PRINTK::PrintK("Free bootmem memory: %dkb out of %dkb.\r\n", BOOTMEM::GetFree() / 1024, BOOTMEM::GetTotal() / 1024);

	VFS::Init(info);

	PRINTK::PrintK(" __  __  _                _  __\r\n"
		       "|  \\/  |(_) __  _ _  ___ | |/ /\r\n"
		       "| |\\/| || |/ _|| '_|/ _ \\|   < \r\n"
		       "|_|  |_||_|\\__||_|  \\___/|_|\\_\\\r\n"
		       "The operating system for the future...at your fingertips.\r\n");
	PRINTK::PrintK("MicroK Started.\r\n");
	PRINTK::PrintK("Free heap memory: %dkb out of %dkb.\r\n", HEAP::GetFree() / 1024, HEAP::GetTotal() / 1024);

	ACPI::Init();

	SCHED::Init();

	MODULE::Init(info);

	SCHED::NewKernelThread(&restInit);
}

void restInit(uint64_t tmp) {

	while (true) {
		asm volatile ("hlt");
	}
}
