/*
 * $Copyright udi_reference:
 * 
 * 
 *    Copyright (c) 1995-2001; Compaq Computer Corporation; Hewlett-Packard
 *    Company; Interphase Corporation; The Santa Cruz Operation, Inc;
 *    Software Technologies Group, Inc; and Sun Microsystems, Inc
 *    (collectively, the "Copyright Holders").  All rights reserved.
 * 
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the conditions are met:
 * 
 *            Redistributions of source code must retain the above
 *            copyright notice, this list of conditions and the following
 *            disclaimer.
 * 
 *            Redistributions in binary form must reproduce the above
 *            copyright notice, this list of conditions and the following
 *            disclaimers in the documentation and/or other materials
 *            provided with the distribution.
 * 
 *            Neither the name of Project UDI nor the names of its
 *            contributors may be used to endorse or promote products
 *            derived from this software without specific prior written
 *            permission.
 * 
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *    "AS IS," AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *    HOLDERS OR ANY CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *    TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 *    DAMAGE.
 * 
 *    THIS SOFTWARE IS BASED ON SOURCE CODE PROVIDED AS A SAMPLE REFERENCE
 *    IMPLEMENTATION FOR VERSION 1.01 OF THE UDI CORE SPECIFICATION AND/OR
 *    RELATED UDI SPECIFICATIONS. USE OF THIS SOFTWARE DOES NOT IN AND OF
 *    ITSELF CONSTITUTE CONFORMANCE WITH THIS OR ANY OTHER VERSION OF ANY
 *    UDI SPECIFICATION.
 * 
 * 
 * $
 */
/*
 * Written for SCO by: Rob Tarte
 * At: Pacific CodeWorks, Inc.
 * Contact: http://www.pacificcodeworks.com
 */

#include	<sys/param.h>
#include	<libelf.h>

#include <stdio.h>
#include <varargs.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/elf.h>
#include <sys/elf_386.h>
#include <sys/elftypes.h>
#include <filehdr.h>
#include <scnhdr.h>
#include <reloc.h>
#include <unistd.h>
#include <memory.h>
#include <malloc.h>
#include <time.h>
#include <stdlib.h>

#include	"obj_osdep.h"

STATIC struct symbol_entry	*coff_symbols;
STATIC int			total_symbols;

STATIC Elf32_Ehdr		*e_header;
STATIC Elf32_Shdr		*s_header;
STATIC Elf32_Sym		*elf_symbols;
STATIC char			*elf_string_table;
STATIC char			*elf_section_string_table;
STATIC int			ext_sym_offset;
STATIC int			coff_num_sections;

STATIC struct symbol_entry	*long_name_head;
STATIC struct symbol_entry	*long_name_tail;

STATIC struct section_entry	*section_entry_head;
STATIC struct section_entry	*section_entry_tail;

STATIC int			debug;

STATIC int			bss_offset;
STATIC int			bss_start;

STATIC struct symbol_entry	**elf_index_to_coff_symbol;
STATIC struct symbol_entry	**coff_symbol_table;

char	elf_class;

/*
 * Operations related to the elf object file header
 */

struct	filehdr		coff_filehdr;
struct scnhdr		*coff_scnhdrs;

int
elf2coff(
char	*elf_filename,
char	*coff_filename
)
{
	int	elf_fd;
	int	coff_fd;
	int	next_arg;

	elf_fd = open(elf_filename, O_RDONLY);

	if (elf_fd < 0)
	{
		perror("open");

		fprintf(stderr, "could not open file %s\n", elf_filename);

		return 0;
	}

	coff_fd = open(coff_filename, O_RDWR | O_CREAT | O_TRUNC, 0660);

	if (coff_fd < 0)
	{
		perror("open");

		fprintf(stderr, "could not open file %s\n", coff_filename);

		return 0;
	}

	elf_verify(elf_fd);

	elf_read_header(elf_fd);

	create_coff_headers(coff_fd);

	read_symbol_table(elf_fd);

	remove_extra_symbols();

	output_relocation_table(elf_fd, coff_fd);

	output_symbol_table(elf_fd, coff_fd);

	output_coff_section_data(elf_fd, coff_fd);

	output_coff_sections(coff_fd);

	/* free_memory(); */

	return 1;
}

#if 0
int
main(
int	argc,
char	*argv[]
)
{
	int	elf_fd;
	int	coff_fd;
	char	*elf_filename;
	char	*coff_filename;
	int	next_arg;

	next_arg = parseargs(argc, argv);

	if (argc - next_arg != 2)
	{
		fprintf(stderr, "elf2coff: incorrect number of arguments\n");

		usage();

		exit(1);
	}

	elf_filename = argv[next_arg];

	next_arg++;

	elf_fd = open(elf_filename, O_RDONLY);

	if (elf_fd < 0)
	{
		perror("open");

		fprintf(stderr, "could not open file %s\n", elf_filename);

		usage();

		exit(1);
	}


	coff_filename = argv[next_arg];

	next_arg++;

	coff_fd = open(coff_filename, O_RDWR | O_CREAT | O_TRUNC, 0660);

	if (coff_fd < 0)
	{
		perror("open");

		fprintf(stderr, "could not open file %s\n", coff_filename);

		usage();

		exit(1);
	}

	elf_verify(elf_fd);

	elf_read_header(elf_fd);

	create_coff_headers(coff_fd);

	read_symbol_table(elf_fd);

	remove_extra_symbols();

	output_relocation_table(elf_fd, coff_fd);

	output_symbol_table(elf_fd, coff_fd);

	output_coff_section_data(elf_fd, coff_fd);

	output_coff_sections(coff_fd);

	free_memory();

	return 0;
}
#endif

void
free_memory()
{
	struct section_entry	*sentryp;
	struct section_entry	*sentryp_next;
	struct relocation_entry	*rentryp;
	struct relocation_entry	*rentryp_next;

	free(coff_symbols);

	free(coff_symbol_table);

	free(elf_index_to_coff_symbol);

	free(elf_symbols);

	sentryp = section_entry_head;

	while (sentryp != NULL)
	{
		sentryp_next = sentryp->next;

		rentryp = sentryp->reloc_entry;

		while (rentryp != NULL)
		{
			rentryp_next = rentryp->next;

			free(rentryp);

			rentryp = rentryp_next;
		}

		free(sentryp->coff_scnhdr);

		free(sentryp);

		sentryp = sentryp_next;
	}

	if (elf_section_string_table != elf_string_table)
	{
		free(elf_section_string_table);
	}

	free(elf_string_table);
	free(s_header);
	free(e_header);
}

void
elf_verify(
int	elf_fd
)
{
	unsigned	i;

	e_header = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));

	if (read(elf_fd, e_header, sizeof(Elf32_Ehdr)) != 
		sizeof(Elf32_Ehdr))
	{
		fprintf(stderr, "Read of elf header failed\n");

		exit(1);
	}

	if (strncmp((char *)e_header, ELFMAG, SELFMAG) != 0)
	{
		free(e_header);

		fprintf(stderr, "Not ELF object\n");

		exit(1);
	}

	dprintf("ELF object\n");

	dprintf("------- ELF HEADER --------\n");

	for (i = 0 ; i < EI_NIDENT ; i++)
	{
		dprintf("%02x ", e_header->e_ident[i]);
	}

	dprintf("type 0x%x\t", e_header->e_type);
	dprintf("machine 0x%x\t", e_header->e_machine);
	dprintf("version 0x%x\t", e_header->e_version);
	dprintf("entry 0x%x\t", e_header->e_entry);
	dprintf("phoff 0x%x\t", e_header->e_phoff);
	dprintf("shoff 0x%x\t", e_header->e_shoff);
	dprintf("flags 0x%x\t", e_header->e_flags);
	dprintf("ehsize 0x%x\t", e_header->e_ehsize);
	dprintf("phentsize 0x%x\t", e_header->e_phentsize);
	dprintf("phnum 0x%x\t", e_header->e_phnum);
	dprintf("shentsize 0x%x\t", e_header->e_shentsize);
	dprintf("shnum 0x%x\t", e_header->e_shnum);
	dprintf("shstrndx 0x%x\t", e_header->e_shstrndx);

	dprintf("\n");
}

