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

static volatile limine_stack_size_request stackRequest {
	.id = LIMINE_STACK_SIZE_REQUEST,
	.revision = 0,
	.stack_size = CONFIG_STACK_SIZE
};

void restInit(uint64_t tmp);
KInfo *info;

extern "C" void kernelStart(void) {
	PRINTK::EarlyInit();

	info = (KInfo*)BOOTMEM::Malloc(sizeof(KInfo) + 1);

	if (stackRequest.response == NULL) PANIC("Stack size request not answered by Limine");

	x86_64::Init(info);

	MODULE::Init(info);

	MEM::Init(info);

	HEAP::InitializeHeap(0x100000000, 0x100);

	BOOTMEM::DeactivateBootmem();
	PRINTK::PrintK("Free bootmem memory: %dkb out of %dkb.\r\n", BOOTMEM::GetFree() / 1024, BOOTMEM::GetTotal() / 1024);

	ACPI::Init();

	SCHED::Init();

	PRINTK::PrintK("MicroK Started.\r\n");
	PRINTK::PrintK("Free heap memory: %dkb out of %dkb.\r\n", HEAP::GetFree() / 1024, HEAP::GetTotal() / 1024);

	while (true) {
		asm volatile("hlt");
	}

	SCHED::NewKernelThread(&restInit);
}

void restInit(uint64_t tmp) {

}
