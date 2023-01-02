#include <sys/printk.h>

void printk_init_serial() {
        if(serial_init(COM1) != 0) {
                return;
        }
}

void printk_init_fb(Framebuffer* new_target_framebuffer, PSF1_FONT* new_psf1_font) {
        //fb = GOP(new_target_framebuffer, new_psf1_font);
}

static void print_all(char *string) {
        serial_print_str(string);
        //fb.print_str(string);
}

static void print_all_char(char ch) {
        serial_print_char(ch);
        //fb.print_char(ch);
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
