#include <stdarg.h>
#include <stdint.h>
#include <sys/printk.hpp>
#include <mm/bootmem.hpp>
#include <dev/uart/uart.hpp>

UARTDevice *kernelPort;
void itoa (char *buf, int base, long long int d) {
        char *p = buf;
        char *p1, *p2;
        unsigned long long ud = d;
        int divisor = 10;

        /*  If %d is specified and D is minus, put ‘-’ in the head. */
        if (base == 'd' && d < 0) {
                *p++ = '-';
                buf++;
                ud = -d;
        } else if (base == 'x')
                divisor = 16;

        /*  Divide UD by DIVISOR until UD == 0. */
        do {
                int remainder = ud % divisor;
                *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
        } while (ud /= divisor);

        /*  Terminate BUF. */
        *p = 0;

        /*  Reverse BUF. */
        p1 = buf;
        p2 = p - 1;

        while (p1 < p2) {
                char tmp = *p1;
                *p1 = *p2;
                *p2 = tmp;
                p1++;
                p2--;
        }
}


namespace PRINTK {
void PrintK(char *format, ...) {
        va_list ap;
        va_start(ap, format);

        char *ptr = format;
        char buf[128];

        while(*ptr) {
                if (*ptr == '%') {
                        ptr++;
                        switch (*ptr++) {
                                case 's':
					kernelPort->PutStr(va_arg(ap, char *));
                                        break;
                                case 'd':
                                case 'u':
                                        itoa(buf, 'd', va_arg(ap, int64_t));
                                        kernelPort->PutStr(buf);
                                        break;
                                case 'x':
                                        itoa(buf, 'x', va_arg(ap, int64_t));
                                        kernelPort->PutStr(buf);
                                        break;
                                case '%':
                                        kernelPort->PutChar('%');
                                        break;
                                case 'c':
                                        kernelPort->PutChar((va_arg(ap, int32_t)));
                                        break;

                        }
                } else {
                        kernelPort->PutChar(*ptr++);
                }
        }

        va_end(ap);
}

void EarlyInit() {
	kernelPort = (UARTDevice*)BOOTMEM::Malloc(sizeof(UARTDevice) + 1);
	kernelPort->Init(COM1);
	kernelPort->PutStr("Serial PrintK started.\n");
}
}
