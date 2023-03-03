#pragma once
#include <stdint.h>
#include <init/kinfo.hpp>
#include <limine.h>

void memcpy(void *dest, void *src, size_t n);
void memset(void *start, uint8_t value, uint64_t num);
int memcmp(const void* buf1, const void* buf2, size_t count);

void *Malloc(size_t size);
void Free(void *p);

void *operator new(size_t size);
void *operator new[](size_t size);
void operator delete(void* p);
void operator delete(void* p, size_t size);
void operator delete[](void* p);
void operator delete[](void* p, size_t size);

extern bool sseEnabled;

extern volatile limine_hhdm_request hhdmRequest;

namespace MEM {
	void Init(KInfo *info);
}
