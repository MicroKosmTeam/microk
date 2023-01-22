#include <sys/printk.h>

#ifdef PRINTK
#include <stdio.h>

#ifdef KCONSOLE_GOP
GOP GlobalRenderer;
#endif

#ifdef KCONSOLE_GOP
#ifdef __cplusplus
void printk_init_fb(Framebuffer *framebuffer, PSF1_FONT *psf1_font) {
        GlobalRenderer = GOP(framebuffer, psf1_font);
}
void GOP_print_str(const char *str) {
        GlobalRenderer.print_str(str);
}
void GOP_print_char(const char ch) {
        GlobalRenderer.print_char(ch);
}
void GOP_print_pixel(int x, int y, uint32_t color) {
        GlobalRenderer.put_pixel(x, y, color);
}
#endif

#endif
#ifdef __cplusplus
extern "C" {
#endif

void print_image(const uint8_t *data) {
        char *ptr = strtok((char*)data, "\n");
        printk("%s\n", ptr);

        uint16_t height, width;

        ptr = strtok(NULL, " ");
        width = atoi(ptr);
        ptr = strtok(NULL, "\n");
        height = atoi(ptr);

        printk("Width: %d\nHeight: %d\n", width, height);

        ptr = strtok(NULL, "\n");

        for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                        uint8_t r = atoi(strtok(NULL, "\n"));
                        uint8_t g = atoi(strtok(NULL, "\n"));
                        uint8_t b = atoi(strtok(NULL, "\n"));
                        uint32_t color = (b | (g << 8) | (r << 16) | (0xff << 24));
                        //printk("Color : %d %d %d 0x%x\n", r, g, b, color);
                        GOP_print_pixel(x, y, color);
                }
        }
}

#ifdef KCONSOLE_SERIAL
void printk_init_serial() {
        if(serial_init(COM1) != 0) {
                return;
        }
}
#endif

static void print_all(const char *string) {
        #ifdef KCONSOLE_SERIAL
                serial_print_str(string);
        #endif
        #ifdef KCONSOLE_GOP
                GOP_print_str(string);
        #endif
}

static void print_all_char(const char ch) {
        #ifdef KCONSOLE_SERIAL
                serial_print_char(ch);
        #endif
        #ifdef KCONSOLE_GOP
                GOP_print_char(ch);
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
                                case '%':
                                        print_all_char('%');
                                        break;
                                case 'c':
                                        print_all_char((char)(va_arg(ap, int)));
                                        break;

                        }
                } else {
                        print_all_char(*ptr++);
                }
        }

        va_end(ap);
}

#ifdef __cplusplus
}
#endif

#endif
