#pragma once
#include <stddef.h>

void *kmalloc(size_t, unsigned int);
void kfree(const void*);
size_t ksize(const void*);
