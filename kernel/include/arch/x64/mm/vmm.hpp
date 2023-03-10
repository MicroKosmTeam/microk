#pragma once
#include <init/kinfo.hpp>

#define PAGE_SIZE 4096

#define PTE_PRESENT (1ull << 0ull)
#define PTE_WRITABLE (1ull << 1ull)
#define PTE_USER (1ull << 2ull)
#define PTE_NX (1ull << 63ull)

#define PTE_ADDR_MASK 0x000ffffffffff000
#define PTE_GET_ADDR(VALUE) ((VALUE) & PTE_ADDR_MASK)
#define PTE_GET_FLAGS(VALUE) ((VALUE) & ~PTE_ADDR_MASK)

struct Pagemap {
	uint64_t *topLevel;
};

namespace x86_64 {
	void InitVMM(KInfo *info);
}
