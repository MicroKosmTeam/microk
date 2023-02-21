#include <sys/printk.h>
#include <stdio.h>

GOP GlobalRenderer;

bool fbInit = false;

void printk_init_fb(Framebuffer *framebuffer, PSF1_FONT *psf1_font) {
        GlobalRenderer = GOP(framebuffer, psf1_font);
	fbInit = true;
}
void GOP_print_str(const char *str) {
        GlobalRenderer.print_str(str);
	//GlobalRenderer.BufferSwitch();
}
void GOP_print_char(const char ch) {
        GlobalRenderer.print_char(ch);
	//GlobalRenderer.BufferSwitch();
}
void GOP_print_pixel(int x, int y, uint32_t color) {
        GlobalRenderer.put_pixel(x, y, color);
	//GlobalRenderer.BufferSwitch();
}

void print_image(const uint8_t *data) {
        char *ptr = strtok((char*)data, "\n");
        //dprintf("%s\n", ptr);

        uint16_t height, width;

        ptr = strtok(NULL, " ");
        width = atoi(ptr);
        ptr = strtok(NULL, "\n");
        height = atoi(ptr);

        //dprintf("Width: %d\nHeight: %d\n", width, height);

        ptr = strtok(NULL, "\n");

        for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                        uint8_t r = atoi(strtok(NULL, "\n"));
                        uint8_t g = atoi(strtok(NULL, "\n"));
                        uint8_t b = atoi(strtok(NULL, "\n"));
                        uint32_t color = (b | (g << 8) | (r << 16) | (0xff << 24));
                        //dprintf("Color : %d %d %d 0x%x\n", r, g, b, color);
                        GOP_print_pixel(x, y, color);
                }
        }
}

void printk_init_serial() {
        if(serial_init(COM1) != 0) {
                return;
        }
}

static void print_all(const char *string) {
        serial_print_str(string);
	if (fbInit) GOP_print_str(string);
}

static void print_all_char(const char ch) {
        serial_print_char(ch);
        if(fbInit) GOP_print_char(ch);
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
                                        itoa(buf, 'd', va_arg(ap, int64_t));
                                        print_all(buf);
                                        break;
                                case 'x': {
                                        itoa(buf, 'x', va_arg(ap, int64_t));
					int length = strlen(buf);
					for (int i = 0; i < 18 - length; i++) {
						print_all_char('0');
					}
                                        print_all(buf);
					}
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
