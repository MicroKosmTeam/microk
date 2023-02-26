#include <arch/x64/main.hpp>
#include <arch/x64/cpu/gdt.hpp>
#include <sys/printk.hpp>

namespace x86_64 {
void Init(KInfo *info) {
	asm ("mov %%rsp, %0" : "=r"(info->kernelStack) : : "memory");

	PRINTK::PrintK("Loading the x86_64 GDT\r\n");
	x86_64::LoadGDT(info->kernelStack);
	PRINTK::PrintK("GDT Loaded.\r\n");
}
}
