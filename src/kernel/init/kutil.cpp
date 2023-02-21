#include <cdefs.h>
#include <init/kutil.h>
#include <sys/icxxabi.h>
#include <sys/cstr.h>
#include <mm/memory.h>
#include <sys/panik.h>
#include <sys/printk.h>
#include <mm/heap.h>

#define PREFIX "[KINIT] "

#ifdef CONFIG_ARCH_x86_64
#include <arch/x86_64/init.h>
#include <arch/x86_64/cpu/cpu.h>
#include <arch/x86_64/mm/vmm.h>
#endif

uintptr_t kernelStack;
void PrepareMemory(BootInfo *bootInfo) {
#ifdef CONFIG_ARCH_x86_64
	// Loading the GDT
	printk(PREFIX "Loading the GDT...\n");
	x86_64::LoadGDT();
#endif
	// Initializing the GlobalAllocator with EFI Memory data
	printk(PREFIX "Initializing the page frame allocator...\n");
	PMM::InitPageFrameAllocator(bootInfo->mMap, bootInfo->mMapEntries);

	printk(PREFIX "Stack at: 0x%x\n", kernelStack);
	printk(PREFIX "HHDM at: 0x%x\n", bootInfo->hhdmOffset);
	printk(PREFIX "Framebuffers: %d\n", bootInfo->framebufferCount);
	printk(PREFIX "Modules: %d\n", bootInfo->moduleCount);

	// Loading modules
	for (int i = 0; i < bootInfo->moduleCount; i++) {
		printk(PREFIX " - Module: [0x%x - 0x%x] %s %s\n",
		       bootInfo->modules[i]->address,
		       bootInfo->modules[i]->address + bootInfo->modules[i]->size,
		       bootInfo->modules[i]->path, bootInfo->modules[i]->cmdline);
	}


#ifdef CONFIG_ARCH_x86_64
        // Interrupt initialization
	printk(PREFIX "Loading interrupts...\n");
	x86_64::PrepareInterrupts(bootInfo);

	printk(PREFIX "Preparing paging...\n");
	x86_64::PreparePaging(bootInfo->mMap, bootInfo->mMapEntries, bootInfo->hhdmOffset);

        // Init CPU Features
	printk(PREFIX "Setting up x86_64 CPU Features...\n");
        x86_64::CPUInit();
#endif

	uint64_t heapAddress = bootInfo->hhdmOffset + 0x000000100000000000;
	uint64_t heapPages = 0x100;
	InitializeHeap(heapAddress, heapPages);

	limine_framebuffer *fb = bootInfo->framebuffers[0];
	printk(PREFIX "%d x %d, %d-bit color. R: %d G: %d B: %d\n", fb->width, fb->height, fb->bpp, fb->red_mask_shift, fb->green_mask_shift, fb->blue_mask_shift);
	uint32_t *fb_mem = fb->address + bootInfo->hhdmOffset;
	uint8_t r = 255;
        uint8_t g = 0;
        uint8_t b = 0;
	uint32_t color;
	color = (b << fb->blue_mask_shift) | (g << fb->green_mask_shift) | (r << fb->red_mask_shift);

	memset(fb_mem, color, fb->height * fb->width);
	printk(PREFIX "Memory initialized.\n");
}

void kinit(BootInfo *bootInfo) {
	printk(PREFIX "%s %s %s %s\n", CONFIG_KERNEL_CNAME, CONFIG_KERNEL_CVER, __DATE__, __TIME__);
	printk(PREFIX "Cmdline: %s\n", bootInfo->cmdline);

	// Starting serial printk
        printk_init_serial();
	printk("\n");

        // Memory initialization
        PrepareMemory(bootInfo);
/*
	// Checking for SMP
	if(bootInfo->smp) {
		printk(PREFIX "Available CPUs: %d\n", bootInfo->cpuCount);
		for (int i = 1; i < bootInfo->cpuCount; i++) {
			printk(PREFIX "Starting CPU %d...", i);
			limine_smp_info *cpuInfo = bootInfo->cpus[i];
			cpuInfo->goto_address = &smpInit;
			printk("OK.\n");
		}
	} else {
		printk(PREFIX "No SMP capabilities available.\n");
	}
*/
	// Saying hello
	printk(PREFIX "Kenrel initialization complete.\n");

}

void smpInit(limine_smp_info info) {

	while(true) {
		asm volatile ("hlt");
	}
}

void restInit() {

	while(true) {
		asm volatile ("hlt");
	}
}
