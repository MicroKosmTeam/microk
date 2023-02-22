#pragma once
#include <stdint.h>

typedef struct {
    const uint64_t addr;
    const char *name;
} symbol_t;

extern const symbol_t symbols[];
extern const uint64_t symbolCount;

const symbol_t *lookup_symbol(const uint64_t addr);

