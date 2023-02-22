#include <dev/serial/serial.h>
#include <cdefs.h>
#include <stdarg.h>

#ifdef CONFIG_ARCH_x86_64
#include <arch/x86_64/io/io.h>
#endif

uint64_t UARTDevice::ioctl(uint64_t request, ...) {
	va_list ap;
        va_start(ap, request);

	uint64_t result;

	switch (request) {
		case 0: // Init();
			result = Init(va_arg(ap, int));
			break;
		case 1: // PutChar();
			PutChar((const char)va_arg(ap, int));
			result = 0;
			break;
		case 2: // PutStr();
			PutStr((const char*)va_arg(ap, int*));
			result = 0;
			break;
		case 3: // GetChar();
			result = GetChar();
			break;
		default:
			result = 0;
	}

        va_end(ap);

	return result;
}

uint64_t UARTDevice::Init(SerialPorts serialPort) {
	port = (int)serialPort;

        outb(port + 1, 0x00);    // Disable all interrupts
        outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
        outb(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
        outb(port + 1, 0x00);    //                  (hi byte)
        outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
        outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
        outb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
        outb(port + 4, 0x1E);    // Set in loopback mode, test the serial chip
        outb(port + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

        // Check if serial is faulty (i.e: not same byte as sent)
        if(inb(port + 0) != 0xAE) {
		active = false;
                return 1;
        }

        // If serial is not faulty set it in normal operation mode
        // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
        outb(port + 4, 0x0F);

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

        outb(port, ch);
}

int UARTDevice::GetChar() {
	if(!active) return;
	while (serialReceived() == 0);

	return inb(port);
}

int UARTDevice::isTransmitEmpty() {
	return inb(port + 5) & 0x20;
}

int UARTDevice::serialReceived() {
	return inb(port + 5) & 1;
}
