#include <elf.h>
#include <elf64.h>
#include <sys/elf.hpp>
#include <sys/printk.hpp>
#include <mm/memory.hpp>
#include <mm/string.hpp>
#include <sys/panic.hpp>

uint64_t *LoadELF(uint8_t *data) {
	if (data[0] != 0x7F || data[1] != 'E' || data[2] != 'L' || data[3] != 'F') {
		PRINTK::PrintK("ELF File not valid.\r\n");
		return NULL;
	}

	PRINTK::PrintK("ELF file is valid\r\n");
}
