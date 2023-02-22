#include <init/kutil.h>
#include <cdefs.h>
#include <mm/pageframe.h>
#include <mm/bitmap.h>
#include <mm/pageindexer.h>
#include <mm/pagetable.h>
#include <mm/paging.h>
#include <mm/heap.h>
#include <io/io.h>
#include <dev/8259/pic.h>
#include <dev/tty/gpm/gpm.h>
#include <dev/tty/tty.h>
#include <dev/pci/pci.h>
#include <dev/timer/pit/pit.h>
#include <fs/fs.h>
#include <sys/icxxabi.h>
#include <fs/vfs.h>
#include <sys/cstr.h>
#include <module/module.h>
#include <mm/memory.h>
#include <sys/panik.h>
#include <sys/printk.h>
#include <fs/ustar/ustar.h>
#include <proc/scheduler.h>

#ifdef CONFIG_ARCH_x86_64
#include <arch/x86_64/init.h>
#include <arch/x86_64/cpu.h>
#endif

#define PREFIX "[KINIT] "

uintptr_t kernelStack;
KernelInfo kInfo;

void PrepareACPI(BootInfo *bootInfo) {
        ACPI::SDTHeader *xsdt = (ACPI::SDTHeader*)(bootInfo->rsdp->XSDTAddress);

        int entries = (xsdt->Length - sizeof(ACPI::SDTHeader)) / 8;
        dprintf(PREFIX "Available ACPI tables: %d\n", entries);

        dprintf(PREFIX "Loading the FADT table...\n");
        ACPI::FindTable(xsdt, (char*)"FADT");

        dprintf(PREFIX "Loading the MADT table...\n");
        ACPI::FindTable(xsdt, (char*)"MADT");

        dprintf(PREFIX "Loading the MCFG table...\n");
        ACPI::MCFGHeader *mcfg = (ACPI::MCFGHeader*)ACPI::FindTable(xsdt, (char*)"MCFG");

        dprintf(PREFIX "Loading the HPET table...\n");
        ACPI::FindTable(xsdt, (char*)"HPET");

        dprintf(PREFIX "Enumerating PCI devices...\n");
        PCI::EnumeratePCI(mcfg);
}

void PrepareMemory(BootInfo *bootInfo) {
#ifdef CONFIG_ARCH_x86_64
	// Loading the GDT
	printk(PREFIX "Loading the GDT...\n");
	x86_64::LoadGDT();
#endif
	printk(PREFIX "Stack at: 0x%x\n", kernelStack);
	printk(PREFIX "HHDM at: 0x%x\n", bootInfo->hhdmOffset);
	printk(PREFIX "Framebuffers: %d\n", bootInfo->framebufferCount);
	/*Framebuffer *fb = bootInfo->framebuffers[0];
	printk(PREFIX "%d x %d, %d-bit color. R: %d G: %d B: %d\n", fb->width, fb->height, fb->bpp, fb->red_mask_shift, fb->green_mask_shift, fb->blue_mask_shift);
	uint32_t *fb_mem = fb->address;
	uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
	uint32_t color;
	color = (b << fb->blue_mask_shift) | (g << fb->green_mask_shift) | (r << fb->red_mask_shift);

	memset(fb_mem, color, fb->height * fb->width);*/

	printk(PREFIX "Modules: %d\n", bootInfo->moduleCount);
	for (int i = 0; i < bootInfo->moduleCount; i++) {
		printk(PREFIX " - Module: [0x%x - 0x%x] %s %s\n", bootInfo->modules[i]->address, bootInfo->modules[i]->address + bootInfo->modules[i]->size, bootInfo->modules[i]->path, bootInfo->modules[i]->cmdline);
	}
	// Initializing the GlobalAllocator with EFI Memory data
	printk(PREFIX "Initializing the page frame allocator...\n");
        GlobalAllocator = PageFrameAllocator();
        GlobalAllocator.ReadMemoryMap(bootInfo->mMap, bootInfo->mMapEntries);

	/*
        // Locking kernel pages
        kInfo.kernelSize = (uint64_t)&kernel_end - (uint64_t)&kernel_start;
        uint64_t kernel_pages = (uint64_t)kInfo.kernelSize / 4096 + 1;
        GlobalAllocator.LockPages(&kernel_start, kernel_pages);

	// Locking initrd pages
        uint64_t initrd_pages = (uint64_t)kInfo.initrdSize / 4096 + 1;
        GlobalAllocator.LockPages(kInfo.initrd, initrd_pages);
	*/

#ifdef CONFIG_ARCH_x86_64
        // Interrupt initialization
	printk(PREFIX "Loading interrupts...\n");
	x86_64::PrepareInterrupts(bootInfo);

	printk(PREFIX "Setting up x86_64 paging...\n");
	x86_64::SetupPaging(bootInfo);

        // Init CPU Features
	printk(PREFIX "Setting up x86_64 CPU Features...\n");
        x86_64::CPUInit();

	// Setting the timer frequency
	// printk(PREFIX "Starting the PIT...\n");
        // PIT::SetFrequency(1000);
        // printk(PREFIX "PIT Frequency: %d\n", PIT::GetFrequency());
#endif
	// Init heap
        printk(PREFIX "Initializing the heap...\n");
        InitializeHeap((void*)0xffff900000000000, 0x100);
	printk(PREFIX "Done setting up memory.\n");
	return;
}

