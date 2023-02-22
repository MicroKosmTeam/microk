#include 	<stdio.h>
#include	<libelf/libelf.h>
#include	"obj_osdep.h"

char	elf_class;

/*
 * Operations related to the elf object file header
 */

/* Return a ptr to the file header */
void *
elf_getehdr (Elf *elf)
{
	if (elf_class == ELFCLASS32) {
		return(elf32_getehdr(elf));
	} else if (elf_class == ELFCLASS64) {
		return(elf64_getehdr(elf));
	}
	return (0);
}

/* Return the section index of the section header string table */
int
elf_geteshstrndx (void *ehdr)
{
	if (elf_class == ELFCLASS32) {
		return(((Elf32_Ehdr *)(ehdr))->e_shstrndx);
	} else if (elf_class == ELFCLASS64) {
		return(((Elf64_Ehdr *)(ehdr))->e_shstrndx);
	}
	return (0);
}


/*
 * Operations related to the elf section headers
 */

/* Return a pointer to the elf section header */
void *
elf_getshdr (Elf_Scn *scn)
{
	if (elf_class == ELFCLASS32) {
		return(elf32_getshdr(scn));
	} else if (elf_class == ELFCLASS64) {
		return(elf64_getshdr(scn));
	}
	return (0);
}

/* Return the index in the section header string table, for the section name */
size_t
elf_getsname (void *shdr)
{
	if (elf_class == ELFCLASS32) {
		return(((Elf32_Shdr *)(shdr))->sh_name);
	} else if (elf_class == ELFCLASS64) {
		return(((Elf64_Shdr *)(shdr))->sh_name);
	}
	return (0);
}

/* Set the section name, an index into the section header string table */
void
elf_setsname (void *shdr, int ndx)
{
	if (elf_class == ELFCLASS32) {
		((Elf32_Shdr *)(shdr))->sh_name = ndx;
	} else if (elf_class == ELFCLASS64) {
		((Elf64_Shdr *)(shdr))->sh_name = ndx;
	}
}
