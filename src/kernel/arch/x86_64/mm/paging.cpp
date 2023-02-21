#include <arch/x86_64/mm/paging.h>

void flush_tlb(uintptr_t address) {
	asm volatile("invlpg (%0)" : : "r" (address));
}
