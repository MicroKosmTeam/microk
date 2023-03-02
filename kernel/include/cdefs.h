#pragma once

#define CONFIG_KENREL_BASE (0xffffffff80000000)
#define CONFIG_HEAP_BASE (0xffffffff90000000)
#define CONFIG_SYMBOL_TABLE_BASE (0xffffffffffff0000)

#define CONFIG_SYMBOL_TABLE_PAGES (2)
#define CONFIG_HEAP_SIZE (64 * 1024 * 1024) // 64MB kernel heap
#define CONFIG_STACK_SIZE (4 * 1024 * 1024) // 4MB stack
#define CONFIG_BOOTMEM_SIZE (64 * 1024) // 64kb bootmem