void
elf_read_header(
int elf_fd
)
{
	Elf32_Shdr	*shdr;
	unsigned	i;

	s_header = (Elf32_Shdr *)
		malloc(e_header->e_shentsize * e_header->e_shnum);

	(void)lseek(elf_fd, e_header->e_shoff, SEEK_SET);

	if (sizeof(Elf32_Shdr) != e_header->e_shentsize)
	{
		dprintf("header size doesn't match\n");
		dprintf("should have been 0x%x\n", sizeof(Elf32_Shdr));
		e_header->e_shentsize = sizeof(Elf32_Shdr);
	}

	/*
	 * Read in the ELF section headers.
	 */
	shdr = s_header;

	for (i = 0 ; i < e_header->e_shnum ; i++, shdr++)
	{
		if (read(elf_fd, shdr, e_header->e_shentsize) !=
			e_header->e_shentsize)
		{
			fprintf(stderr, "read of section table entry failed\n");

			exit(1);
		}
	}

	read_string_table(elf_fd);

	dprintf("-------- SECTIONS ---------\n");

	dprintf("%9s: %8s %4s %8s %8s %8s %8s %8s %4s %4s\n",
		"name", "type", "flags", "addr", "off", "size", "link", "info",
		"algn", "ent");

	shdr = s_header;

	for (i = 0 ; i < e_header->e_shnum ; i++, shdr++)
	{
		dprintf("%9s: ", elf_section_string_table +
			shdr->sh_name);

		dprintf("%8x ", shdr->sh_type);

		dprintf("%4x ", shdr->sh_flags);

		dprintf("%8x ", shdr->sh_addr);

		dprintf("%8x ", shdr->sh_offset);

		dprintf("%8x ", shdr->sh_size);

		dprintf("%8x ", shdr->sh_link);

		dprintf("%8x ", shdr->sh_info);

		dprintf("%4x ", shdr->sh_addralign);

		dprintf("%4x\n", shdr->sh_entsize);
	}
	dprintf("\n");
}

/*
 * Read in the string tables from the ELF object file.  There could
 * be a single string table, or two string tables (one for the section
 * names and one for the symbol names).
 */
void
read_string_table(
int	elf_fd
)
{
	int		offset;
	unsigned	i;
	Elf32_Shdr	*shdr;
	int		len;

	shdr = s_header;

	/*
	 * Look for the first string table that isn't the section string
	 * table.  If e_shstrndx is SHN_UNDEF, then this will just find
	 * the first string table.
	 */
	for (i = 0 ; i < e_header->e_shnum ; i++, shdr++)
	{
		if ((shdr->sh_type == SHT_STRTAB) &&
			(i != e_header->e_shstrndx))
		{
			break;
		}
	}

	/*
	 * If we couldn't find a string table that wasn't the section string
	 * table, then either there is no string table, or the section string
	 * table and the string table are one and the same.
	 */
	if (i == e_header->e_shnum)
	{
		if (e_header->e_shstrndx == SHN_UNDEF)
		{
			fprintf(stderr, "couldn't find string table\n");

			exit(1);
		}

		shdr = &s_header[e_header->e_shstrndx];
	}

	/*
	 * Read in the string table.
	 */
	elf_string_table = (char *)malloc(shdr->sh_size);

	offset = lseek(elf_fd, 0, SEEK_CUR);

	dprintf("symbol string table @ 0x%x\n", shdr->sh_offset);

	(void)lseek(elf_fd, shdr->sh_offset, SEEK_SET);

	if (read(elf_fd, elf_string_table, shdr->sh_size) != shdr->sh_size)
	{
		fprintf(stderr, "read of string table failed\n");

		exit(1);
	}

	(void)lseek(elf_fd, offset, SEEK_SET);

	/*
	 * We need to read in the section string table in order to determine
	 * the section names.  If e_shstrndx has an index value, then
	 * it is the index of a special section string table.
	 */
	if (e_header->e_shstrndx != SHN_UNDEF)
	{
		shdr = &s_header[e_header->e_shstrndx];

		len = shdr->sh_size;

		elf_section_string_table = (char *)malloc(len);

		offset = lseek(elf_fd, 0, SEEK_CUR);

		(void)lseek(elf_fd, shdr->sh_offset, SEEK_SET);

		if (read(elf_fd, elf_section_string_table, len) != len)
		{
			fprintf(stderr, "read of section string table "
				"failed\n");

			exit(1);
		}

		(void)lseek(elf_fd, offset, SEEK_SET);
	}
	else
	{
		elf_section_string_table = elf_string_table;
	}
}

void
create_coff_headers(
int	coff_fd
)
{
	int			file_offset;
	Elf32_Shdr		*shdr;
	unsigned		i;
	struct scnhdr		*scnhdrp;
	struct section_entry	*sentryp;
	int			prog_offset;
	int			section_index;

	/*
	 * Skip the file header
	 */
	file_offset = sizeof(struct filehdr);

	/*
	 * Do a quick walk through the ELF sections to see how many
	 * COFF sections there are going to be.
	 */
	shdr = s_header;

	coff_num_sections = 4;

	/*
	 * Everything starts after the file header and the section headers.
	 */
	file_offset += sizeof(struct scnhdr) * coff_num_sections;

	prog_offset = 0;

	/*
	 * walk through all of the ELF sections adding any sections that
	 * will be in the COFF object to the file_offset.
	 */
	shdr = s_header;

	section_index = 1;

	for (i = 0 ; i < e_header->e_shnum ; i++, shdr++)
	{
		/*
		 * If the section is marked PROGBITS and ALLOC
		 * (.data, .data1, .rodata, .rodata1, .text)
		 * or if the section is marked NOBITS and ALLOC)
		 * (.bss)
		 */
		if ((shdr->sh_flags & SHF_ALLOC) != SHF_ALLOC)
		{
			dprintf("skipping flags %s - 0x%x\n", 
				elf_section_string_table +
				shdr->sh_name, shdr->sh_flags);
			continue;
		}

		if ((shdr->sh_type != SHT_NOBITS) &&
			(shdr->sh_type != SHT_PROGBITS))
		{
			dprintf("skipping type %s\n", 
				elf_section_string_table +
				shdr->sh_name);
			continue;
		}

		sentryp = (struct section_entry *)
			malloc(sizeof(struct section_entry));

		sentryp->next = NULL;

		sentryp->reloc_entry = NULL;

		scnhdrp = (struct scnhdr *)calloc(1, sizeof(struct scnhdr));

		dprintf("Creating COFF section\n");

		sentryp->coff_scnhdr = scnhdrp;

		sentryp->elf_scnhdr = shdr;

		sentryp->coff_section_index = section_index;

		section_index++;

		if (section_entry_head == NULL)
		{
			section_entry_head = sentryp;

			section_entry_tail = sentryp;
		}
		else
		{
			section_entry_tail->next = sentryp;

			section_entry_tail = sentryp;
		}

		/*
		 * This is the BSS case.
		 */
		if (shdr->sh_type == SHT_NOBITS)
		{
			dprintf("Creating section header for .bss\n");

			(void)memset(&scnhdrp->s_name, 0,
				sizeof(struct scnhdr));

			sprintf(scnhdrp->s_name, ".bss");

			scnhdrp->s_size = ROUNDUP(shdr->sh_size, 4);

			scnhdrp->s_nreloc = 0;

			scnhdrp->s_flags = STYP_BSS;

			continue;
		}

		scnhdrp->s_paddr = prog_offset;

		scnhdrp->s_vaddr = prog_offset;

		scnhdrp->s_size = ROUNDUP(shdr->sh_size, 4);

		/*
		 * The BSS offset comes after text and data, so if we add
		 * up all of the sizes of text and data we will have the
		 * starting point for BSS.
		 */
		bss_start = bss_start + scnhdrp->s_size;

		scnhdrp->s_scnptr = file_offset;

		file_offset += scnhdrp->s_size;

		dprintf("file_offset = 0x%x shdr->sh_size 0x%x "
			"ROUNDUP = 0x%x\n",
			file_offset, shdr->sh_size, ROUNDUP(shdr->sh_size, 4));

		prog_offset += scnhdrp->s_size;

		/*
		 * we are dealing with the SHT_PROGBITS case
		 */

		if ((shdr->sh_flags & SHF_EXECINSTR) == SHF_EXECINSTR)
		{
			dprintf("Creating section header for .text\n");

			sprintf(scnhdrp->s_name, ".text");

			scnhdrp->s_flags = STYP_TEXT;
		}
		else
		{
			dprintf("Creating section header for .data\n");

			sprintf(scnhdrp->s_name, ".data");

			scnhdrp->s_flags = STYP_DATA;
		}
	}

	(void)lseek(coff_fd, file_offset, SEEK_SET);
}

