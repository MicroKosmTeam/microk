#include <libelf/libelf.h>

/*
 * For UnixWare 7, these things do not exist.
 */
#define	elf64_getehdr(x)	NULL
#define	elf64_getshdr(x)	NULL
#define Elf64_Ehdr		Elf32_Ehdr
#define Elf64_Shdr		Elf32_Shdr


size_t  elf_getsname (void *shdr);
void   *elf_getshdr (Elf_Scn *scn);
int     elf_geteshstrndx (void *ehdr);
void   *elf_getehdr (Elf *elf);
