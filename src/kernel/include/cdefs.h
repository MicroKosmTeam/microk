#pragma once
#include <stdint.h>

#define KNAME "MicroKernel"
#define KVER "devel-alpha"
#define KCOMMENTS "Merry Christmas!"

#define KBOOT_MBOOT1
#define KBOOT_MBOOT2
#define KMAX_CPUS 16
#define KMAX_IOAPIC 8
#define KCONSOLE_SERIAL
#define KCONSOLE_VGA
#undef  KCONSOLE_VBE
#undef  KCONSOLE_GOP