void
read_symbol_table(
int elf_fd
)
{
	Elf32_Shdr		*elf_sym_sheader;
	int			i;
	int			num_symbols;
	int			section_index;

	elf_sym_sheader = elf_find_section(SHT_SYMTAB, 0);

	if (elf_sym_sheader == NULL)
	{
		dprintf("No symbols\n");

		return;
	}

	num_symbols = elf_sym_sheader->sh_size / sizeof(Elf32_Sym);

	elf_symbols = (Elf32_Sym *)malloc(elf_sym_sheader->sh_size);

	elf_index_to_coff_symbol = (struct symbol_entry **)
		malloc(sizeof(struct symbol_entry *) * num_symbols);

	coff_symbol_table = 
		malloc(sizeof(struct symbol_entry *) * num_symbols);

	coff_symbols = (struct symbol_entry *)
		malloc(sizeof(struct symbol_entry) * num_symbols);

	(void)lseek(elf_fd, elf_sym_sheader->sh_offset, SEEK_SET);

	if (read(elf_fd, elf_symbols, elf_sym_sheader->sh_size) !=
		elf_sym_sheader->sh_size)
	{
		fprintf(stderr, "read of symbols failed\n");

		exit(1);
	}

	total_symbols = 0;

	/*
	 * The extended symbol table has an unsigned value at the front
	 * of the table that tells the length of the table.
	 */
	ext_sym_offset = 4;

	dprintf("-------- SYMBOLS ---------\n");

	for (i = 1 ; i < num_symbols ; i++)
	{
		section_index = elf_symbols[i].st_shndx;

		switch (section_index)
		{
		case SHN_UNDEF:
		case SHN_LORESERVE:
		case SHN_ABS:
		case SHN_COMMON:
		case SHN_HIRESERVE:
		case SHN_HIPROC:
			break;

		default:
			if ((s_header[section_index].sh_type != SHT_PROGBITS) &&
			    (s_header[section_index].sh_type != SHT_NOBITS))
			{
				coff_symbols[i].elf_symbol = NULL;

				continue;
			}
		}

		if ((elf_symbols[i].st_value == 0) &&
			(elf_symbols[i].st_size == 0) &&
			(elf_symbols[i].st_info == 0) &&
			(elf_symbols[i].st_shndx == 0) &&
			(elf_symbols[i].st_other == 0))
		{
			coff_symbols[i].elf_symbol = NULL;

			continue;
		}

		coff_symbol_table[total_symbols] = &coff_symbols[i];

		coff_symbol_table[total_symbols]->elf_symbol =
			&elf_symbols[i];

		coff_symbol_table[total_symbols]->elf_index = i;

		dprintf("0x%x: ", i);

		process_symbol(coff_symbol_table[total_symbols]);

		total_symbols++;
	}

	dprintf("\n");

	/*
	 * sort the COFF symbols
	 */
	qsort(&coff_symbol_table[1], total_symbols - 1,
		sizeof(struct symbol_entry *), &sym_compare);

	/*
	 * walk through the newly sorted COFF symbol table and create
	 * a mapping of ELF to COFF symbols so that we can use it for
	 * the relocation table.
	 */
	for (i = 0 ; i < total_symbols ; i++)
	{
		elf_index_to_coff_symbol[coff_symbol_table[i]->elf_index] =
			coff_symbol_table[i];

		coff_symbol_table[i]->coff_index = i;
	}
}

void
remove_extra_symbols(
)
{
	int			i;
	int			j;

	/*
	 * walk through the newly sorted COFF symbol table and create
	 * a mapping of ELF to COFF symbols so that we can use it for
	 * the relocation table.
	 */

	for (i = 0 ; i < total_symbols - 1 ; i = i + j)
	{
		if (coff_symbol_table[i]->elf_symbol->st_shndx == SHN_UNDEF)
		{
			j = 1;

			continue;
		}


		for (j = 1 ; coff_symbol_table[i]->coff_symbol.n_value ==
			coff_symbol_table[i+j]->coff_symbol.n_value ; j++)
		{
			if (ELF32_ST_TYPE(coff_symbol_table[i+j]->
				elf_symbol->st_info) != STT_NOTYPE)
			{
				break;
			}

			if (coff_symbol_table[i]->elf_symbol->st_shndx !=
				coff_symbol_table[i+j]->elf_symbol->st_shndx)
			{
				break;
			}

			dprintf("mapping (elf sym 0x%x:0x%x)->"
				"(elf sym 0x%x:0x%x) "
				"to type=0x%x value=0x%x coff 0x%x - ",
				coff_symbol_table[i+j]->elf_index,
				coff_symbol_table[i+j]->elf_symbol->st_value,
				coff_symbol_table[i]->elf_index,
				coff_symbol_table[i]->elf_symbol->st_value,
				coff_symbol_table[i]->coff_symbol.n_type,
				coff_symbol_table[i]->coff_symbol.n_value,
				coff_symbol_table[i]);

			dprintf("elf section =0x%x\n",
				coff_symbol_table[i+j]->elf_symbol->st_shndx);

			elf_index_to_coff_symbol[
				coff_symbol_table[i+j]->elf_index] =
				coff_symbol_table[i];

			coff_symbol_table[i+j]->coff_symbol.n_type = 0;

			coff_symbol_table[i+j]->coff_symbol.n_value = 0;
		}
	}
}

int
sym_compare(
const void	*a,
const void	*b
)
{
	struct symbol_entry	**sym1;
	struct symbol_entry	**sym2;

	sym1 = (struct symbol_entry **)a;

	sym2 = (struct symbol_entry **)b;

	//dprintf("sym1 = 0x%x sym2 = 0x%x\n", sym1, sym2);

	if ((*sym1)->coff_symbol.n_value != (*sym2)->coff_symbol.n_value)
	{
		return ((*sym1)->coff_symbol.n_value -
			(*sym2)->coff_symbol.n_value);
	}

	if ((*sym1)->name[0] == '.')
	{
		return 1;
	}

	if ((*sym2)->name[0] == '.')
	{
		return -1;
	}

	return 0;
}

