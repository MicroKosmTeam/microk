#pragma once
#include <cdefs.h>

#ifdef CONSOLE_VGA
#include <vga_print.h>
#endif
#include <stdarg.h>
#include <string.h>

void printk(char *format, ...);
void printk_init();
