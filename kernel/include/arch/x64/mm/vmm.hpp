#pragma once
#include <init/kinfo.hpp>

#define PTE_PRESENT (1 << 0)
#define PTE_WRITEABLE (1 << 1)
#define PTE_USER (1 << 2)
#define PTE_WRITE_THROUGH (1 << 3)
#define PTE_CACHE_DISABLED (1 << 4)
#define PTE_ACCESSED (1 << 5)
#define PTE_DIRTY (1 << 6)
#define PTE_PAT (1 << 7)
#define PTE_GLOBAL (1 << 8)
#define PTE_NX (1 << 63)

#define PTE_ADDR_MASK 0x000ffffffffff000
#define PTE_GET_ADDR(VALUE) ((VALUE) & PTE_ADDR_MASK)
#define PTE_GET_FLAGS(VALUE) ((VALUE) & ~PTE_ADDR_MASK)

namespace x86_64 {
	void InitVMM(KInfo *info);
}
