#pragma once
#include <stdint.h>
#include <init/kinfo.hpp>

void memset(void *start, uint8_t value, uint64_t num);
void *operator new(size_t size);
void *operator new[](size_t size);
void operator delete(void* p);

namespace MEM {
	void Init(KInfo *info);
}
