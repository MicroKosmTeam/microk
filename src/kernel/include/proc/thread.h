#pragma once
#include <stdint.h>

struct thread {
	uint64_t tid;
	void *stack_ptr;
	uint64_t state;
};


struct thread *new_thread(void (*function)(void));
extern "C" void switch_stack(void *old_ptr, void *new_ptr);

