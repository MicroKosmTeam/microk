#include <multiboot.h>
#include <multiboot1.h>
#include <multiboot2.h>

int multiboot_init(uint64_t magic, void *mboot_info) {
        if(magic == MBOOT1_REPLY) {
                kernel_boot_data.multiboot_version = 1;
                parse_multiboot1(mboot_info);
        } else if (magic == MBOOT2_REPLY) {
                kernel_boot_data.multiboot_version = 2;
                parse_multiboot2(mboot_info);
        } else {
                return 1;
        }

        printk("Multiboot version: %d\n", kernel_boot_data.multiboot_version);

        return 0;
}

int multiboot_get_memory_area(size_t count, uintptr_t *start, uintptr_t *end, uint32_t *type) {
        if (kernel_boot_data.multiboot_version == 1) {
                multiboot2_get_memory_area(count, start, end, type);
        } else {
                multiboot2_get_memory_area(count, start, end, type);
        }
}

int multiboot_page_used(uintptr_t start) {
        if (kernel_boot_data.multiboot_version == 1) {
                multiboot1_page_used(start);
        } else {
                multiboot2_page_used(start);
        }
}