void
process_symbol(
struct symbol_entry	*sym_entryp
)
{
	int			len;
	int			j;
	int			section_index;
	int			bind_info;
	struct section_entry	*sentryp;

	(void)memset(&sym_entryp->coff_symbol, 0, sizeof(struct coff_syment));

	sym_entryp->name = &elf_string_table[sym_entryp->elf_symbol->st_name];

	len = strlen(sym_entryp->name);

	if (len == 0)
	{
		dprintf("zero length name - index = 0x%x",
			sym_entryp->elf_symbol->st_shndx);

		switch (sym_entryp->elf_symbol->st_shndx)
		{
		case SHN_UNDEF:
		case SHN_LORESERVE:
		case SHN_ABS:
		case SHN_COMMON:
		case SHN_HIRESERVE:
		case SHN_HIPROC:
			sym_entryp->name = ".unknown";
			break;

		default:
			sym_entryp->name = s_header[
				sym_entryp->elf_symbol->st_shndx].sh_name +
				elf_section_string_table;
			break;
		}

		len = strlen(sym_entryp->name);
	}

	dprintf("%s\t", sym_entryp->name);
	dprintf("value=0x%x ", sym_entryp->elf_symbol->st_value);

#if 0
	if ((strcmp(sym_entryp->name, "udi_init_info") == 0) ||
		(strcmp(sym_entryp->name, "udi_meta_info") == 0))
	{
		sym_entryp->elf_symbol->st_info = ELF32_ST_INFO(
			STB_LOCAL,
			ELF32_ST_TYPE(sym_entryp->elf_symbol->st_info));
	}
#endif

	if ((strcmp(sym_entryp->name, ".comment") == 0) ||
		(strcmp(sym_entryp->name, ".udiprops") == 0))
	{
		sym_entryp->elf_symbol->st_shndx = SHN_ABS;
	}

	if (len <= SYMNMLEN)
	{
		for (j = 0 ; j < len ; j++)
		{
			sym_entryp->coff_symbol.n_name[j] = sym_entryp->name[j];
		}
	}
	else
	{
		sym_entryp->coff_symbol.n_zeroes = 0;

		sym_entryp->coff_symbol.n_offset = ext_sym_offset;

		ext_sym_offset = ext_sym_offset + strlen(sym_entryp->name) + 1;

		if (long_name_head == NULL)
		{
			long_name_head = sym_entryp;

			long_name_tail = sym_entryp;
		}
		else
		{
			long_name_tail->next_long_name = sym_entryp;

			long_name_tail = sym_entryp;
		}

		sym_entryp->next_long_name = NULL;
	}

	section_index = sym_entryp->elf_symbol->st_shndx;

	dprintf("shndx 0x%x - ", section_index);

	switch (section_index)
	{
	case SHN_UNDEF:
		dprintf("SHN_UNDEF\n");
		sym_entryp->coff_symbol.n_scnum = 0;
		sym_entryp->coff_symbol.n_type = 0;
		sym_entryp->coff_symbol.n_sclass = C_EXT;
		break;

	case SHN_LORESERVE:
		dprintf("SHN_LORESERVE\n");
		break;

	case SHN_ABS:
		dprintf("SHN_ABS\n");
		sym_entryp->coff_symbol.n_scnum = (short)0xfffe;
		sym_entryp->coff_symbol.n_type = 0;
		sym_entryp->coff_symbol.n_sclass = C_FILE;
		sym_entryp->coff_symbol.n_value = 0;
		break;

	case SHN_COMMON:
		/*
		 * This is a form of BSS, although it is not in the
		 * BSS section.
		 */
		dprintf("SHN_COMMON\n");
		sym_entryp->coff_symbol.n_scnum = 0;
		sym_entryp->coff_symbol.n_type = 0;
		sym_entryp->coff_symbol.n_sclass = C_EXT;
		sym_entryp->coff_symbol.n_value =
			sym_entryp->elf_symbol->st_size;

		break;

	case SHN_HIRESERVE:
		dprintf("SHN_HIRESERVE\n");
		break;

	case SHN_HIPROC:
		dprintf("SHN_HIPROC\n");
		break;

	default:
		sentryp = find_coff_section_header(&s_header[section_index]);

		switch (s_header[section_index].sh_type)
		{
		case SHT_PROGBITS:
			dprintf("SHT_PROGBITS - ");
			bind_info = ELF32_ST_BIND(
					sym_entryp->elf_symbol->st_info);

			switch (bind_info)
			{
			case STB_LOCAL:
				dprintf("STB_LOCAL - ");
				sym_entryp->coff_symbol.n_sclass = C_STAT;
				break;

			case STB_GLOBAL:
				dprintf("STB_GLOBAL - ");
				sym_entryp->coff_symbol.n_sclass = C_EXT;
				break;

			case STB_WEAK:
				dprintf("STB_WEAK - ");
				sym_entryp->coff_symbol.n_sclass = 0;
				break;

			case STB_NUM:
				dprintf("STB_NUM - ");
				sym_entryp->coff_symbol.n_sclass = 0;
				break;

			default:
				dprintf("oops - 0x%x ", bind_info);
				sym_entryp->coff_symbol.n_sclass = 0;
				break;
			}

			dprintf("st_shndx = 0x%x - ", 
				sym_entryp->elf_symbol->st_shndx);

			switch (s_header[section_index].sh_flags)
			{
			case SHF_EXECINSTR | SHF_ALLOC:
				dprintf("SHF_EXECINSTR | SHF_ALLOC ");
				sym_entryp->coff_symbol.n_type =
					(DT_FCN << 4) | T_INT;
				sym_entryp->coff_symbol.n_scnum = 1;
				break;

			case SHF_WRITE | SHF_ALLOC:
				dprintf("SHF_WRITE | SHF_ALLOC ");
				sym_entryp->coff_symbol.n_scnum = 2;
				break;

			case SHF_ALLOC:
				dprintf("SHF_ALLOC ");
				sym_entryp->coff_symbol.n_scnum = 2;
				break;
			}

			switch (ELF32_ST_TYPE(sym_entryp->elf_symbol->st_info))
			{
			case STT_SECTION:
				dprintf("resetting n_type - ");
				sym_entryp->coff_symbol.n_type = 0;
				break;
			default:
				break;
			}

			sym_entryp->coff_symbol.n_value =
				sym_entryp->elf_symbol->st_value;

			if (sentryp != NULL)
			{
				sym_entryp->coff_symbol.n_value +=
					sentryp->coff_scnhdr->s_vaddr;

			}
			break;

		case SHT_NOTE:
			dprintf("SHT_NOTE - ");
			sym_entryp->coff_symbol.n_type = 0;
			sym_entryp->coff_symbol.n_sclass = C_STAT;
			sym_entryp->coff_symbol.n_value =
				sym_entryp->elf_symbol->st_value;
			break;

		case SHT_SYMTAB:
			dprintf("SHT_SYMTAB - ");
			sym_entryp->coff_symbol.n_type = (DT_FCN << 4) | T_INT;
			sym_entryp->coff_symbol.n_sclass = C_EXT;
			sym_entryp->coff_symbol.n_value =
				sym_entryp->elf_symbol->st_value;
			break;

		case SHT_STRTAB:
			dprintf("SHT_STRTAB - ");
			sym_entryp->coff_symbol.n_type = (DT_FCN << 4) | T_INT;
			sym_entryp->coff_symbol.n_sclass = C_EXT;
			sym_entryp->coff_symbol.n_value =
				sym_entryp->elf_symbol->st_value;
			break;

		case SHT_RELA:
			dprintf("SHT_RELA - ");
			sym_entryp->coff_symbol.n_type = (DT_FCN << 4) | T_INT;
			sym_entryp->coff_symbol.n_sclass = C_EXT;
			sym_entryp->coff_symbol.n_value =
				sym_entryp->elf_symbol->st_value;
			break;

		case SHT_HASH:
			dprintf("SHT_HASH - ");
			sym_entryp->coff_symbol.n_type = (DT_FCN << 4) | T_INT;
			sym_entryp->coff_symbol.n_sclass = C_EXT;
			sym_entryp->coff_symbol.n_value =
				sym_entryp->elf_symbol->st_value;
			break;

		case SHT_DYNAMIC:
			dprintf("SHT_DYNAMIC - ");
			sym_entryp->coff_symbol.n_type = (DT_FCN << 4) | T_INT;
			sym_entryp->coff_symbol.n_sclass = C_EXT;
			sym_entryp->coff_symbol.n_value =
				sym_entryp->elf_symbol->st_value;

			break;

		case SHT_NOBITS:
			dprintf("SHT_NOBITS - ");
			sym_entryp->coff_symbol.n_type = 0;
#if 0
			sym_entryp->coff_symbol.n_scnum = 0;
#else
			sym_entryp->coff_symbol.n_scnum = 3;
#endif

#if 0
			sym_entryp->coff_symbol.n_value =
				sym_entryp->elf_symbol->st_value;
#else
			sym_entryp->coff_symbol.n_value = 	
				bss_start + bss_offset;
#endif
			bss_offset += sym_entryp->elf_symbol->st_size;

			bind_info = ELF32_ST_BIND(
					sym_entryp->elf_symbol->st_info);

			switch (bind_info)
			{
			case STB_LOCAL:
				dprintf("STB_LOCAL - ");
				sym_entryp->coff_symbol.n_sclass = C_STAT;
				break;

			case STB_GLOBAL:
				dprintf("STB_GLOBAL - ");
				sym_entryp->coff_symbol.n_sclass = C_EXT;
				break;

			case STB_WEAK:
				dprintf("STB_WEAK - ");
				sym_entryp->coff_symbol.n_sclass = 0;
				break;

			case STB_NUM:
				dprintf("STB_NUM - ");
				sym_entryp->coff_symbol.n_sclass = 0;
				break;

			default:
				dprintf("oops - 0x%x ", bind_info);
				sym_entryp->coff_symbol.n_sclass = 0;
				break;
			}

			dprintf("value = 0x%x\n", 
				sym_entryp->coff_symbol.n_value);

			break;

		case SHT_REL:
			dprintf("SHT_REL - ");
			sym_entryp->coff_symbol.n_type = (DT_FCN << 4) | T_INT;
			sym_entryp->coff_symbol.n_sclass = C_EXT;
			sym_entryp->coff_symbol.n_value =
				sym_entryp->elf_symbol->st_value;
			break;

		case SHT_SHLIB:
			dprintf("SHT_SHLIB - ");
			sym_entryp->coff_symbol.n_type = (DT_FCN << 4) | T_INT;
			sym_entryp->coff_symbol.n_sclass = C_EXT;
			sym_entryp->coff_symbol.n_value =
				sym_entryp->elf_symbol->st_value;
			break;

		case SHT_DYNSYM:
			dprintf("SHT_DYNSYM - ");
			sym_entryp->coff_symbol.n_type = (DT_FCN << 4) | T_INT;
			sym_entryp->coff_symbol.n_sclass = C_EXT;
			sym_entryp->coff_symbol.n_value =
				sym_entryp->elf_symbol->st_value;
			break;

		case SHT_NUM:
			dprintf("SHT_NUM - ");
			sym_entryp->coff_symbol.n_type = (DT_FCN << 4) | T_INT;
			sym_entryp->coff_symbol.n_sclass = C_EXT;
			sym_entryp->coff_symbol.n_value =
				sym_entryp->elf_symbol->st_value;
			break;

		case SHT_MOD:
			dprintf("SHT_MOD - ");
			sym_entryp->coff_symbol.n_type = (DT_FCN << 4) | T_INT;
			sym_entryp->coff_symbol.n_sclass = C_EXT;
			sym_entryp->coff_symbol.n_value =
				sym_entryp->elf_symbol->st_value;
			break;

		default:
			dprintf("default - ");
			sym_entryp->coff_symbol.n_type = (DT_FCN << 4) | T_INT;
			sym_entryp->coff_symbol.n_sclass = C_EXT;
			sym_entryp->coff_symbol.n_value =
				sym_entryp->elf_symbol->st_value;
			break;
		}

		dprintf("\n");

		break;
	}
}


