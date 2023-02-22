#include <proc/thread.h>
#include <mm/pageframe.h>
#include <mm/memory.h>
#include <cdefs.h>

struct swtch_stack{
	uint64_t RBP;
	uint64_t RBX;
	uint64_t R12;
	uint64_t R13;
	uint64_t R14;
	uint64_t R15;
	uint64_t RBP2;
	uint64_t ret;
}__attribute__((packed))__attribute__((align(0x1000)));

uint64_t next_tid = 1;
struct thread *new_thread(void (*function)(void)) {
	struct thread *thrd = GlobalAllocator.RequestPage(); //vmalloc(sizeof(thread));
	thrd->tid = next_tid++;
	thrd->stack_ptr = GlobalAllocator.RequestPage(); //vmalloc(THREAD_STACK_SIZE + 1);

	struct swtch_stack *stk;
	stk = thrd->stack_ptr;
	stk->RBP = (uint64_t)&stk->RBP2;
	stk->ret = (uint64_t)function;
	return thrd;
}
