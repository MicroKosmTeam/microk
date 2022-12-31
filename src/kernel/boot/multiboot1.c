#include <multiboot1.h>
#include <sys/panik.h>
#include <sys/printk.h>

#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

typedef multiboot_memory_map_t mmap_entry_t;

int parse_multiboot1(multiboot_info_t *mbi) {
        PANIK("Multiboot 1 Protocol not yet implemented.");
}

int multiboot1_get_memory_area(size_t count, uintptr_t *start, uintptr_t *end, uint32_t *type) {
        PANIK("Multiboot 1 Protocol not yet implemented.");
}

int multiboot1_page_used(uintptr_t start) {
        PANIK("Multiboot 1 Protocol not yet implemented.");
}
