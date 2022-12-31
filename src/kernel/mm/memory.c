#include <mm/memory.h>
#include <multiboot.h>
#include <stdio.h>

void *BootP4; //TMP fix, do not use
uint64_t kernel_P4;

void memory_init() {
        kernel_P4 = (uint64_t)&BootP4;
        uint64_t start, end;
        uint32_t type, i = 0;
        printk("Parsing memory map\n");

        uint64_t free_mem = 0,
                 used_mem = 0,
                 kernel_mem = 0,
                 acpi_mem = 0,
                 nvs_mem = 0,
                 bad_mem = 0;
        
        while(!multiboot_get_memory_area(i++, &start, &end, &type)) {
                printk("0x%x-0x%x (%d)\n", start, end, type);
                
                for(uint64_t p = start; p < end; p += PAGE_SIZE) {
                        if(p >= V2P(&kernel_start) && p < V2P(&kernel_end)) {
                                kernel_mem += PAGE_SIZE;
                                continue;
                        }
                        
                        if(multiboot_page_used(p)) {
                                continue;
                        }

                        uint64_t addr = (uint64_t)P2V(p);
                        uint64_t page = vmm_get_page(kernel_P4, addr);

                        if(!PAGE_EXIST(page) || !(page & PAGE_PRESENT)) {
                                uint16_t flags = PAGE_WRITE;
                                vmm_set_page(kernel_P4, addr, p, flags | PAGE_PRESENT);
                        }

                        switch(type) {
                                case MBOOT_MMAP_FREE:
                                        free_mem += PAGE_SIZE;
                                        pmm_free(p);
                                        break;
                                case MBOOT_MMAP_USED:
                                        used_mem += PAGE_SIZE;
                                        break;
                                case MBOOT_MMAP_ACPI:
                                        acpi_mem += PAGE_SIZE;
                                        break;
                                case MBOOT_MMAP_NVS:
                                        nvs_mem += PAGE_SIZE;
                                        break;
                                case MBOOT_MMAP_BAD:
                                        bad_mem += PAGE_SIZE;
                                        break;
                                default:
                                        break;
                        }
                }
        }

        printk(" ------------------------------------\n"
               "|Total usable memory: %dkb\n"
               "|Free memory: %dkb\n"
               "|Memory reclaimable from ACPI: %dkb\n"
               "|Memory used by the kernel: %dkb\n"
               "|Memory used by the firmware: %dkb\n"
               "|Memory used by ACPI: %dkb\n"
               "|Bad memory: %dkb\n"
               " ------------------------------------\n",
               (free_mem + kernel_mem + acpi_mem + nvs_mem) / 1024,
               free_mem / 1024,
               acpi_mem / 1024,
               kernel_mem / 1024,
               used_mem / 1024,
               (acpi_mem + nvs_mem) / 1024,
               bad_mem / 1024);
}
