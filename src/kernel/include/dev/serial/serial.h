#pragma once
#include <stdint.h>
#include <stddef.h>

#define COM1 0x3f8
#define COM2 0x2f8
#define COM3 0x3e8
#define COM4 0x2e8
#define COM5 0x5f8
#define COM6 0x4f8
#define COM7 0x5e8
#define COM8 0x4e8
extern int port;

int serial_init(int set_port);
void serial_print_str(char* str);
void serial_print_char(char ch);