STATIC void
output_relocation_table(
int elf_fd,
int coff_fd
)
{
	int			num_relocation_entries;
	unsigned		i;
	struct relocation_entry	*rentryp;
	int			reloc_offset;
	int			count;
	int			rel_name_len;
	Elf32_Shdr		*rel_shdr;
	Elf32_Shdr		*shdr;
	char			*relocation_prefix;
	struct scnhdr		*output_scnhdrp;
	struct section_entry	*sentryp;

	dprintf("-------- RELOCATION TABLES ---------\n");

	rel_shdr = s_header;

	relocation_prefix = ".rel";

	rel_name_len = strlen(relocation_prefix);

	for (i = 0 ; i < e_header->e_shnum ; i++, rel_shdr++)
	{
		if (rel_shdr->sh_type != SHT_REL)
		{
			continue;
		}

		dprintf("Looking for %s\n", elf_section_string_table +
			rel_shdr->sh_name + rel_name_len);

		/*
		 * Find the ELF section that matches this relocation section
		 */
		shdr = elf_find_section_name(elf_section_string_table +
			rel_shdr->sh_name + rel_name_len);

		if (shdr == NULL)
		{
			fprintf(stderr, "relocation for unknown section %s\n",
				elf_section_string_table + rel_shdr->sh_name);

			exit(1);
		}

		/*
		 * Find the COFF section that corresponds to this ELF Section
		 */
		sentryp = find_coff_section_header(shdr);

		if (sentryp == NULL)
		{
			fprintf(stderr, 
                              "Could not find COFF section(in %s)\n",
		              elf_section_string_table + 
                              rel_shdr->sh_name + 
                              rel_name_len);

			exit(1);
		}

		num_relocation_entries =
			rel_shdr->sh_size / sizeof(Elf32_Rel);

		dprintf("num_relocation_entries = 0x%x\n",
			num_relocation_entries);

		dprintf("seeking to 0x%x\n", rel_shdr->sh_offset);

		(void)lseek(elf_fd, rel_shdr->sh_offset, SEEK_SET);

		rentryp = create_relocation_chain(elf_fd,
			num_relocation_entries);

		sentryp->reloc_entry = rentryp;
	}

	/* -------- TEXT ------- */

	count = 0;

	output_scnhdrp = NULL;

	reloc_offset = lseek(coff_fd, 0, SEEK_CUR);

	/*
	 * Write out .text
	 */
	for (sentryp = section_entry_head ; sentryp != NULL ; 
		sentryp = sentryp->next)
	{
		if (get_section_type(sentryp) != TEXT_SECTION)
		{
			continue;
		}

		count += output_relocation_chain(coff_fd, sentryp);

		if (output_scnhdrp == NULL)
		{
			output_scnhdrp = sentryp->coff_scnhdr;
		}
	}

	dprintf("number of relocation table entries 0x%x\n", count);

	dprintf("relocation table offset 0x%x\n", reloc_offset);

	if (count != 0)
	{
		output_scnhdrp->s_nreloc = (unsigned short)count;

		output_scnhdrp->s_relptr = reloc_offset;
	}

	/* -------- DATA ------- */

	count = 0;

	output_scnhdrp = NULL;

	reloc_offset = lseek(coff_fd, 0, SEEK_CUR);

	/*
	 * Write out .text
	 */
	for (sentryp = section_entry_head ; sentryp != NULL ; 
		sentryp = sentryp->next)
	{
		if (get_section_type(sentryp) != DATA_SECTION)
		{
			continue;
		}

		count += output_relocation_chain(coff_fd, sentryp);

		if (output_scnhdrp == NULL)
		{
			output_scnhdrp = sentryp->coff_scnhdr;
		}
	}

	dprintf("number of relocation table entries 0x%x\n", count);

	dprintf("relocation table offset 0x%x\n", reloc_offset);

	if (count != 0)
	{
		output_scnhdrp->s_nreloc = (unsigned short)count;

		output_scnhdrp->s_relptr = reloc_offset;
	}
}

int
output_relocation_chain(
int			coff_fd,
struct section_entry	*sentryp
)
{
	int			count;
	struct scnhdr		*scnhdrp;
	struct relocation_entry	*rentryp;

	count = 0;

	scnhdrp = sentryp->coff_scnhdr;

	rentryp = sentryp->reloc_entry;

	/*
	 * COFF has per section relocation tables.
	 */
	while (rentryp != NULL)
	{
		rentryp->coff_reloc.r_vaddr += scnhdrp->s_vaddr;

		write(coff_fd, &rentryp->coff_reloc,
			sizeof(struct coff_reloc));

		rentryp = rentryp->next;

		count++;
	}

	return count;
}

