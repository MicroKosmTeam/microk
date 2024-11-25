#pragma once
#include <stdint.h>
#include <stddef.h>
#include <mkmi.h>

struct heap_header {
	heap_header *previous, *next;
	usize size;
	bool used;
}__attribute__((packed));

void init_heap(void *addr, usize size);
void *malloc(usize size);
void free(void *ptr);
void dump_hdrs();
