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

#include	<libelf.h>
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
