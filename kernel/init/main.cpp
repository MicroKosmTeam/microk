#include <stdint.h>
#include <stddef.h>
#include <limine.h>
#include <sys/printk.hpp>
#include <mm/bootmem.hpp>

void restInit();

extern "C" void kernelStart(void) {
	PRINTK::EarlyInit();
	PRINTK::PrintK("MicroK Started. Free bootmem memory: %d out of %d.\n", BOOTMEM::GetFree(), BOOTMEM::GetTotal());
/*
	x86_64::Init();

	MEM::Init();

	CPU::Init();

	ACPI::Init();

	SCHED::Init();

	SCHED::NewKernelThread(restInit, NULL, true);
*/
        asm volatile("hlt");
}

void restInit() {

}
