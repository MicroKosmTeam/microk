#include <sys/printk.h>
#include <image.h>
GOP GlobalRenderer;

void print_image() {
        for (int j = 0; j < 2; j++) {
                for (int i = 0; pixels[i]>-1;) {
                        int row = pixels[i++];
                        int col = pixels[i++];
                        int r = pixels[i++];
                        int g = pixels[i++];
                        int b = pixels[i++];
                        uint32_t color = b | (g << 8) | (r << 16) | (0xff << 24);

                        GlobalRenderer.put_pixel(row + j * 90, col, color);
                }
        }
}

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
