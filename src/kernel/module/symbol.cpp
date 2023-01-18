#include <module/symbol.h>
#include <sys/printk.h>
#include <mm/pagetable.h>
#include <mm/heap.h>

#define PREFIX "[KMOD] "

uint64_t KRNLSYMTABLE[SYMTABLE_SIZE];

namespace KRNSYM {
        void Setup() {
                printk(PREFIX "Loading symtable...\n");
                printk(PREFIX "Symtable at: 0x%x\n", &KRNLSYMTABLE);

                KRNLSYMTABLE[0] = (uint64_t)&printk;
                printk(PREFIX "Function at: 0x%x\n",&printk);
                printk(PREFIX "In symtable at: 0x%x\n", KRNLSYMTABLE[0]);
                KRNLSYMTABLE[1] = (uint64_t)&malloc;
                printk(PREFIX "Function at: 0x%x\n",&malloc);
                printk(PREFIX "In symtable at: 0x%x\n", KRNLSYMTABLE[1]);
                KRNLSYMTABLE[2] = (uint64_t)&free;
                printk(PREFIX "Function at: 0x%x\n",&free);
                printk(PREFIX "In symtable at: 0x%x\n", KRNLSYMTABLE[2]);

                printk(PREFIX "Symtable initialized!\n");
        
                Test((uint64_t)&KRNLSYMTABLE);
        }

        void Test(uint64_t address) {
                printk(PREFIX "Assigning function...\n");
                uint64_t *symbolTable = (uint64_t*)address;

                printk(PREFIX "Function address: 0x%x\n", symbolTable[0]);
                printk(PREFIX "Function address: 0x%x\n", symbolTable[1]);
                printk(PREFIX "Function address: 0x%x\n", symbolTable[2]);

                void(*test)(char *format, ...) = (void(*)(char*,...)) (symbolTable[0]);

                printk(PREFIX "Running function...\n");

                test("[TESTMOD]" "Hello World!\n");
                test("[TESTMOD]" "Done!\n");

                printk(PREFIX "Done.\n");
        }
}
