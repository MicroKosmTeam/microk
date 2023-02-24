#pragma once
#include <cdefs.h>
#include <dev/serial/serial.h>

void printk(char *format, ...);
//void printk_init_serial(UARTDevice device);
void printk_init_serial(UARTDevice *device);

