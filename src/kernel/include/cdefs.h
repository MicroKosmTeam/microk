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

#define THREAD_SUBSYSTEM
#ifdef THREAD_SUBSYSTEM
	#define THREAD_STACK_SIZE 1024 * 1024 * 1 // 1MB stack per thread
#endif

// PRINTK
#define PRINTK_SUBSYSTEM
#ifdef PRINTK_SUBSYSTEM
        #define KCONSOLE_SERIAL
        #define KCONSOLE_GOP
#endif

// FS
#define VFS_SUBSYSTEM
#ifdef VFS_SUBSYSTEM
	#define FS_SUBSYSTEM
	#define VFS_FILE_MAX_NAME_LENGTH 256
	#ifdef FS_SUBSYSTEM
		#define FS_TOTAL_SUPPORTED_DRIVES 128
	#endif
#endif

// PCI
#define PCI_SUBSYSYEM
#ifdef PCI_SUBSYSYEM
        #define AHCI_SUBSYTEM
        #ifdef AHCI_SUBSYTEM
                #define AHCI_BUFFER_SIZE 1024 * 1024 * 4 // 4MB per disk
        #endif
#endif

// UBSAN
#undef UBSAN
#ifdef UBSAN
        #undef UBSAN_NULL_PTR
        #undef UBSAN_MEM_ALIGN
        #undef UBSAN_OOB
        #undef UBSAN_INSUFFSIZE
        #undef UBSAN_PANIK
#endif

