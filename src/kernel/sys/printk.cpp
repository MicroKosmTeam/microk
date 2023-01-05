#include <sys/printk.h>

GOP GlobalRenderer;

void printk_init_serial() {
        if(serial_init(COM1) != 0) {
                return;
        }
}

void printk_init_fb(Framebuffer *framebuffer, PSF1_FONT *psf1_font) {
        GlobalRenderer = GOP(framebuffer, psf1_font);
}

static void print_all(const char *string) {
        serial_print_str(string);
        GlobalRenderer.print_str(string);
}

static void print_all_char(const char ch) {
        serial_print_char(ch);
        GlobalRenderer.print_char(ch);
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
                                case '%':
                                        print_all_char('%');


                        }
                } else {
                        print_all_char(*ptr++);
                }
        }

        va_end(ap);
}
