#include <stdint.h>
#include <stddef.h>

void PrintChar(uint8_t ch) {
	const uint16_t port = 0x3f8;
	asm volatile ("outb %0, %1" : : "a"(ch), "d"(port));
}

void _start(void) {
	while(1) {
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

		asm volatile ("hlt");
	}
}

