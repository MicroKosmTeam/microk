#pragma once
#include <stdbool.h>
#include <stdio.h>
#include <cdefs.h>

#ifndef PREFIX
        #define RENAME(f)
#else
       #define RENAME(f) PREFIX ## f
#endif
// Example: rename printk to printf
// #define printf RENAME(printk)

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif
