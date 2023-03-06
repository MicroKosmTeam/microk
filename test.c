#include <stdint.h>
#include <stddef.h>

void PrintChar(uint8_t ch) {
	const uint16_t port = 0x3f8;
	asm volatile ("outb %0, %1" : : "a"(ch), "d"(port));
}

int _start(void) {
	for (int i = 0; i < 10; i++) {
		PrintChar('H');
                PrintChar('e');
                PrintChar('l');
                PrintChar('l');
                PrintChar('o');
                PrintChar(' ');
                PrintChar('E');
                PrintChar('L');
                PrintChar('F');
                PrintChar('!');
                PrintChar('\r');
                PrintChar('\n');
	}

	return 1;
}

