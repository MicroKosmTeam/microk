#include <arch/x64/io/io.hpp>
#include <dev/uart/uart.hpp>
#include <stdarg.h>

uint64_t UARTDevice::Init(SerialPorts serialPort) {
	port = (int)serialPort;

        x86_64::OutB(port + 1, 0x00);    // Disable all interrupts
        x86_64::OutB(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
        x86_64::OutB(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
        x86_64::OutB(port + 1, 0x00);    //                  (hi byte)
        x86_64::OutB(port + 3, 0x03);    // 8 bits, no parity, one stop bit
        x86_64::OutB(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
        x86_64::OutB(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
        x86_64::OutB(port + 4, 0x1E);    // Set in loopback mode, test the serial chip
        x86_64::OutB(port + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

        // Check if serial is faulty (i.e: not same byte as sent)
        if(x86_64::InB(port + 0) != 0xAE) {
		active = false;
                return 1;
        }

        // If serial is not faulty set it in normal operation mode
        // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
        x86_64::OutB(port + 4, 0x0F);

	active = true;
        return 0;
}

void UARTDevice::PutStr(const char* str) {
	if (!active) return;
        for (size_t i = 0; 1; i++) {
                char character = (uint8_t) str[i];

                if (character == '\0') {
                        return;
                }

                if (character == '\n') {
                        PutChar('\n');
                        PutChar('\r');
                        return;
                }


                PutChar(character);
        }
}

void UARTDevice::PutChar(const char ch) {
	if (!active) return;
        while (isTransmitEmpty() == 0);

        x86_64::OutB(port, ch);
}

int UARTDevice::GetChar() {
	if(!active) return 0;
	while (serialReceived() == 0);

	return x86_64::InB(port);
}

int UARTDevice::isTransmitEmpty() {
	return x86_64::InB(port + 5) & 0x20;
}

int UARTDevice::serialReceived() {
	return x86_64::InB(port + 5) & 1;
}
