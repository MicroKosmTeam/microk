#include <module/symbol.h>
#include <fs/vfs.h>
#include <stdio.h>
#include <mm/pagetable.h>
#include <mm/heap.h>

#define PREFIX "[KMOD] "

uint64_t KRNLSYMTABLE[SYMTABLE_SIZE];

namespace KRNSYM {
        void Setup() {
                fprintf(VFS_FILE_STDLOG, PREFIX "Loading symtable...\n");
                fprintf(VFS_FILE_STDLOG, PREFIX "Symtable at: 0x%x\n", &KRNLSYMTABLE);

                KRNLSYMTABLE[0] = (uint64_t)&fprintf;
                fprintf(VFS_FILE_STDLOG, PREFIX "Function at: 0x%x\n",&fprintf);
                fprintf(VFS_FILE_STDLOG, PREFIX "In symtable at: 0x%x\n", KRNLSYMTABLE[0]);
                fprintf(VFS_FILE_STDLOG, PREFIX "Symtable initialized!\n");
        
                Test((uint64_t)&KRNLSYMTABLE);
        }

        void Test(uint64_t address) {
                fprintf(VFS_FILE_STDLOG, PREFIX "Assigning function...\n");
                uint64_t *symbolTable = (uint64_t*)address;

                fprintf(VFS_FILE_STDLOG, PREFIX "Function address: 0x%x\n", symbolTable[0]);

                void(*test)(fd_t file, char *format, ...) = (void(*)(fd_t,char*,...)) (symbolTable[0]);

                fprintf(VFS_FILE_STDLOG, PREFIX "Running function...\n");

                test(VFS_FILE_STDLOG, "[TESTMOD] " "Hello World!\n");
                test(VFS_FILE_STDLOG, "[TESTMOD] " "Done!\n");

                fprintf(VFS_FILE_STDLOG, PREFIX "Done.\n");
        }
}
