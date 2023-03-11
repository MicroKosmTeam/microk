/*
   File: arch/x64/io/io.cpp
*/

#include <arch/x64/io/io.hpp>

namespace x86_64 {
/* Puts out a byte to the IO bus */
void OutB(uint16_t port, uint8_t val) {
        asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

/* Puts out a word to the IO bus */
void OutW(uint16_t port, uint16_t val) {
        asm volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}

/* Gets a byte from the IO bus */
uint8_t InB(uint16_t port) {
        uint8_t ret;
        asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
        return ret;
}

/* Gets a word from the IO bus */
uint16_t InW(uint16_t port) {
        uint16_t ret;
        asm volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) );
        return ret;
}

/* Wastes a cycle of the IO bus */
void IoWait(void) {
        OutB(0x80, 0);
}
}
