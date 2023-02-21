#pragma once
#include <stdint.h>

extern "C" void PushPageDir(uint64_t *pagedir);
extern "C" void invalidate_tlb();
void flush_tlb(uintptr_t address);

