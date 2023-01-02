#include <stdint.h>
#include <mm/efimem.h>
#include <dev/fb/gop.h>
#include <sys/printk.h>
#include <sys/cstr.h>
#include <mm/memory.h>
#include <mm/pageframe.h>
#include <mm/bitmap.h>

extern uint64_t kernel_start, kernel_end;

struct BootInfo {
	Framebuffer* framebuffer;
	PSF1_FONT* psf1_Font;
	EFI_MEMORY_DESCRIPTOR* mMap;
	uint64_t mMapSize;
	uint64_t mMapDescSize;
};

/* 
for (int i = 0; i < mMapEntries; i++){
                EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)bootInfo->mMap + (i * bootInfo->mMapDescSize));
                newRenderer.print_str(EFI_MEMORY_TYPE_STRINGS[desc->type]);
                newRenderer.print_set_color(0xffff00ff, 0x00000000);
                newRenderer.print_str(" ");
                newRenderer.print_str(to_string(desc->numPages * 4096 / 1024));
                newRenderer.print_str(" KB\n");
                newRenderer.print_set_color(0xffffffff, 0x00000000);
        }
*/

extern "C" void _start(BootInfo* bootInfo){
        GOP newRenderer = GOP(bootInfo->framebuffer, bootInfo->psf1_Font);
        PageFrameAllocator newAllocator;
        newAllocator.ReadEFIMemoryMap(bootInfo->mMap, bootInfo->mMapSize, bootInfo->mMapDescSize);

        printk_init_serial();
   
        printk("Test!");

        uint64_t kernel_size = (uint64_t)&kernel_end - (uint64_t)&kernel_start;
        uint64_t kernel_pages = (uint64_t)kernel_size / 4096 + 1;

        newAllocator.LockPages(&kernel_start, kernel_pages);

        newRenderer.print_str("Kernel is ");
        newRenderer.print_str(to_string(kernel_size / 1024));
        newRenderer.print_str("kb.\n");
        newRenderer.print_str("Free memory: ");
        newRenderer.print_str(to_string(newAllocator.GetFreeMem() / 1024));
        newRenderer.print_str("kb\nUsed memory: ");
        newRenderer.print_str(to_string(newAllocator.GetUsedMem() / 1024));
        newRenderer.print_str("kb\nReserved memory: ");
        newRenderer.print_str(to_string(newAllocator.GetReservedMem() / 1024));
        newRenderer.print_str("kb\n");

        for (int i = 0; i < 20; i++) {
                void *address = newAllocator.RequestPage();
                newRenderer.print_str(to_hstring((uint64_t)address));
                newRenderer.print_str("\n");
        }



        while(true) {
                asm("hlt");
        }
}
