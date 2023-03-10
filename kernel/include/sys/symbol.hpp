#pragma once
#include <stdint.h>

typedef struct {
    const uint64_t addr;
    const char *name;
} Symbol;

extern const Symbol symbols[];
extern const uint64_t symbolCount;

const Symbol *LookupSymbol(const char *name);
const Symbol *LookupSymbol(const uint64_t addr);
