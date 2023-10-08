OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(ModuleInit)

SECTIONS
{
    . = 0x400000;

    .text : {
        *(.text .text.*)
    }
    .rodata : {
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
