#pragma once
#include <cdefs.h>

#ifdef KCONSOLE_SERIAL
#include <serial.h>
#endif
#ifdef KCONSOLE_VGA
#include <vga.h>
#endif
#ifdef KCONSOLE_VBE
#include <vbe.h>
#endif
#ifdef KCONSOLE_GOP
#include <gop.h>
#endif
#include <stdarg.h>
#include <string.h>

void printk(char *format, ...);
void printk_init();
