#include <stdint.h>

void _start(void) {
	while(1) {
		asm volatile ("hlt");
	}

	uint64_t *KRNLSYMTABLE = 0xffffffffffff0000;
	void (*testPrintk)(char *format, ...) = KRNLSYMTABLE[0];

	testPrintk("Hello, world.\r\n");

	while(1) {
		asm volatile ("hlt");
	}
}

