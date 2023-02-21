#pragma once
#include <cdefs.h>
#include <dev/serial/serial.h>
#include <stdarg.h>
#include <mm/string.h>

void printk(char *format, ...);
void printk_init_serial();

