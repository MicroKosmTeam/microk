#pragma once
#include <stdint.h>

#define KNAME "MicroKernel"
#define KVER "devel-alpha"
#define KCOMMENTS "Touching the sky."


#ifdef x86_64
#elif x86
        #error "i686 is not supported."
#elif ARM7
        #error "ARM7 is not supported."
#elif AARCH64
        #error "AARCH64 is not supported."
#else
        #error "ARCH is not specified."
#endif

// PRINTK
#define PRINTK
#ifdef PRINTK
        #define KCONSOLE_SERIAL
        #define KCONSOLE_GOP
#endif

// PCI
#define PCI_SUBSYSYEM
#ifdef PCI_SUBSYSYEM
        #define AHCI_SUBSYTEM
        #ifdef AHCI_SUBSYTEM
                #define AHCI_BUFFER_SIZE 512 * 64
        #endif
#endif


// UBSAN
#undef UBSAN
#ifdef UBSAN
        #undef  UBSAN_NULL_PTR
        #undef  UBSAN_MEM_ALIGN
        #undef  UBSAN_OOB
        #undef  UBSAN_INSUFFSIZE
        #undef  UBSAN_PANIK
#endif
