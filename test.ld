OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(_start)

SECTIONS
{
    /* We wanna be placed in the topmost 2GiB of the address space, for optimisations */
    /* and because that is what the Limine spec mandates. */
    /* Any address in this region will do, but often 0xffffffff80000000 is chosen as */
    /* that is the beginning of the region. */
    . = 0x100000;
/*	. =0x100000000;*/

    .text : {
        *(.text .text.*)
    }    .rodata : {
        *(.rodata .rodata.*)
    }
    .data : {
        *(.data .data.*)
    }
    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    }
    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}
