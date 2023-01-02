#pragma once
#include <cdefs.h>
#include <dev/serial/serial.h>
#include <dev/fb/gop.h>
#include <dev/fb/fb.h>
#include <stdarg.h>
#include <mm/string.h>

void printk(char *format, ...);
void printk_init_serial();
void printk_init_fb(Framebuffer* new_target_framebuffer, PSF1_FONT* new_psf1_font);
