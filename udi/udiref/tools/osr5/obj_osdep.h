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

#include <libelf.h>
#include <syms.h>

/*
 * For UnixWare 7, these things do not exist.
 */
#define	elf64_getehdr(x)	NULL
#define	elf64_getshdr(x)	NULL
#define Elf64_Ehdr		Elf32_Ehdr
#define Elf64_Shdr		Elf32_Shdr

#define ROUNDUP(a, b)		(((a) + (b) - 1) & ~ (b - 1))

#ifndef STATIC
#ifdef DEBUG_ELF2COFF
#define STATIC
#else
#define STATIC static
#endif
#endif

/* RST - From Linux */
#define R_386_NONE	0
#define R_386_32	1
#define R_386_PC32	2
#define R_386_GOT32	3
#define R_386_PLT32	4
#define R_386_COPY	5
#define R_386_GLOB_DAT	6
#define R_386_JMP_SLOT	7
#define R_386_RELATIVE	8
#define R_386_GOTOFF	9
#define R_386_GOTPC	10
#define R_386_NUM	11

#define UNKNOWN_SECTION	0
#define TEXT_SECTION	1
#define DATA_SECTION	2
#define BSS_SECTION	3


#pragma pack(1)
struct coff_reloc {
	long	r_vaddr;	/* (virtual) address of reference  */
	long	r_symndx;	/* index into symbol table  */
	unsigned short	r_type;		/* relocation type  */
};

struct coff_syment
{
	union
	{
		char		_n_name[SYMNMLEN];	/* old COFF version  */
		struct
		{
			long	_n_zeroes;	/* new == 0  */
			long	_n_offset;	/* offset into string table  */
		} _n_n;
		char		*_n_nptr[2];	/* allows for overlaying  */
	} _n;
	unsigned
	long			n_value;	/* value of symbol  */
	short			n_scnum;	/* section number  */
	unsigned short		n_type;		/* type and derived type  */
	char			n_sclass;	/* storage class  */
	char			n_numaux;	/* number of aux. entries  */
};
#pragma pack()

struct chain {
	struct chain	*head;
	struct chain	*tail;
};

struct symbol_entry {
	Elf32_Sym		*elf_symbol;
	struct coff_syment	coff_symbol;
	char			*name;
	unsigned		coff_index;
	struct symbol_entry	*next_long_name;
	unsigned		elf_index;
};

struct relocation_entry {
	struct relocation_entry	*next;
	Elf32_Rel		elf_reloc;
	struct coff_reloc	coff_reloc;
	struct symbol_entry	*sym_entry;
};

struct section_entry {
	struct section_entry	*next;
	struct scnhdr		*coff_scnhdr;
	Elf32_Shdr		*elf_scnhdr;
	struct relocation_entry	*reloc_entry;
	unsigned		coff_section_index;
};

STATIC void
usage(
);

STATIC void
elf_verify(
int
);

STATIC void
elf_read_header(
int
);

STATIC void
output_coff_sections(
int
);

STATIC Elf32_Shdr *
elf_find_section(
int,
int
);

STATIC void
output_relocation_table(
int,
int
);

STATIC void
output_symbol_table(
int,
int
);

STATIC void
read_symbol_table(
int
);

STATIC void
process_symbol(
struct symbol_entry *
);

STATIC Elf32_Shdr *
elf_find_section_name(
char *
);

STATIC void
read_string_table(
int
);

STATIC struct relocation_entry	*
create_relocation_chain(
int,
int
);

STATIC void
create_coff_headers(
int
);

STATIC struct section_entry *
find_coff_section_header(
Elf32_Shdr *
);

STATIC void
output_coff_section_data(
int,
int
);

STATIC void
fixup_relocation(
struct section_entry *,
char *
);

STATIC void
write_section_data(
int,
int,
struct section_entry *
);

STATIC int
parseargs(
int,
char *[]
);

STATIC void
dprintf(
);

int
sym_compare(
const void *,
const void *
);

void
remove_extra_symbols(
);

int
get_section_type(
struct section_entry *
);

int
output_relocation_chain(
int,
struct section_entry *
);

void
free_memory(
);

void
coff_verify(
int
);

void
coff_read_header(
int
);

void
coff_mod_syms(
int
);

int
elf2coff(
char *,
char *
);

void
fix_static(
char *
);
