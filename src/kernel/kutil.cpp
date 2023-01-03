#include <kutil.h>

KernelInfo kInfo;
IDTR idtr;

KernelInfo kinit(BootInfo *bootInfo) {
        // Loading the GDT
        GDTDescriptor gdtDescriptor;
        gdtDescriptor.size = sizeof(GDT) - 1;
        gdtDescriptor.offset = (uint64_t)&DefaultGDT;
        LoadGDT(&gdtDescriptor);

        // Starting printk
        printk_init_serial();
        printk_init_fb(bootInfo->framebuffer, bootInfo->psf1_Font);
        printk("MicroK Loading...\n");

        // Initializing the GlobalAllocator with EFI Memory data
        GlobalAllocator = PageFrameAllocator();
        GlobalAllocator.ReadEFIMemoryMap(bootInfo->mMap, bootInfo->mMapSize, bootInfo->mMapDescSize);
   
        // Locking kernel pages
        kInfo.kernel_size = (uint64_t)&kernel_end - (uint64_t)&kernel_start;
        uint64_t kernel_pages = (uint64_t)kInfo.kernel_size / 4096 + 1;
        GlobalAllocator.LockPages(&kernel_start, kernel_pages);

        // New page table
        PageTable *PML4 = (PageTable*)GlobalAllocator.RequestPage();
        memset(PML4, 0, 0x1000);
        PageTableManager newPageTableManager = PageTableManager(PML4);
        kInfo.pageTableManager = &newPageTableManager;

        // Identity mapping
        uint64_t mMapEntries = bootInfo->mMapSize / bootInfo->mMapDescSize;
        uint64_t memSize = get_memory_size(bootInfo->mMap, mMapEntries, bootInfo->mMapDescSize);
        for (uint64_t i = 0; i < memSize; i+= 0x1000) {
                kInfo.pageTableManager->MapMemory((void*)i, (void*)i);
        }

        // Make sure the framebuffer is in the correct page
        uint64_t fbBase = (uint64_t)bootInfo->framebuffer->BaseAddress;
        uint64_t fbSize = (uint64_t)bootInfo->framebuffer->BufferSize + 0x1000;
        GlobalAllocator.LockPages((void*)fbBase, fbSize / 0x1000 + 1);
        for (uint64_t i = fbBase; i < fbSize; i+=0x1000) {
                kInfo.pageTableManager->MapMemory((void*)i, (void*)i);
        }

        // Loading the page table
        asm ("mov %0, %%cr3" : : "r" (PML4));

        // Clearing framebuffer
        GlobalRenderer.print_set_color(0xf0f0f0f0, 0x0f0f0f0f);
        GlobalRenderer.print_clear();
        
        // Preparing interrupts
        idtr.Limit = 0x0FFF;
        idtr.Offset = (uint64_t)GlobalAllocator.RequestPage();

        // Insert the Page fault
        IDTDescEntry *int_PageFault = (IDTDescEntry*)(idtr.Offset + 0xE * sizeof(IDTDescEntry));
        int_PageFault->SetOffset((uint64_t)PageFault_handler);
        int_PageFault->type_attr = IDT_TA_InterruptGate;
        int_PageFault->selector = 0x08;
        
        // Load the IDT
        asm("lidt %0" : : "m" (idtr));

        return kInfo;
}
