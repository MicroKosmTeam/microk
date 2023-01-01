#pragma once
#include <stdint.h>

#define KNAME "MicroKernel"
#define KVER "devel-alpha"
#define KCOMMENTS "Merry Christmas!"

#define KBOOT_EFI
#undef  KBOOT_GRUB
#ifdef  KBOOT_GRUB
        #define KBOOT_MBOOT1
        #define KBOOT_MBOOT2
#endif
#define KMAX_CPUS 16
#define KMAX_IOAPIC 8
#define KCONSOLE_SERIAL
#undef  KCONSOLE_VGA
#undef  KCONSOLE_VBE
#define  KCONSOLE_GOP
