#pragma once
#include <cdefs.h>
#include <dev/serial/serial.h>
#include <dev/fb/gop.h>
#include <dev/fb/fb.h>
#include <stdarg.h>
#include <mm/string.h>

void print_image(int count);
void printk(char *format, ...);
void printk_init_serial();
void printk_init_fb(Framebuffer *framebuffer, PSF1_FONT *psf1_font);

extern GOP GlobalRenderer;
