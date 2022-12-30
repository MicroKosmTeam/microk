#pragma once

#define GDT_CODE        (0x18<<8)
#define GDT_DATA        (0x12<<8)
#define GDT_TSS         (0x09<<8)
#define GDT_DPL(lvl)    ((lvl)<<13)
#define GDT_PRESENT     (1<<15)
#define GDT_LONG        (1<<21)
