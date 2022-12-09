#pragma once
#include <cdefs.h>

#ifdef KCONSOLE_SERIAL
#include <dev/serial/serial.h>
#endif
#ifdef KCONSOLE_VGA
#include <dev/vga/vga.h>
#endif
#ifdef KCONSOLE_VBE
#include <dev/vbe/vbe.h>
#endif
#ifdef KCONSOLE_GOP
#include <dev/gop/gop.h>
#endif
#include <stdarg.h>
#include <string.h>

void printk(char *format, ...);
void printk_init();