struct relocation_entry	*
create_relocation_chain(
int		elf_fd,
int		num_relocation_entries
)
{
	unsigned			i;
	struct relocation_entry		*rentryp;
	struct relocation_entry		*rentryp_head;
	struct relocation_entry		*rentryp_tail;
	unsigned			symbol_index;

	rentryp_head = NULL;

	rentryp_tail = NULL;

	for (i = 0 ; i < num_relocation_entries ; i++)
	{
		rentryp = (struct relocation_entry *)
			malloc(sizeof(struct relocation_entry));

		if (read(elf_fd, &rentryp->elf_reloc, sizeof(Elf32_Rel)) !=
			sizeof(Elf32_Rel))
		{
			fprintf(stderr, "Relocation table entry read failed\n");

			exit(1);
		}

		/* RST - remove this */
		(void)memset(&rentryp->coff_reloc, i + 0xaa,
			sizeof(struct coff_reloc));

		dprintf("type = 0x%x - ",
			ELF32_R_TYPE(rentryp->elf_reloc.r_info));

		symbol_index = ELF32_R_SYM(rentryp->elf_reloc.r_info);

		dprintf("elf symbol_index = 0x%x - ", symbol_index);

		rentryp->coff_reloc.r_symndx =
			elf_index_to_coff_symbol[symbol_index]->coff_index;

#if 0
		if (rentryp->coff_reloc.r_symndx == 0)
		{
			fprintf(stderr, "Bad relocation index\n");

			exit(1);
		}
#endif

		dprintf("coff symbol_index = 0x%x - ",
			rentryp->coff_reloc.r_symndx);

		rentryp->coff_reloc.r_vaddr = rentryp->elf_reloc.r_offset;

		rentryp->sym_entry = elf_index_to_coff_symbol[symbol_index];

		switch (ELF32_R_TYPE(rentryp->elf_reloc.r_info))
		{
		case R_386_NONE:
			break;

		case R_386_32:
			dprintf("R_386_32\n");
			rentryp->coff_reloc.r_type = R_DIR32;
			break;

		case R_386_PC32:
			dprintf("R_386_PC32\n");
			rentryp->coff_reloc.r_type = R_PCRLONG;
			break;

		case R_386_GOT32:
			dprintf("R_386_GOT32\n");
			break;

		case R_386_PLT32:
			dprintf("R_386_PLT32\n");
			break;

		case R_386_COPY:
			dprintf("R_386_COPY\n");
			break;

		case R_386_GLOB_DAT:
			dprintf("R_386_GLOB_DAT\n");
			break;

		case R_386_JMP_SLOT:
			dprintf("R_386_JMP_SLOT\n");
			break;

		case R_386_RELATIVE:
			dprintf("R_386_RELATIVE\n");
			break;

		case R_386_GOTOFF:
			dprintf("R_386_GOTOFF\n");
			break;

		case R_386_GOTPC:
			dprintf("R_386_GOTPC\n");
			break;

		case R_386_NUM:
			dprintf("R_386_NUM\n");
			break;

		default:
			dprintf("unknown\n");
			break;
		}

		if (rentryp_head == NULL)
		{
			rentryp_head = rentryp;
			rentryp_tail = rentryp;
			rentryp->next = NULL;
		}
		else
		{
			rentryp_tail->next = rentryp;
			rentryp_tail = rentryp;
			rentryp->next = NULL;
		}
	}

	return rentryp_head;
}

void
output_symbol_table(
int elf_fd,
int coff_fd
)
{
	Elf32_Shdr		*elf_sym_sheader;
	int			num_symbols;
	int			i;
	struct	filehdr		coff_filehdr;
	int			symbol_offset;
	struct symbol_entry	*sym_entryp;

	elf_sym_sheader = elf_find_section(SHT_SYMTAB, 0);

	if (elf_sym_sheader == NULL)
	{
		dprintf("No symbols\n");

		return;
	}

	num_symbols = elf_sym_sheader->sh_size / sizeof(Elf32_Sym);

	dprintf("num_symbols = 0x%x\n", num_symbols);

	(void)lseek(elf_fd, elf_sym_sheader->sh_offset, SEEK_SET);

	symbol_offset = lseek(coff_fd, 0, SEEK_CUR);

	dprintf("symbol_offset = 0x%x\n", symbol_offset);

	for (i = 0 ; i < total_symbols ; i++)
	{
		dprintf("writing out %s\n", coff_symbol_table[i]->name);

		write(coff_fd, &(coff_symbol_table[i]->coff_symbol),
			sizeof(struct coff_syment));
	}

	/*
	 * Output the COFF file header
	 */
	(void)lseek(coff_fd, 0, SEEK_SET);

	(void)memset(&coff_filehdr, 0, sizeof(struct filehdr));

	coff_filehdr.f_magic = I386MAGIC;

	coff_filehdr.f_nscns = (unsigned short)coff_num_sections;

	coff_filehdr.f_timdat = time(0);

	coff_filehdr.f_symptr = symbol_offset;

	coff_filehdr.f_nsyms = total_symbols;

	coff_filehdr.f_opthdr = 0;

	coff_filehdr.f_flags = F_LNNO | F_AR32WR;

	write(coff_fd, &coff_filehdr, sizeof(struct filehdr));

	/*
	 * Output the extended symbol table.
	 */
	(void)lseek(coff_fd, 0, SEEK_END);

	sym_entryp = long_name_head;

	/* Output the extended symbol table */

	write(coff_fd, &ext_sym_offset, sizeof(unsigned));

	while (sym_entryp != NULL)
	{
		dprintf("entry 0x%x longname: %s 0x%x\n", sym_entryp,
			sym_entryp->name, sym_entryp->name);

		write(coff_fd, sym_entryp->name, strlen(sym_entryp->name) + 1);

		sym_entryp = sym_entryp->next_long_name;
	}
}

void
output_coff_section_data(
int	elf_fd,
int	coff_fd
)
{
	struct section_entry	*sentryp;
	int			file_offset;
	struct section_entry	*first_sentryp;
	int			section_size;
	struct section_entry	*first_data_sentryp;
	struct scnhdr		*scnhdrp;

	dprintf("------- Writing section data --------\n");

	file_offset = sizeof(struct filehdr);

	file_offset += sizeof(struct scnhdr) * coff_num_sections;

	(void)lseek(coff_fd, file_offset, SEEK_SET);

	first_sentryp = NULL;
	section_size = 0;

	/*
	 * Write out .text
	 */
	for (sentryp = section_entry_head ; sentryp != NULL ; 
		sentryp = sentryp->next)
	{
		if (get_section_type(sentryp) != TEXT_SECTION)
		{
			continue;
		}

		if (first_sentryp == NULL)
		{
			first_sentryp = sentryp;
		}

		section_size = section_size + sentryp->coff_scnhdr->s_size;

		write_section_data(elf_fd, coff_fd, sentryp);
	}

	if (first_sentryp != NULL)
	{
		first_sentryp->coff_scnhdr->s_size = section_size;
	}

	first_sentryp = NULL;
	section_size = 0;

	/*
	 * Write out .data
	 */
	for (sentryp = section_entry_head ; sentryp != NULL ; 
		sentryp = sentryp->next)
	{
		if (get_section_type(sentryp) != DATA_SECTION)
		{
			continue;
		}

		if (first_sentryp == NULL)
		{
			first_sentryp = sentryp;

			first_data_sentryp = sentryp;
		}

		section_size = section_size + sentryp->coff_scnhdr->s_size;

		write_section_data(elf_fd, coff_fd, sentryp);
	}

	if (first_sentryp != NULL)
	{
		first_sentryp->coff_scnhdr->s_size = section_size;
	}

	first_sentryp = NULL;
	section_size = 0;

	/*
	 * Calculate bss
	 */
	for (sentryp = section_entry_head ; sentryp != NULL ; 
		sentryp = sentryp->next)
	{
		if (get_section_type(sentryp) != BSS_SECTION)
		{
			continue;
		}

		if (first_sentryp == NULL)
		{
			first_sentryp = sentryp;
		}

		section_size = section_size + sentryp->coff_scnhdr->s_size;
	}

	if (first_sentryp != NULL)
	{
		first_sentryp->coff_scnhdr->s_size = section_size;

		first_sentryp->coff_scnhdr->s_paddr = 
			first_data_sentryp->coff_scnhdr->s_paddr + 
			first_data_sentryp->coff_scnhdr->s_size;

		first_sentryp->coff_scnhdr->s_vaddr = 
			first_sentryp->coff_scnhdr->s_paddr;
	}
	else
	{
		sentryp = (struct section_entry *)
			malloc(sizeof(struct section_entry));

		sentryp->next = NULL;

		scnhdrp = (struct scnhdr *)calloc(1, sizeof(struct scnhdr));

		dprintf("Creating COFF section\n");

		sentryp->coff_scnhdr = scnhdrp;

		sentryp->coff_section_index = 3;

		if (section_entry_head == NULL)
		{
			section_entry_head = sentryp;

			section_entry_tail = sentryp;
		}
		else
		{
			section_entry_tail->next = sentryp;

			section_entry_tail = sentryp;
		}

		sprintf(scnhdrp->s_name, ".bss");

		scnhdrp->s_flags = STYP_BSS;

		sentryp->elf_scnhdr = (Elf32_Shdr *)
			malloc(e_header->e_shentsize);

		sentryp->elf_scnhdr->sh_flags = SHF_ALLOC;

		sentryp->elf_scnhdr->sh_type = SHT_NOBITS;
	}
}

