#include <module/main.h>
#include <stdint.h>

extern "C" void modmain(uint64_t symtable_address) {
        uint64_t *symbolTable = (uint64_t*)symtable_address;

        void(*printk)(char *format, ...) = (void(*)(char*,...)) (symbolTable[0]);
        printk("[MODMAIN]" "Hello World!\n");
}
