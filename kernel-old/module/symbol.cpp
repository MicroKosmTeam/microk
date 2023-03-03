#include <module/symbol.h>
#include <fs/vfs.h>
#include <stdio.h>
#include <mm/pagetable.h>
#include <mm/heap.h>

#define PREFIX "[KMOD] "

uint64_t KRNLSYMTABLE[SYMTABLE_SIZE];

namespace KRNSYM {
        void Setup() {
                dprintf(PREFIX "Loading symtable...\n");
                dprintf(PREFIX "Symtable at: 0x%x\n", &KRNLSYMTABLE);

                KRNLSYMTABLE[0] = (uint64_t)&dprintf;
                dprintf(PREFIX "Function at: 0x%x\n",&dprintf);
                dprintf(PREFIX "In symtable at: 0x%x\n", KRNLSYMTABLE[0]);
                dprintf(PREFIX "Symtable initialized!\n");

                Test((uint64_t)&KRNLSYMTABLE);
        }

        void Test(uint64_t address) {
                dprintf(PREFIX "Assigning function...\n");
                uint64_t *symbolTable = (uint64_t*)address;

                dprintf(PREFIX "Function address: 0x%x\n", symbolTable[0]);

                void(*test)(char *format, ...) = (void(*)(char*,...)) (symbolTable[0]);

                dprintf(PREFIX "Running function...\n");

                test("[TESTMOD] " "Hello World!\n");
                test("[TESTMOD] " "Done!\n");

                dprintf(PREFIX "Done.\n");
        }
}