void
write_section_data(
int			elf_fd,
int			coff_fd,
struct section_entry	*sentryp
)
{
	char			*data;

	dprintf("writing section %s 0x%x\n",
		sentryp->coff_scnhdr->s_name,
		sentryp->coff_scnhdr);

	data = (char *)malloc(sentryp->elf_scnhdr->sh_size);

	(void)lseek(elf_fd, sentryp->elf_scnhdr->sh_offset, SEEK_SET);

	if (read(elf_fd, data, sentryp->elf_scnhdr->sh_size) !=
		sentryp->elf_scnhdr->sh_size)
	{
		fprintf(stderr, "read of section data failed\n");

		exit(1);
	}

	fixup_relocation(sentryp, data);

	dprintf("Section starts 0x%x\n",
		sentryp->coff_scnhdr->s_scnptr);

	(void)lseek(coff_fd, sentryp->coff_scnhdr->s_scnptr, SEEK_SET);

	write(coff_fd, data, sentryp->elf_scnhdr->sh_size);

	free(data);
}


void
fixup_relocation(
struct section_entry	*sentryp,
char			*data
)
{
	struct section_entry	*section_sentryp;
	struct relocation_entry	*rentryp;
	int			*reloc_data;
	Elf32_Shdr		*shdr;
	struct symbol_entry	*sym_entryp;

	rentryp = sentryp->reloc_entry;

	while (rentryp != NULL)
	{
		reloc_data = (int *)(data + 
			rentryp->elf_reloc.r_offset);

		sym_entryp = elf_index_to_coff_symbol[
			ELF32_R_SYM(rentryp->elf_reloc.r_info)];

		dprintf("%s 0x%x (sym = 0x%x: offset = 0x%x): Was 0x%x - ",
			sym_entryp->name,
			sym_entryp,
			ELF32_R_SYM(rentryp->elf_reloc.r_info),
			rentryp->elf_reloc.r_offset, *reloc_data);

		shdr = elf_find_section_name(sym_entryp->name);

		if (shdr != NULL)
		{
			dprintf("Reloc is section %s - ",
				sym_entryp->name);

			section_sentryp =
				find_coff_section_header(shdr);

			if (section_sentryp == NULL)
			{
				fprintf(stderr, "Could not find COFF "
					"section\n");

				exit(1);
			}
		}

		switch (ELF32_R_TYPE(rentryp->elf_reloc.r_info))
		{
		case R_386_PC32:
			dprintf("Fixing up PC32 - 0x%x + 0x%x ",
				rentryp->elf_reloc.r_offset,
				sym_entryp->elf_symbol->st_value);

			*reloc_data = *reloc_data - 
				rentryp->elf_reloc.r_offset +
				sym_entryp->elf_symbol->st_value;

			break;

		case R_386_32:
			dprintf("Fixing up 32 - 0x%x ",
				rentryp->sym_entry->coff_symbol.n_value);

			*reloc_data = *reloc_data +
				rentryp->sym_entry->coff_symbol.n_value;

			break;
		default:
			dprintf("No fixup needed - ");

			break;
		}

		dprintf("Is 0x%x\n", *reloc_data);

		rentryp = rentryp->next;
	}
}

void
output_coff_sections(
int	coff_fd
)
{
	struct section_entry	*sentryp;

	dprintf("------- Writing sections --------\n");

	(void)lseek(coff_fd, sizeof(struct filehdr), SEEK_SET);

	/*
	 * Write out .text
	 */
	for (sentryp = section_entry_head ; sentryp != NULL ; 
		sentryp = sentryp->next)
	{
		if ((sentryp->elf_scnhdr->sh_flags & SHF_ALLOC) != SHF_ALLOC)
		{
			continue;
		}

		if ((sentryp->elf_scnhdr->sh_flags & SHF_EXECINSTR) !=
			SHF_EXECINSTR)
		{
			continue;
		}

		write(coff_fd, sentryp->coff_scnhdr, sizeof(struct scnhdr));

		break;
	}

	/*
	 * Write out .data
	 */
	for (sentryp = section_entry_head ; sentryp != NULL ; 
		sentryp = sentryp->next)
	{
		if ((sentryp->elf_scnhdr->sh_flags & SHF_ALLOC) != SHF_ALLOC)
		{
			continue;
		}

		if ((sentryp->elf_scnhdr->sh_type == SHT_NOBITS) ||
			(sentryp->elf_scnhdr->sh_flags & SHF_EXECINSTR) ==
			SHF_EXECINSTR)
			
		{
			continue;
		}

		write(coff_fd, sentryp->coff_scnhdr, sizeof(struct scnhdr));

		break;
	}

	/*
	 * Write out .bss
	 */
	for (sentryp = section_entry_head ; sentryp != NULL ; 
		sentryp = sentryp->next)
	{
		if ((sentryp->elf_scnhdr->sh_flags & SHF_ALLOC) != SHF_ALLOC)
		{
			continue;
		}

		if (sentryp->elf_scnhdr->sh_type != SHT_NOBITS)
		{
			continue;
		}

		write(coff_fd, sentryp->coff_scnhdr, sizeof(struct scnhdr));

		break;
	}
}

struct section_entry *
find_coff_section_header(
Elf32_Shdr	*shdr
)
{
	struct section_entry	*sentryp;

	sentryp = section_entry_head;

	while (sentryp != NULL)
	{
		if (shdr == sentryp->elf_scnhdr)
		{
			return sentryp;
		}

		sentryp = sentryp->next;
	}

	return NULL;
}


Elf32_Shdr *
elf_find_section(
int type,
int flags
)
{
	Elf32_Shdr	*shdr;
	unsigned	i;
	
	shdr = s_header;

	for (i = 0 ; i < e_header->e_shnum ; i++, shdr++)
	{
		if ((shdr->sh_type == type) && (shdr->sh_flags == flags))
		{
			return shdr;
		}
	}

	return NULL;
}

Elf32_Shdr *
elf_find_section_name(
char	*section_name
)
{
	Elf32_Shdr	*shdr;
	unsigned	i;

	if (strlen(section_name) == 0)
	{
		return NULL;
	}

	shdr = s_header;

	for (i = 0 ; i < e_header->e_shnum ; i++, shdr++)
	{
		if (strcmp(elf_section_string_table + shdr->sh_name,
			section_name) == 0)
		{
			return shdr;
		}
	}

	return NULL;
}

/*
 * Parse the arguments for the program
 */
