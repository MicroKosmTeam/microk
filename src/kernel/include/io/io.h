#pragma once
#include <cdefs.h>

#ifdef x86_64

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void outb(uint16_t port, uint8_t val);
void outw(uint16_t port, uint16_t val);
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
void io_wait(void);

#ifdef __cplusplus
}
#endif

#endif
