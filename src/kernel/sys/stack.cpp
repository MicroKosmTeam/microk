#include <stdint.h>
#include <stddef.h>
#include <sys/panik.h>
#include <sys/symbols.h>
#include <sys/printk.h>

#if UINT32_MAX == UINTPTR_MAX
	#define STACK_CHK_GUARD 0xe2dee396
#else
	#define STACK_CHK_GUARD 0x595e9fbd94fda766
#endif

uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

__attribute__((noreturn))
void __stack_chk_fail(void) {
	printk("\n\nWarning!! Stack smashing in the kernel has been detected.\n\n");
	PANIK("Stack smashing detected");
}

/* Assume, as is often the case, that EBP is the first thing pushed. If not, we are in trouble. */
struct stackframe {
	struct stackframe* rbp;
	uint64_t rip;
};

void UnwindStack(int MaxFrames) {
	struct stackframe *stk;
	stk = (stackframe*)__builtin_frame_address(0);
	//asm volatile ("mov %%rbp,%0" : "=r"(stk) ::);
	printk("Stack trace:\n");
	for(unsigned int frame = 0; stk && frame < MaxFrames; ++frame) {
		// Unwind to previous stack frame
		const symbol_t *symbol = lookup_symbol(stk->rip);
		if (symbol != NULL) {
			printk("  0x%x   %s\n", stk->rip, symbol->name);
		} else {
			printk("  0x%x   <unknown>\n", stk->rip);
		}
		stk = stk->rbp;
	}
}
