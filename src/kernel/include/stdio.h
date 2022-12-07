#pragma once
#include <stdlib.h>
#include <cdefs.h>

#define EOF (-1)
 
#ifdef __cplusplus
extern "C" {
#endif
void printk(char *format, ...);
#ifdef __cplusplus
}
#endif
