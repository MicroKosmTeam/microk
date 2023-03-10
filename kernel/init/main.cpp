/*
   File: init/main.cpp
*/

#include <cdefs.h>
#include <stdint.h>
#include <stddef.h>
#include <limine.h>
#include <mm/pmm.hpp>
#include <fs/vfs.hpp>
#include <mm/heap.hpp>
#include <sys/panic.hpp>
#include <mm/memory.hpp>
#include <mm/bootmem.hpp>
#include <init/kinfo.hpp>
#include <sys/printk.hpp>
#include <sys/symbol.hpp>
#include <init/modules.hpp>
#include <arch/x64/main.hpp>
#include <dev/acpi/acpi.hpp>
#include <proc/scheduler.hpp>
#include <sys/ustar/ustar.hpp>

/*
   Stack request for Limine
    Stack size configured in autoconf.h
*/
static volatile limine_stack_size_request stackRequest {
	.id = LIMINE_STACK_SIZE_REQUEST,
	.revision = 0,
	.stack_size = CONFIG_STACK_SIZE
};

/*
   void restInit(KInfo *info)
    Function that is started by the schedduler once kernel startup is complete.
*/
void restInit(KInfo *info);

/*
   KInfo *info
    Contains some basic information to be passed between components of the kernel
*/
KInfo *info;

/*
   extern "C" void kenrnelStart()
    Main kernel function. Declared extern "C" so that its name doesn't get
    mangled by the compiler
*/
extern "C" void kernelStart(void) {
	/* Allocating memory for the info struct*/
	info = (KInfo*)BOOTMEM::Malloc(sizeof(KInfo) + 1);

	/* Loading early serial printk */
	PRINTK::EarlyInit(info);

	/* Checking if the stack request has been answered by Limine */
	/* If not, give up and shut down */
	if (stackRequest.response == NULL) PANIC("Stack size request not answered by Limine");

	/* Starting specific arch-dependent instructions */
#ifdef CONFIG_ARCH_x86_64
	/* Starting x8_64 specific instructions */
	x86_64::Init(info);
#endif

	/* Starting the memory subsystem */
	MEM::Init(info);

	/* Initializing the heap */
	HEAP::InitializeHeap(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE / 0x1000);
	/* With the heap initialize, disable new bootmem allocations */
	BOOTMEM::DeactivateBootmem();
	PRINTK::PrintK("Free bootmem memory: %dkb out of %dkb.\r\n", BOOTMEM::GetFree() / 1024, BOOTMEM::GetTotal() / 1024);
	/* Starting the VFS */
	VFS::Init(info);

	/* Starting ACPI subsystem */
	ACPI::Init(info);

	PRINTK::PrintK(" __  __  _                _  __\r\n"
		       "|  \\/  |(_) __  _ _  ___ | |/ /\r\n"
		       "| |\\/| || |/ _|| '_|/ _ \\|   < \r\n"
		       "|_|  |_||_|\\__||_|  \\___/|_|\\_\\\r\n"
		       "The operating system for the future...at your fingertips.\r\n");
	PRINTK::PrintK("%s %s Started.\r\n", CONFIG_KERNEL_CNAME, CONFIG_KERNEL_CVER);
	PRINTK::PrintK("Kernel size %dkb\r\n",
			(LookupSymbol("kernelEndAddr")->addr - LookupSymbol("kernelStartAddr")->addr) / 1024
			);
	PRINTK::PrintK("Free memory: %dkb out of %dkb (%d%% free).\r\n",
			PMM::GetFreeMem() / 1024,
			(PMM::GetFreeMem() + PMM::GetUsedMem()) / 1024,
			(PMM::GetFreeMem() + PMM::GetUsedMem()) / PMM::GetFreeMem() * 100);

	PRINTK::PrintK("Press \'c\' to continue and load the ELF file.\n\r");

	char ch;
	while (ch != 'c') {
		ch = info->kernelPort->GetChar();
	}

	/* Starting the modules subsystem */
	MODULE::Init(info);

	PRINTK::PrintK("Press \'r\' to reset the machine.\n\r");
	while (ch != 'r') {
		ch = info->kernelPort->GetChar();
	}

	/* Finishing kernel startup */
	restInit(info);
}

void restInit(KInfo *info) {
	/* We are done with the boot process */
	PRINTK::PrintK("Kernel is now resting...\r\n");

	while (true) {
		asm volatile ("hlt");
	}
}
