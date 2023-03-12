#include <elf.h>
#include <elf64.h>
#include <sys/elf.hpp>
#include <sys/printk.hpp>
#include <mm/memory.hpp>
#include <mm/string.hpp>
#include <sys/panic.hpp>
#include <stdarg.h>
#include <sys/driver.hpp>
#include <dev/pci/pci.hpp>

uint64_t *currentStack;

uint64_t *LoadELF(uint8_t *data) {
	if (data[0] != 0x7F || data[1] != 'E' || data[2] != 'L' || data[3] != 'F') {
		PRINTK::PrintK("ELF File not valid.\r\n");
		return NULL;
	}

	PRINTK::PrintK("ELF file is valid\r\n");

	uint64_t elfHeaderSize = ((Elf64_Ehdr*)data)->e_ehsize;
	if (elfHeaderSize == 0) return NULL;
	Elf64_Ehdr *elfHeader = (Elf64_Ehdr*)Malloc(elfHeaderSize);
	memcpy(elfHeader, data, elfHeaderSize);

	/* uint64_t sectionHeaderSize = elfHeader->e_shentsize;
	uint64_t sectionHeaderOffset = elfHeader->e_shoff;
	uint64_t sectionHeaderNumber = elfHeader->e_shnum;

	Elf64_Shdr *sectionHeader = (Elf64_Shdr*)Malloc(sectionHeaderSize);

	for (int i = 0; i < sectionHeaderNumber; i++) {
		memcpy(sectionHeader, data + sectionHeaderOffset + sectionHeaderSize * i, sectionHeaderSize);
		PRINTK::PrintK("Section Header %d\r\n"
			       "- Name: %d\r\n"
			       "- Type: %d\r\n"
			       "- Offset: %d\r\n"
			       "- Size: %d\r\n",
			       i,
			       sectionHeader->sh_name,
			       sectionHeader->sh_type,
			       sectionHeader->sh_offset,
			       sectionHeader->sh_size);

	} */

	uint64_t programHeaderSize = elfHeader->e_phentsize;
	uint64_t programHeaderOffset = elfHeader->e_phoff;
	uint64_t programHeaderNumber = elfHeader->e_phnum;

	uint64_t progSize = 0;

	Elf64_Phdr *programHeader = (Elf64_Phdr*)Malloc(programHeaderSize);
	for (int i = 0; i < programHeaderNumber; i++) {
		memcpy(programHeader, data + programHeaderOffset + programHeaderSize * i, programHeaderSize);

		if(programHeader->p_type == PT_LOAD && programHeader->p_memsz > 0) {
			VMalloc(programHeader->p_vaddr, programHeader->p_memsz);
			memset(programHeader->p_vaddr, 0, programHeader->p_memsz);
			memcpy(programHeader->p_vaddr, data + programHeader->p_offset, programHeader->p_filesz);
		}

		progSize += programHeader->p_memsz;

		PRINTK::PrintK("Program Header %d\r\n"
			       "- Offset: %d\r\n"
			       "- Virtual address: 0x%x\r\n"
			       "- File Size: 0x%x\r\n"
			       "- Memory Size: 0x%x\r\n",
			       i,
			       programHeader->p_offset,
			       programHeader->p_vaddr,
			       programHeader->p_filesz,
			       programHeader->p_memsz);
	}

	PRINTK::PrintK("Loading Complete. Program is %d bytes. Jumping to entry point at 0x%x\r\n", progSize ,elfHeader->e_entry);

	Driver* (*elfEntry)(void) = elfHeader->e_entry;

	Driver *driver= elfEntry();

	PRINTK::PrintK("\r\nELF file returned to kernel. Driver info address: 0x%x.\r\n", driver);

	delete programHeader;
	delete elfHeader;

	PRINTK::PrintK("Cleaned up.\r\n");

	PRINTK::PrintK("Now calling %s with request 0.\r\n", driver->Name);
	Ioctl(driver, 0, PCI::GetHeader());

	PRINTK::PrintK("\r\nELF Loader is done.\r\n");

	return NULL;
}