int
parseargs(
int	argc,
char	*argv[]
)
{
	int	c;
	int	errflag;

	errflag = 0;

	while ((c = getopt(argc, argv, "d?")) != EOF)
	{
		switch (c)
		{
		case 'd':
			debug = 1;
			break;

		case '?':
		default:
			errflag = 1;
			break;
		}
	}

	if (errflag)
	{
		usage();
	}

	return optind;
}


void
usage(
)
{
	fprintf(stderr, "usage: elf2coff infile outfile\n");
}

/*VARARGS*/
void
dprintf(va_alist)
va_dcl
{
	va_list	args;
	char	*fmt;

	if (debug == 0)
	{
		return;
	}

	va_start(args);

	fmt = va_arg(args, char *);

	vfprintf(stderr, fmt, args);

	fflush(stderr);

	va_end(args);
}

int
get_section_type(
struct section_entry	*sentryp
)
{
	if ((sentryp->elf_scnhdr->sh_flags & SHF_ALLOC) != SHF_ALLOC)
	{
		return UNKNOWN_SECTION;
	}

	if ((sentryp->elf_scnhdr->sh_flags & SHF_EXECINSTR) == SHF_EXECINSTR)
	{
		return TEXT_SECTION;
	}

	if (sentryp->elf_scnhdr->sh_type == SHT_NOBITS)
	{
		return BSS_SECTION;
	}

	return DATA_SECTION;
}

void
fix_static(
char	*coff_filename
)
{
	int	coff_fd;
	int	next_arg;

	coff_fd = open(coff_filename, O_RDWR);

	if (coff_fd < 0)
	{
		perror("open");

		fprintf(stdout, "could not open file %s\n", coff_filename);

		exit(1);
	}

	coff_verify(coff_fd);

	coff_read_header(coff_fd);

	coff_mod_syms(coff_fd);
}

void
coff_verify(
int	coff_fd
)
{
	read(coff_fd, &coff_filehdr, sizeof(struct filehdr));

	if (coff_filehdr.f_magic != I386MAGIC)
	{
		fprintf(stdout, "Not COFF object\n");

		exit(1);
	}

#if 0
	fprintf(stdout, "COFF object\n");

	fprintf(stdout, "------- COFF HEADER --------\n");

	fprintf(stdout, "magic    nscns    timdat   symptr   nsyms    opthdr   flags\n");

	fprintf(stdout, "%08x ", coff_filehdr.f_magic);
	fprintf(stdout, "%08x ", coff_filehdr.f_nscns);
	fprintf(stdout, "%08x ", coff_filehdr.f_timdat);
	fprintf(stdout, "%08x ", coff_filehdr.f_symptr);
	fprintf(stdout, "%08x ", coff_filehdr.f_nsyms);
	fprintf(stdout, "%08x ", coff_filehdr.f_opthdr);
	fprintf(stdout, "%08x ", coff_filehdr.f_flags);

	fprintf(stdout, "\n");
#endif
}

void
coff_read_header(
int coff_fd
)
{
	struct scnhdr	*shdr;
	unsigned	i;

	lseek(coff_fd, coff_filehdr.f_opthdr, SEEK_CUR);

	coff_scnhdrs = (struct scnhdr *)
		malloc(coff_filehdr.f_nscns * sizeof(struct scnhdr));

	read(coff_fd, coff_scnhdrs, 
		coff_filehdr.f_nscns * sizeof(struct scnhdr));


#if 0
	fprintf(stdout, "\n-------- SECTIONS ---------\n");
#endif

	shdr = coff_scnhdrs;

#if 0
	fprintf(stdout, "name     paddr    vaddr    size     scnptr   relptr   nreloc flags\n");

	for (i = 0 ; i < coff_filehdr.f_nscns ; i++, shdr++)
	{
		fprintf(stdout, "%8.8s ", shdr->s_name);
		fprintf(stdout, "%08x ", shdr->s_paddr);
		fprintf(stdout, "%08x ", shdr->s_vaddr);
		fprintf(stdout, "%08x ", shdr->s_size);
		fprintf(stdout, "%08x ", shdr->s_scnptr);
		fprintf(stdout, "%08x ", shdr->s_relptr);
		//fprintf(stdout, "%08x ", shdr->s_lnnoptr);
		fprintf(stdout, "%06x ", shdr->s_nreloc);
		//fprintf(stdout, "%06x ", shdr->s_nlnno);
		fprintf(stdout, "%08x\n", shdr->s_flags);
	}

	fprintf(stdout, "\n");
#endif
}

void
coff_mod_syms(
int	coff_fd
)
{
	int			i;
	int			j;
	struct coff_syment	*coff_sym;
	struct coff_syment	*symp;
	char			c;
	unsigned		extended_sym_size;
	char			*extended_sym_table;
	int			ret;
	int			aux;
	char			name[9];
	char			*sym_name;

#if 0
	fprintf(stdout, "\n-------- SYMBOL TABLE ---------\n");
#endif

	lseek(coff_fd, coff_filehdr.f_symptr, SEEK_SET);

#if 0
	fprintf(stdout, "  index: value    scnum type clss naux name\n");
#endif


	coff_sym = (struct coff_syment *)malloc(
		sizeof(struct coff_syment) * coff_filehdr.f_nsyms);

	if (coff_sym == NULL)
	{
		fprintf(stdout, "malloc failed\n");

		exit(1);
	}

	ret = read(coff_fd, coff_sym,
		sizeof(struct coff_syment) * coff_filehdr.f_nsyms);

	if (ret < 0)
	{
		fprintf(stdout, "Symbol table read failed\n");

		exit(1);
	}

	symp = coff_sym;

	read(coff_fd, &extended_sym_size, sizeof(unsigned));

#if 0
	fprintf(stdout, "extended sym size = 0x%x\n", extended_sym_size);
#endif

	if (extended_sym_size != 0)
	{
		extended_sym_table = (char *)malloc(extended_sym_size);

		read(coff_fd, extended_sym_table, extended_sym_size);
	}

	aux = 0;

	for (i = 0 ; i < coff_filehdr.f_nsyms ; i++, symp++)
	{
#if 0
		fprintf(stdout, "  %05x: ", i);

		fprintf(stdout, "%08x %04x  %04x %02x   %02x   ",
			(unsigned)symp->n_value,
			(unsigned short)symp->n_scnum,
			(unsigned short)symp->n_type,
			(unsigned)symp->n_sclass,
			(unsigned)symp->n_numaux);
#endif

		if (aux == 0)
		{
			aux = symp->n_numaux;
		}
		else
		{
#if 0
			for (j = 0 ; j < SYMNMLEN ; j++)
			{
				fprintf(stdout, "%02x ", 
					*((unsigned char *)&symp->n_name + j));
			}

			fprintf(stdout, "\n");
#endif
			aux--;
			continue;
		}

		if (symp->n_zeroes != 0)
		{
			for (j = 0 ; j < SYMNMLEN ; j++)
			{
				name[j] = *((unsigned char *)&symp->n_name + j);
			}

			name[8] = '\0';

			sym_name = name;
		}
		else
		{
#if 0
			fprintf(stdout, "0x%x - extended_sym_table 0x%x - "
				"size 0x%x\n", 
				extended_sym_table + symp->n_offset - 4,
				extended_sym_table,
				extended_sym_size);
			fflush(stdout);

			fprintf(stdout, "%s", extended_sym_table +
				symp->n_offset - 4);
#endif
			sym_name = extended_sym_table + symp->n_offset - 4;
		}

		if ((strcmp(sym_name, "udi_init_info") != 0) &&
			(strcmp(sym_name, "udi_meta_info") != 0))
		{
			continue;
		}

		symp->n_sclass = C_STAT;
	}

	lseek(coff_fd, coff_filehdr.f_symptr, SEEK_SET);

	ret = write(coff_fd, coff_sym,
		sizeof(struct coff_syment) * coff_filehdr.f_nsyms);

	free(coff_sym);
}
