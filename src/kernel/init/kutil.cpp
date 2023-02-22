#include <cdefs.h>
#include <init/kutil.h>
#include <sys/icxxabi.h>
#include <sys/cstr.h>
#include <mm/memory.h>
#include <sys/panik.h>
#include <sys/printk.h>
#include <mm/heap.h>
#include <dev/pci/pci.h>

#define PREFIX "[KINIT] "

#ifdef CONFIG_ARCH_x86_64
#include <arch/x86_64/init.h>
#include <arch/x86_64/cpu/cpu.h>
#include <arch/x86_64/mm/vmm.h>
#endif

uintptr_t kernelStack;
static void PrepareMemory(BootInfo *bootInfo) {
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
//	uint64_t heapAddress = bootInfo->hhdmOffset + 0x000000200000000000;
	uint64_t heapAddress = 0x00007FFFFFFFFFFF - 0xFFFFFFFFFFF;
	uint64_t heapPages = 0x100;
	InitializeHeap(heapAddress, heapPages);

	printk(PREFIX "Memory initialized.\n");
}

#include <dev/device.h>
#include <dev/serial/serial.h>

static void PrepareDevices(BootInfo *bootInfo) {
	// Starting serial printk
	rootNode = new UARTDevice();
	rootNode->ioctl(0,SerialPorts::COM1);
	rootNode->ioctl(2,"Hello, world.\n");
        printk_init_serial(rootNode);


	// Loading PCI devices
        ACPI::SDTHeader *xsdt = (ACPI::SDTHeader*)(bootInfo->rsdp->XSDTAddress);
        printk(PREFIX "Loading the MCFG table...\n");
        ACPI::MCFGHeader *mcfg = (ACPI::MCFGHeader*)ACPI::FindTable(xsdt, (char*)"MCFG");
        printk(PREFIX "Enumerating PCI devices...\n");
        PCI::EnumeratePCI(mcfg);
}

void kinit(BootInfo *bootInfo) {
	printk("\n");
	printk(PREFIX "%s %s %s %s\n", CONFIG_KERNEL_CNAME, CONFIG_KERNEL_CVER, __DATE__, __TIME__);
	printk(PREFIX "Cmdline: %s\n", bootInfo->cmdline);

        // Memory initialization
        PrepareMemory(bootInfo);

	// Device initialization
	PrepareDevices(bootInfo);

	// Checking for SMP
	if(bootInfo->smp) {
		printk(PREFIX "Available CPUs: %d\n", bootInfo->cpuCount);
		for (int i = 1; i < bootInfo->cpuCount; i++) {
			printk(PREFIX "Starting CPU %d...", i);
			bootInfo->cpus[i]->goto_address = &smpInit;
			printk("OK.\n");
		}
	} else {
		printk(PREFIX "No SMP capabilities available.\n");
	}

	// Saying hello
	printk(PREFIX "Kernel initialization complete.\n");
}

void smpInit(limine_smp_info *info) {
	asm volatile ("cli");
	while(true) {
		asm volatile ("hlt");
	}
}

void restInit() {

	while(true) {
		asm volatile ("hlt");
	}
}
