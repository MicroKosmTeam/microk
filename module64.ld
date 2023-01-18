OUTPUT_FORMAT(elf64-x86-64)
ENTRY(modmain)

SECTIONS
{
        . = 0x100000;
        module_start = .;
	.text : ALIGN(0x1000)
	{
		*(.text)
	}
	.data : ALIGN(0x1000)
	{
		*(.data)
	}
	.rodata : ALIGN(0x1000)
	{
		*(.rodata)
	}
	.bss : ALIGN(0x1000)
	{
		*(COMMON)
		*(.bss)
	}
        module_end = .;
}
