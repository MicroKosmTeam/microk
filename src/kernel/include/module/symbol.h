#pragma once
#include <stdint.h>

const uint64_t SYMTABLE_SIZE = 3;
extern uint64_t KRNLSYMTABLE[SYMTABLE_SIZE];

namespace KRNSYM {
        void Setup();
        void Test(uint64_t address);
}
