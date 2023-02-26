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

	PRINTK::PrintK("Free bootmem memory: %dkb out of %dkb.\r\n", BOOTMEM::GetFree() / 1024, BOOTMEM::GetTotal() / 1024);

	MEM::Init(info);

	ACPI::Init();

	SCHED::Init();

	PRINTK::PrintK("MicroK Started.\r\n");

	SCHED::NewKernelThread(&restInit);

        asm volatile("hlt");
}

void restInit(uint64_t tmp) {

}
