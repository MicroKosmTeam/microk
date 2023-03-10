#include <stdint.h>
#include <stddef.h>
#include <cdefs.h>

void PrintChar(uint8_t ch) {
	const uint16_t port = 0x3f8;
	asm volatile ("outb %0, %1" : : "a"(ch), "d"(port));
}

void PrintString(const char *str) {
	for (size_t i = 0; 1; i++) {
                char character = (uint8_t) str[i];

                if (character == '\0') {
                        return;
                }

                PrintChar(character);
        }
}
uint64_t *KRNLSYMTABLE;
extern "C" int _start(void) {
	PrintChar('O');
	PrintChar('K');
	PrintChar('\r');
	PrintChar('\n');

	PrintString("Hello, world!\r\n");

	KRNLSYMTABLE = CONFIG_SYMBOL_TABLE_BASE;
	void (*kernelPrintk)(char *format, ...) = KRNLSYMTABLE[0];
	void* (*kernelMalloc)(size_t size) = KRNLSYMTABLE[1];
	void (*kernelFree)(void *p) = KRNLSYMTABLE[2];

	PrintString("Now printing from kernel...\r\n");
	kernelPrintk("Hello from kernel printk!\r\n");

	uint64_t *kernel = kernelMalloc(10);
	*kernel = 0x69;
	kernelPrintk("Allocation kernel: 0x%x (at 0x%x)\r\n", *kernel, kernel);
	kernelFree(kernel);

	PrintString("Bye!\n\r");

	return 1;
}
