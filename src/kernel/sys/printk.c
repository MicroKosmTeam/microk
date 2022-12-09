#include <sys/printk.h>

void printk_init() {
#ifdef KCONSOLE_SERIAL
        if(serial_init(COM1) != 0) {
                return;
        }
#endif
#ifdef KCONSOLE_VGA
        vga_print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
        vga_print_clear();
#endif
#ifdef KCONSOLE_VBE
        vbe_print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
        vbe_print_clear();
#endif
#ifdef KCONSOLE_GOP
        gop_print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
        gopprint_clear();
#endif
}

static void print_all(char *string) {
#ifdef KCONSOLE_SERIAL
        serial_print_str(string);
#endif
#ifdef KCONSOLE_VGA
        vga_print_str(string);
#endif
#ifdef KCONSOLE_VBE
        vbe_print_str(string);
#endif
#ifdef KCONSOLE_GOP
        gop_print_str(string);
#endif
}

static void print_all_char(char ch) {
#ifdef KCONSOLE_SERIAL
        serial_print_char(ch);
#endif
#ifdef KCONSOLE_VGA
        vga_print_char(ch);
#endif
#ifdef KCONSOLE_VBE
        vbe_print_char(ch);
#endif
#ifdef KCONSOLE_GOP
        gop_print_char(ch);
#endif
}


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
                                        print_all(va_arg(ap, char *));
                                        break;
                                case 'd':
                                case 'u':
                                        itoa(buf, 'd', va_arg(ap, long long int));
                                        print_all(buf);
                                        break;
                                case 'x':
                                        itoa(buf, 'x', va_arg(ap, long long int));
                                        print_all(buf);
                                        break;


                        }
                } else {
                        print_all_char(*ptr++);
                }
        }

        va_end(ap);
}
