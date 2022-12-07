#include <printk.h>

void printk(char *format, ...) {
        va_list ap;
        va_start(ap, format);

        char *ptr = format;
        char buf[128];

        while(*ptr) {
                if (*ptr == '%') {
                        ptr++;
                        switch (*ptr++) {
                                case 's':
                                        print_str(va_arg(ap, char *));
                                        break;
                                case 'd':
                                case 'u':
                                        itoa(buf, 'd', va_arg(ap, long long int));
                                        print_str(buf);
                                        break;
                                case 'x':
                                        itoa(buf, 'x', va_arg(ap, long long int));
                                        print_str(buf);
                                        break;


                        }
                } else {
                        print_char(*ptr++);
                }
        }

        va_end(ap);
}

void printk_init() {
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
        print_clear();
}
