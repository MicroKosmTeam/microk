#include <sys/printk.hpp>
#ifdef CONFIG_PRINTK
#include <stdarg.h>
#include <stdint.h>
#include <mm/bootmem.hpp>
#include <mm/string.hpp>

UARTDevice *kernelPort;

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

#ifdef CONFIG_HW_SERIAL
void EarlyInit(KInfo *info) {
	info->kernelPort = (UARTDevice*)BOOTMEM::Malloc(sizeof(UARTDevice) + 1);
	info->kernelPort->Init(COM1);
	info->kernelPort->PutStr("Serial PrintK started.\n");
	kernelPort = info->kernelPort;
}
#endif
}

#endif
