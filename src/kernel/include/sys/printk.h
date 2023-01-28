#pragma once
#include <cdefs.h>

#ifdef PRINTK_SUBSYSTEM
#ifdef KCONSOLE_SERIAL
#include <dev/serial/serial.h>
#endif
#ifdef KCONSOLE_GOP
#ifdef __cplusplus
#include <dev/fb/gop.h>
#include <dev/fb/fb.h>
#endif
#endif
#include <stdarg.h>
#include <mm/string.h>

#ifdef __cplusplus
extern "C" {
#endif
void print_image(const uint8_t *data);
void printk(char *format, ...);
void printk_init_serial();
void GOP_print_str(const char *str);
void GOP_print_char(const char ch);
void GOP_print_pixel(int x, int y, uint32_t color);
#ifdef __cplusplus
}
#endif

#ifdef KCONSOLE_GOP
#ifdef __cplusplus
void printk_init_fb(Framebuffer *framebuffer, PSF1_FONT *psf1_font);
#endif
#endif

#ifdef KCONSOLE_GOP
#ifdef __cplusplus
extern GOP GlobalRenderer;
#endif
#endif

#endif