void kinit(BootInfo *bootInfo) {
	//kInfo.initrd = bootInfo->initrdData;
	//kInfo.initrdSize = bootInfo->initrdSize;

	// Starting serial printk
        printk_init_serial();
	printk("\n"PREFIX "%s %s %s %s\n", CONFIG_KERNEL_CNAME, CONFIG_KERNEL_CVER, __DATE__, __TIME__);

        // Memory initialization
        PrepareMemory(bootInfo);

	printk("OK\n");
	uint8_t *alloc = malloc(128);
	printk("0x%x\n", alloc);
	alloc = malloc(128);
	printk("0x%x\n", alloc);

	while(true) {
		asm("hlt");
	}

	// Starting the VFS subsystem
	VFS_Init();

	// Initializing the framebuffer
	printk_init_fb((Framebuffer*)bootInfo->framebuffers[0], /*bootInfo->psf1_Font*/NULL);
        printk(PREFIX "MicroK Loading...\n");
	GlobalRenderer.BufferSwitch();

        // Clearing framebuffer
        GlobalRenderer.print_set_color(0xf0f0f0f0, 0x0f0f0f0f);
        GlobalRenderer.print_clear();

        // Starting the modules subsystem
        GlobalModuleManager = new ModuleManager();

        // Starting the filesystem Manager
        GlobalFSManager = new Filesystem::FSManager();

        // Initializing a TTY
        GlobalTTY = new TTY();

	// Load and parse the initrd
        USTAR::LoadArchive(bootInfo->initrdData);
        USTAR::ReadArchive();

	// ACPI initialization
        PrepareACPI(bootInfo);
	/*
        size_t size;
        USTAR::GetFileSize("module.elf", &size);
        dprintf(PREFIX "Size of module.elf: %d\n", size);
        uint8_t *buffer = (uint8_t*)malloc(size);
        memset(buffer, 0, size);
        if(USTAR::ReadFile("module.elf", &buffer, size)) {
                dprintf(PREFIX "Sending it to the module manager...\n");
                GlobalModuleManager->LoadELF((uint8_t*)buffer);
        } else {
                dprintf(PREFIX "Failed to read module.elf on initrd.\n");
        }

        free(buffer);
        */
}

void restInit() {
        GlobalRenderer.print_clear();

        printf(" __  __  _                _  __    ___   ___\n"
               "|  \\/  |(_) __  _ _  ___ | |/ /   / _ \\ / __|\n"
               "| |\\/| || |/ _|| '_|/ _ \\|   <   | (_) |\\__ \\\n"
               "|_|  |_||_|\\__||_|  \\___/|_|\\_\\   \\___/ |___/\n"
               "The operating system from the future...at your fingertips.\n"
               "\n"
               " Memory Status:\n"
               " -> Kernel:      %dkb.\n"
	       " -> Initrd:      %dkb.\n"
               " -> Free:        %dkb.\n"
               " -> Used:        %dkb.\n"
               " -> Reserved:    %dkb.\n"
               " -> Total:       %dkb.\n"
               "\n"
	       "Continuing startup...\n",
                kInfo.kernelSize / 1024,
		kInfo.initrdSize / 1024,
                GlobalAllocator.GetFreeMem() / 1024,
                GlobalAllocator.GetUsedMem() / 1024,
                GlobalAllocator.GetReservedMem() / 1024,
                (GlobalAllocator.GetFreeMem() + GlobalAllocator.GetUsedMem()) / 1024);

	GlobalTTY->Activate();

	init_scheduler();
	start_scheduler();

        printf("Done!\n");

	stop_scheduler();

        while (true) {
                asm("hlt");
        }

        PANIK("Reached the end of kernel operations.");
}
