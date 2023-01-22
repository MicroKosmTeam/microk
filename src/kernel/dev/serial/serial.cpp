#include <dev/serial/serial.h>

#ifdef KCONSOLE_SERIAL

#include <io/io.h>

int port;

#ifdef __cplusplus
extern "C" {
#endif

static int is_transmit_empty() {
        return inb(port + 5) & 0x20;
}

int serial_init(const int set_port) {
        port = set_port;
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
                return 1;
        }
 
        // If serial is not faulty set it in normal operation mode
        // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
        outb(port + 4, 0x0F);
        return 0;
}

void serial_print_str(const char* str) {
        for (size_t i = 0; 1; i++) {
                char character = (uint8_t) str[i];

                if (character == '\0') {
                        return;
                }

                if (character == '\n') {
                        serial_print_char('\n');
                        serial_print_char('\r');
                        return;
                }


                serial_print_char(character);
        }
}

void serial_print_char(const char ch) {
        while (is_transmit_empty() == 0);
        
        outb(port, ch);
}

#ifdef __cplusplus
}
#endif

#endif
