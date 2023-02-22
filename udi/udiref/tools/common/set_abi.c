/*
 * File: tools/common/set_abi.c
 *
 * UDI Package Properties Attach Functions
 */

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

#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "osdep.h"
#include "udi_abi.h"
#include "common_api.h"

/* This file contains the various ABI-specific functions used to
   attach the static properties to the primary module file. */

#include <fcntl.h>

/*
 * Create a malloc'd memory block filled with the sprops src (a filepath) translated
 * into a suitable form for inclusion into a object file in a sprops section.
 */
void abi_make_string_table_sprops(char *src, void **strtable_sprops,
					size_t *strtable_size)
{
	int cptr_size, cptr_index;
	FILE *inpfp;
	char *cptr;
	char buf[513];
	int buflen;
	int d_size;

	cptr_size = 1024;
	cptr_index = 0;
	if ((cptr = (char *)malloc(cptr_size)) == NULL) {
	    printloc(stderr, UDI_MSG_FATAL, 1801, "unable to malloc "
		             "for string table of size %d!\n",
		             cptr_size);
	}
	/* ABI. String tables must begin with a NULL */
	cptr[cptr_index] = '\0';
	cptr_index++;
	d_size = 1;
	/* Open the input file */
	if ((inpfp = fopen(src, "r")) == NULL) {
		printloc(stderr, UDI_MSG_FATAL, 1800, "Unable to open file "
			         "%s for reading!\n", src);
	}
	/* Read it in */
	while (fgets(buf, 512, inpfp)) {
		if (strncmp(buf, "source_files ", 13) == 0 ||
				strncmp(buf, "compile_options ", 16) == 0 ||
				strncmp(buf, "source_requires ", 16) == 0)
			/*
			 * Do not put source-building static property lines
			 * into a binary file.
			 */
			continue;
		if (buf[strlen(buf)-1] == '\n')
			buf[strlen(buf)-1] = '\0';
		buflen = strlen(buf) + 1;
		if (cptr_index + buflen > cptr_size) {
			/* Allocate more memory */
			cptr_size += 1024;
			if ((cptr = (char *)realloc(cptr, cptr_size)) == NULL) {
				printloc(stderr, UDI_MSG_FATAL, 1801,
					         "unable to realloc for "
						 "string table of size %d!\n",
						 cptr_size);
			}
		}
		memcpy(&cptr[cptr_index], buf, buflen);
		cptr_index += buflen;
		d_size += buflen;
	}
	fclose(inpfp);
	/* We are done */
	*strtable_sprops = cptr;
	*strtable_size = d_size;
}

#ifdef UDI_ABI_ELF_UDIPROPS_SECTION


char	elf_class;

/*
 * Operations related to the elf object file header
 */
static void *elf_getehdr (Elf *elf);
static int elf_geteshstrndx (void *ehdr);
static void *elf_getshdr (Elf_Scn *scn);
static void *elf_getshdr (Elf_Scn *scn);
static size_t elf_getsname (void *shdr);
static void elf_setsname (void *shdr, int ndx);
static int elf_snotempty (void *shdr);
static int elf_stestflag(void *shdr, int flagmask);
static void elf_putnewdata (Elf_Data *edata, char *src);

/* Return a ptr to the file header */
static void *
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
static int
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
static void *
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
static size_t
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
static void
elf_setsname (void *shdr, int ndx)
{
	if (elf_class == ELFCLASS32) {
		((Elf32_Shdr *)(shdr))->sh_name = ndx;
	} else if (elf_class == ELFCLASS64) {
		((Elf64_Shdr *)(shdr))->sh_name = ndx;
	}
}


/* Test if the specified flag(s) are set in the flags of the section
 * header string table */
static int
elf_stestflag(void *shdr, int flagmask)
{
	if (elf_class == ELFCLASS32) {
		return((((Elf32_Shdr *)(shdr))->sh_flags) & flagmask);
	} else if (elf_class == ELFCLASS64) {
		return((((Elf64_Shdr *)(shdr))->sh_flags) & flagmask);
	}
	return (0);
}

/* Test if the section is empty in the section header string table */
static int
elf_snotempty (void *shdr)
{
	if (elf_class == ELFCLASS32) {
		return((((Elf32_Shdr *)(shdr))->sh_size) > 0);
	} else if (elf_class == ELFCLASS64) {
		return((((Elf64_Shdr *)(shdr))->sh_size) > 0);
	}
	return (0);
}

static void
elf_putnewdata (Elf_Data *edata, char *src)
{
	abi_make_string_table_sprops(src, &edata->d_buf, &edata->d_size);
}


/*
 * [Re]creates a UDI properties section in the Elf file dst, and populates it
 * with the contents of src.
 */
void
set_abi_sprops(char *src, char *dst)
{
	int fildes;
	Elf *elf;
	Elf_Scn *scn;
	void *ehdr;
	int ndx;
	void *shdr;
	char *secname;
	char *cptr;
	Elf_Data *edata, *ddata;
	extern char elf_class;
	int found, secnameoffset, size;

	if (elf_version(EV_CURRENT) == EV_NONE) {
		printloc (stderr, UDI_MSG_FATAL, 1802,
			          "Elf library out of date!\n");
	}
	if((fildes = open(dst, O_RDWR)) < 0) {
		printloc (stderr, UDI_MSG_FATAL, 1800,
			  "Unable to open file %s!\n", dst);
	}
	elf = elf_begin(fildes, ELF_C_RDWR, (Elf *)0);
	if (elf_kind(elf) != ELF_K_ELF) {
		printloc (stderr, UDI_MSG_FATAL, 1803, 
			          "%s is not an ELF format file!\n", dst);
	}
	cptr = elf_getident(elf, NULL);
	elf_class = cptr[EI_CLASS];
	elf_flagelf(elf, ELF_C_SET, ELF_F_DIRTY);
	if ((ehdr = (void *)elf_getehdr(elf)) == 0) {
		printloc (stderr, UDI_MSG_FATAL, 1804, 
			          "Unable to get ELF file header "
			          "info for %s!\n", src);
	}
	ndx = elf_geteshstrndx(ehdr);
	scn = 0;
	found = 0;
	while ((scn = elf_nextscn(elf, (Elf_Scn *)scn)) != 0) {
		if ((shdr = (void *)elf_getshdr(scn)) != 0) {
			secname = elf_strptr(elf, ndx, elf_getsname(shdr));
			if (strcmp(secname, ".udiprops") == 0) {
				/* Zero it out */
				edata = 0;
				edata = elf_getdata(scn, edata);
				elf_putnewdata (edata, src);

				/* Made udiprops.txt, so no need to continue */
				found = 1;
				break;
			}
		}
	}
	if (! found) {
		/*
		 * Create a new section named .udiprops
		 */
		/* First, add the string to the string section, if not there */
		scn = elf_getscn(elf, ndx);
/* 		shdr = (void *)elf_getshdr(scn); */
		edata = 0;
		edata = elf_getdata(scn, edata);
		secnameoffset = 1;	/* The first byte is always a null */
		found = 0;
		while (found == 0 && secnameoffset < edata->d_size) {
			if (strcmp(((char *)edata->d_buf)+secnameoffset,
					".udiprops") != 0)
				secnameoffset +=
					strlen(((char *)edata->d_buf)+
							secnameoffset) + 1;
			else
				found = 1;
		}
		if (! found) {
			/* Section name does not exist */
			if ((cptr = (char *)malloc(edata->d_size+
					strlen(".udiprops")+1)) == NULL) {
			        printloc(stderr, UDI_MSG_FATAL, 1805,
					         "unable to malloc space "
					         "for Elf string data!\n");
			}
			memcpy(cptr, edata->d_buf, edata->d_size);
			strcpy(cptr+edata->d_size, ".udiprops");
			secnameoffset = edata->d_size;
			size = edata->d_size;
			edata->d_size = size + strlen(".udiprops") + 1;
			edata->d_buf = cptr;
		}

		/* We have the section name, so create the new section now */
		if ((scn = elf_newscn(elf)) == NULL) {
			printloc (stderr, UDI_MSG_FATAL, 1806,
				          "unable to create a new elf "
					  "section in file %s!\n", dst);
		}
		shdr = (void *)elf_getshdr(scn);
		/* Put the new data into dptr */
		if ((ddata = elf_newdata(scn)) == NULL) {
			printloc(stderr, UDI_MSG_FATAL, 1807,
				         "unable to create a new elf "
					 "section data in file %s!\n", dst);
		}
		elf_putnewdata(ddata, src);
		elf_setsname(shdr, secnameoffset);
 		if (elf_class == ELFCLASS32)
 		{
 			((Elf32_Shdr *)shdr)->sh_type = SHT_PROGBITS;
 		}
 		else
 		{
 			((Elf64_Shdr *)shdr)->sh_type = SHT_PROGBITS;
 		}
	}

	elf_update(elf, ELF_C_WRITE);
	elf_end(elf);
	close(fildes);
}

/*
 * Scans the Elf format file src for the UDI static properties section,
 * and outputs a text file equivalent in the file dest.
 */
void
get_abi_sprops(char *src, char *dst)
{
	int i, fildes;
	FILE *outfp;
	Elf *elf;
	Elf_Scn *scn;
	void *ehdr;
	int ndx;
	void *shdr;
	char *secname;
	char *cptr;
	Elf_Data *edata;
	extern char elf_class;

	if (elf_version(EV_CURRENT) == EV_NONE) {
		printloc (stderr, UDI_MSG_FATAL, 1802,
			          "Elf library out of date!\n");
	}
	if((fildes = open(src, O_RDONLY)) < 0) {
		printloc (stderr, UDI_MSG_FATAL, 1800,
			  "Unable to open file %s!\n", src);
	}
	elf = elf_begin(fildes, ELF_C_READ, (Elf *)0);
	if (elf_kind(elf) != ELF_K_ELF) {
		printloc (stderr, UDI_MSG_WARN, 1803, 
			          "%s is not an ELF format file!\n", src);
		elf_end(elf);
		close(fildes);
		return;
	}
	cptr = elf_getident(elf, NULL);
	elf_class = cptr[EI_CLASS];
	if ((ehdr = (void *)elf_getehdr(elf)) == 0) {
		printloc (stderr, UDI_MSG_FATAL, 1804, 
			          "Unable to get ELF file header "
			          "info for %s!\n", src);
		exit(1);
	}
	ndx = elf_geteshstrndx(ehdr);
	scn = 0;
	while ((scn = elf_nextscn(elf, (Elf_Scn *)scn)) != 0) {
		if((shdr = (void *)elf_getshdr(scn)) != 0) {
			secname = elf_strptr(elf, ndx, elf_getsname(shdr));
			if (strcmp(secname, ".udiprops") == 0) {
				/* Open the output file */
				if ((outfp = fopen(dst, "w+")) == NULL) {
					printloc(stderr, UDI_MSG_FATAL, 1808,
						         "Unable to open "
						         "file %s for "
						         "appending!\n", dst);
				}
				edata = 0;
				edata = elf_getdata(scn, edata);
				cptr = edata->d_buf;
				if (*cptr != '\0' && fprintf(outfp, "%s\n",
						cptr) < 0) {
					printloc (stderr, UDI_MSG_FATAL, 1809,
						          "Unable to "
							  "write to output "
						          "file %s!\n", dst);
				}
				for (i = 0; i < edata->d_size-1; i++, cptr++) {
					if (*cptr == '\0') {
						i++;
						cptr++;
						if (*cptr != '\0' &&
								fprintf(outfp,
								"%s\n", cptr) <
								0) {
							printloc (stderr,
								  UDI_MSG_FATAL,
								  1809,
								  "Unable "
								  "to write to "
								  "output file "
								  "%s!\n", dst);
						}
					}
				}
				fclose(outfp);
				/* Made udiprops.txt, so no need to continue */
				break;
			}
		}
	}
	elf_end(elf);
	close(fildes);
}


/* Verifies that the accumulated section sizes in the Elf file don't
 * exceed the ABI warning threshold; if they do, generate the
 * warning. */

#include <sys/stat.h>
#include <unistd.h>

static void
check_abi32_sizes(char *fname, char *modname, Elf *elf, int ndx,
		  unsigned int *textsize,
		  unsigned int *datasize)
{
	Elf_Scn *scn;
	Elf32_Shdr *shdr;
/*     Elf32_Word textsize, datasize; */
	struct stat filestats;
	
	scn = 0;
	*textsize = 0;
	*datasize = 0;
	
	while ((scn = elf_nextscn(elf, (Elf_Scn *)scn)) != 0) {
		if ((shdr = (void *)elf_getshdr(scn)) == 0) continue;
		if (elf_stestflag(shdr, SHF_WRITE) && elf_snotempty(shdr)) {
		    printloc(stderr, UDI_MSG_WARN, 1814,
			     "section `%s' of %s contains 0x%x bytes "
			     "of writeable data\n",
			     elf_strptr(elf, ndx, elf_getsname(shdr)),
			     modname, shdr->sh_size);
		}
		if (elf_stestflag(shdr, SHF_EXECINSTR)) {
			if (shdr->sh_size > UDI_ABI_MAX_MOD_CODE) {
				printloc(stderr, UDI_MSG_WARN, 1815,
					 "code section `%s' of %s "
					 "contains 0x%x bytes; exceeds "
					 "ABI guarantee of 0x%x bytes\n",
					 elf_strptr(elf, ndx,
						    elf_getsname(shdr)),
					 modname, shdr->sh_size,
					 UDI_ABI_MAX_MOD_CODE);
			} else {
				if (shdr->sh_size >
				    (UDI_ABI_MAX_MOD_CODE * 
				     ABI_SIZE_WARN_THRESH)) {
					printloc(stderr, UDI_MSG_WARN, 1816,
						 "code section `%s' of %s "
						 "contains 0x%x bytes; "
						 "approaching ABI guarantee "
						 "of 0x%x bytes\n",
						 elf_strptr(elf, ndx,
							    elf_getsname(shdr)),
						 modname, shdr->sh_size, 
						 UDI_ABI_MAX_MOD_CODE);
				}
			}
			*textsize += shdr->sh_size;
		} else {
			if (elf_stestflag(shdr, SHF_ALLOC)) {
				if (shdr->sh_size > UDI_ABI_MAX_MOD_DATA) {
					printloc(stderr, UDI_MSG_WARN, 1817,
						 "data section `%s' of %s "
						 "contains 0x%x bytes; exceeds "
						 "ABI guarantee "
						 "of 0x%x bytes\n",
						 elf_strptr(elf, ndx,
							    elf_getsname(shdr)),
						 modname, shdr->sh_size,
						 UDI_ABI_MAX_MOD_DATA);
				} else {
					if (shdr->sh_size >
					    (UDI_ABI_MAX_MOD_DATA * 
					     ABI_SIZE_WARN_THRESH)) {
						printloc(stderr, UDI_MSG_WARN,
							 1818,
							 "code section `%s' "
							 "of %s "
							 "contains 0x%x bytes; "
							 "approaching ABI "
							 "guarantee of "
							 "0x%x bytes\n",
							 elf_strptr(elf, ndx,
								    elf_getsname(shdr)),
							 modname, shdr->sh_size,
							 UDI_ABI_MAX_MOD_DATA);
					}
				}
				*datasize += shdr->sh_size;
			}
		}
	}
	
	if (*textsize > UDI_ABI_MAX_MOD_CODE) {
		printloc(stderr, UDI_MSG_WARN, 1819,
			 "code sections of %s contain 0x%x bytes; "
			 "exceeds ABI guarantee of 0x%x bytes\n",
			 modname, *textsize, UDI_ABI_MAX_MOD_CODE);
	} else {
		if (*textsize > (UDI_ABI_MAX_MOD_CODE * ABI_SIZE_WARN_THRESH)) {
			printloc(stderr, UDI_MSG_WARN, 1820,
				 "code sections of %s contain 0x%x bytes; "
				 "approaching ABI guarantee of 0x%x bytes\n",
				 modname, *textsize, UDI_ABI_MAX_MOD_CODE);
		}
	}
	
	if (*datasize > UDI_ABI_MAX_MOD_DATA) {
		printloc(stderr, UDI_MSG_WARN, 1821,
			 "data sections of %s contain 0x%x bytes; "
			 "exceeds ABI guarantee of 0x%x bytes\n",
			 modname, *datasize, UDI_ABI_MAX_MOD_DATA);
	} else {
		if (*datasize > (UDI_ABI_MAX_MOD_DATA * ABI_SIZE_WARN_THRESH)) {
			printloc(stderr, UDI_MSG_WARN, 1822,
				 "data sections of %s contain 0x%x bytes; "
				 "approaching ABI guarantee of 0x%x bytes\n",
				 modname, *datasize, UDI_ABI_MAX_MOD_DATA);
		}
	}
	
	if (!stat(fname, &filestats)) {
		if (filestats.st_size > UDI_ABI_MAX_MOD_FILE) {
			printloc(stderr, UDI_MSG_WARN, 1823,
				 " %s contains 0x%x bytes; "
				 "exceeds ABI guarantee of 0x%x bytes\n",
				 modname, filestats.st_size,
				 UDI_ABI_MAX_MOD_FILE);
		} else {
			if (*datasize > (UDI_ABI_MAX_MOD_FILE *
					 ABI_SIZE_WARN_THRESH)) {
				printloc(stderr, UDI_MSG_WARN, 1824,
					 " %s contains 0x%x bytes; "
					 "approaching ABI guarantee "
					 "of 0x%x bytes\n",
					 modname, filestats.st_size,
					 UDI_ABI_MAX_MOD_FILE);
			}
		}
	}
}

static void
check_abi64_sizes(char *fname, char *modname, Elf *elf, int ndx,
		  unsigned int *textsize,
		  unsigned int *datasize)
{
	Elf_Scn *scn;
	Elf64_Shdr *shdr;
/*     Elf64_Xword textsize, datasize; */
	struct stat filestats;
	
	scn = 0;
	*textsize = 0;
	*datasize = 0;
	
	while ((scn = elf_nextscn(elf, (Elf_Scn *)scn)) != 0) {
		if ((shdr = (void *)elf_getshdr(scn)) == 0) continue;
		if (elf_stestflag(shdr, SHF_WRITE) && elf_snotempty(shdr)) {
		    printloc(stderr, UDI_MSG_WARN, 1814,
			     "section `%s' of %s contains 0x%x bytes "
			     "of writeable data\n",
			     elf_strptr(elf, ndx, elf_getsname(shdr)),
			     modname, shdr->sh_size);
		}
		if (elf_stestflag(shdr, SHF_EXECINSTR)) {
			if (shdr->sh_size > UDI_ABI_MAX_MOD_CODE) {
				printloc(stderr, UDI_MSG_WARN, 1815,
					 "code section `%s' of %s "
					 "contains 0x%x bytes; exceeds "
					 "ABI guarantee of 0x%x bytes\n",
					 elf_strptr(elf, ndx,
						    elf_getsname(shdr)),
					 modname, shdr->sh_size,
					 UDI_ABI_MAX_MOD_CODE);
			} else {
				if (shdr->sh_size >
				    (UDI_ABI_MAX_MOD_CODE *
				     ABI_SIZE_WARN_THRESH)) {
					printloc(stderr, UDI_MSG_WARN, 1816,
						 "code section `%s' of %s "
						 "contains 0x%x bytes; "
						 "approaching ABI guarantee "
						 "of 0x%x bytes\n",
						 elf_strptr(elf, ndx,
							    elf_getsname(shdr)),
						 modname, shdr->sh_size,
						 UDI_ABI_MAX_MOD_CODE);
				}
			}
			*textsize += shdr->sh_size;
		} else {
			if (elf_stestflag(shdr, SHF_ALLOC)) {
				if (shdr->sh_size > UDI_ABI_MAX_MOD_DATA) {
					printloc(stderr, UDI_MSG_WARN, 1817,
						 "data section `%s' of %s "
						 "contains 0x%x bytes; "
						 "exceeds ABI guarantee "
						 "of 0x%x bytes\n",
						 elf_strptr(elf, ndx,
							    elf_getsname(shdr)),
						 modname, shdr->sh_size,
						 UDI_ABI_MAX_MOD_DATA);
				} else {
					if (shdr->sh_size >
					    (UDI_ABI_MAX_MOD_DATA *
					     ABI_SIZE_WARN_THRESH)) {
						printloc(stderr, UDI_MSG_WARN,
							 1818,
							 "code section `%s' "
							 "of %s "
							 "contains 0x%x bytes; "
							 "approaching ABI"
							 "guarantee "
							 "of 0x%x bytes\n",
							 elf_strptr(elf, ndx,
								    elf_getsname(shdr)),
							 modname, shdr->sh_size,
							 UDI_ABI_MAX_MOD_DATA);
					}
				}
				*datasize += shdr->sh_size;
			}
		}
	}
	
	if (*textsize > UDI_ABI_MAX_MOD_CODE) {
		printloc(stderr, UDI_MSG_WARN, 1819,
			 "code sections of %s contain 0x%x bytes; "
			 "exceeds ABI guarantee of 0x%x bytes\n",
			 modname, *textsize, UDI_ABI_MAX_MOD_CODE);
	} else {
		if (*textsize > (UDI_ABI_MAX_MOD_CODE *
				 ABI_SIZE_WARN_THRESH)) {
			printloc(stderr, UDI_MSG_WARN, 1820,
				 "code sections of %s contain 0x%x bytes; "
				 "approaching ABI guarantee of 0x%x bytes\n",
				 modname, *textsize, UDI_ABI_MAX_MOD_CODE);
		}
	}
	
	if (*datasize > UDI_ABI_MAX_MOD_DATA) {
		printloc(stderr, UDI_MSG_WARN, 1821,
			 "data sections of %s contain 0x%x bytes; "
			 "exceeds ABI guarantee of 0x%x bytes\n",
			 modname, *datasize, UDI_ABI_MAX_MOD_DATA);
	} else {
		if (*datasize > (UDI_ABI_MAX_MOD_DATA *
				 ABI_SIZE_WARN_THRESH)) {
			printloc(stderr, UDI_MSG_WARN, 1822,
				 "data sections of %s contain 0x%x bytes; "
				 "approaching ABI guarantee of 0x%x bytes\n",
				 modname, *datasize, UDI_ABI_MAX_MOD_DATA);
		}
	}
	
	if (!stat(fname, &filestats)) {
		if (filestats.st_size > UDI_ABI_MAX_MOD_FILE) {
			printloc(stderr, UDI_MSG_WARN, 1823,
				 " %s contains 0x%x bytes; "
				 "exceeds ABI guarantee of 0x%x bytes\n",
				 modname, filestats.st_size,
				 UDI_ABI_MAX_MOD_FILE);
		} else {
			if (*datasize > (UDI_ABI_MAX_MOD_FILE *
					 ABI_SIZE_WARN_THRESH)) {
				printloc(stderr, UDI_MSG_WARN, 1824,
					 " %s contains 0x%x bytes; "
					 "approaching ABI guarantee "
					 "of 0x%x bytes\n",
					 modname, filestats.st_size,
					 UDI_ABI_MAX_MOD_FILE);
			}
		}
	}
}


void
abi_add_section_sizes(char *objfile, char *modname,
		      unsigned int *ttl_code_size,
		      unsigned int *ttl_data_size)
{
	int fildes;
	Elf *elf;
	void *ehdr;
	int ndx;
	void *shdr;
	char *cptr;
	extern char elf_class;
	unsigned int codesize = 0, datasize = 0;
	
	if (elf_version(EV_CURRENT) == EV_NONE) {
		printloc (stderr, UDI_MSG_FATAL, 1802,
			  "Elf library out of date!\n");
	}
	if((fildes = open(objfile, O_RDWR)) < 0) {
		printloc (stderr, UDI_MSG_FATAL, 1800,
			  "Unable to open file %s!\n", objfile);
	}
	elf = elf_begin(fildes, ELF_C_READ, (Elf *)0);
	if (elf_kind(elf) != ELF_K_ELF) {
		printloc (stderr, UDI_MSG_FATAL, 1803, 
			  "%s is not an ELF format file!\n", objfile);
	}
	cptr = elf_getident(elf, NULL);
	elf_class = cptr[EI_CLASS];
	elf_flagelf(elf, ELF_C_SET, ELF_F_DIRTY);
	if ((ehdr = (void *)elf_getehdr(elf)) == 0) {
		printloc (stderr, UDI_MSG_FATAL, 1804, 
			  "Unable to get ELF file header "
			  "info for %s!\n", objfile);
	}
	ndx = elf_geteshstrndx(ehdr);

	if (elf_class == ELFCLASS32)
		check_abi32_sizes(objfile, modname, elf, ndx, &codesize, &datasize);
	else if (elf_class == ELFCLASS64)
		check_abi64_sizes(objfile, modname, elf, ndx, &codesize, &datasize);

	*ttl_code_size += codesize;
	*ttl_data_size += datasize;

	elf_end(elf);
	close(fildes);
}

/*
 * Add a symbol table entry with a given name, binding and value.
 */
static void
add_elf_symbol(int binding, Elf_Scn *strscn, Elf_Scn *symscn, char *name, long val, int shndx)
{
        Elf32_Off new_off;
        Elf_Data *data, *symdata;
        Elf32_Sym *symp;
	int i;
	Elf_Data *curdata = NULL;
	Elf_Data *curstring = NULL;
	Elf32_Sym *cursym;
	char *curstr;

	curdata = elf_getdata(symscn, curdata);
	cursym = (Elf32_Sym *) curdata->d_buf;
	curstring = elf_getdata(strscn, curstring);
	curstr = curstring->d_buf;
	
	for (i = 0; i < curdata->d_size / sizeof(Elf32_Sym); i++) {
		if (strcmp(curstr + cursym[i].st_name, name) == 0) {
			cursym[i].st_value = val;
			cursym[i].st_shndx = shndx;
			cursym[i].st_info = ELF32_ST_INFO(binding, STT_OBJECT);
			return;
		}
	}

        new_off = elf32_getshdr(strscn)->sh_size;
	data = elf_newdata(strscn);
	symdata = elf_newdata(symscn);
	symp = calloc(1, sizeof (Elf32_Sym));

        data->d_buf = name;
        data->d_type = ELF_T_BYTE;
        data->d_size = strlen(name) + 1;
        data->d_align = 1;

        /*
         * Populate a new symtab entry, make it a global that's
         * relative to the section index shndx.
         */
        symp->st_name = new_off;
        symp->st_value = val;
        symp->st_size = 0;
        symp->st_shndx = shndx;
        symp->st_info = ELF32_ST_INFO(binding, STT_OBJECT);

        /*
         * Tell libelf how to append this symtab entry.
         */

        symdata->d_buf = symp;
        symdata->d_type = ELF_T_SYM;;
        symdata->d_size = sizeof(*symp);
        symdata->d_align = 1;
}

#ifndef ELF64_ST_INFO
#define add_elf64_symbol add_elf_symbol
#else
static void
add_elf64_symbol(int binding, Elf_Scn *strscn, Elf_Scn *symscn, char *name,
		 Elf64_Addr val, int shndx)
{
	Elf64_Xword	new_off;
	Elf_Data	*data, *symdata;
	Elf64_Sym	*symp = calloc(1, sizeof(Elf64_Sym));
	Elf64_Shdr	*shdr;

	shdr = elf64_getshdr(strscn);
	new_off = shdr->sh_size;
	data = elf_newdata(strscn);
	symdata = elf_newdata(symscn);

	data->d_buf = name;
	data->d_type = ELF_T_BYTE;
	data->d_size = strlen(name) + 1;
	data->d_align = 1;

	/*
	 * Populate a new symtab entry, make it a global that's relative
	 * to the new section index shndx.
	 */
	symp->st_name = new_off;
	symp->st_value = val;
	symp->st_size = 0;
	symp->st_shndx = shndx;
	symp->st_info = ELF64_ST_INFO(binding, STT_OBJECT);

	/*
	 * Tell libelf how to append this symtab entry.
	 */
	symdata->d_buf = symp;
	symdata->d_type = ELF_T_SYM;
	symdata->d_size = sizeof(*symp);
	symdata->d_align = 1;
}
#endif  /* ELF64_ST_INFO */

void 
prepare_elf_object(const char *fname, const char *modname, const char** symbols)
{
	Elf *elf;
	Elf_Scn *scn;
	Elf_Scn *symscn = NULL;
	Elf_Scn *strscn = NULL;
	Elf_Scn *propsscn;
	int fd;
	char *cptr;
	void *ehdr;
	void *shdr;
	int ndx;
	char *secname;
	char symname[100] = "_udi_sprops_scn_for_";
	char sizename[100] = "_udi_sprops_scn_end_for_";
	int is_elf64 = 0;
	size_t identsz;

	elf_version(EV_CURRENT);	
	if((fd = open(fname, O_RDWR)) < 0) {
		printloc(stderr, UDI_MSG_FATAL, 1202, "Cannot open %s!\n", 
			 fname);
        }

        elf = elf_begin(fd, ELF_C_RDWR, (Elf *)0);
        if (elf_kind(elf) != ELF_K_ELF) {
		/* FIXME: */
		printloc(stderr, UDI_MSG_FATAL, 2109,
			"Not an ELF file: %s\n", fname);
        }
	/*
	 * Determine if we're using ELF64. This is a convoluted test as we need
	 * to support those architecture/OS combinations which do not have full
	 * ELF64 support, and fake the values to ELF32 equivalents. We 
	 * explicitly check for ELF32 first to cater for this.
	 */
        cptr = elf_getident(elf, &identsz);
	elf_class = cptr[EI_CLASS];
	if (identsz < EI_NIDENT) {
		printloc(stderr, UDI_MSG_FATAL, 2110,
			"Invalid ELF identifier in %s\n", fname);
	}
	if (cptr[EI_CLASS] == ELFCLASS32){
		is_elf64 = 0;
	} else if (cptr[EI_CLASS] == ELFCLASS64) {
		is_elf64 = 1;
	}

        elf_flagelf(elf, ELF_C_SET, ELF_F_DIRTY);
        if ((ehdr = (void *)elf_getehdr(elf)) == 0) {
		printloc(stderr, UDI_MSG_FATAL, 2110,
			"Invalid ELF identifier in %s\n", fname);
        }

	ndx = elf_geteshstrndx(ehdr);
	scn = NULL;

	/*
	 * Make two passes over the section table becuase the .udiprops
	 * section manipulation depends on knowing the symtab and strtabs.
	 */

        while ((scn = elf_nextscn(elf, (Elf_Scn *)scn)) != 0) {
                if ((shdr = (void *)elf_getshdr(scn)) != 0) {
                        secname = elf_strptr(elf, ndx, elf_getsname(shdr));
			if (strcmp(secname, ".symtab") == 0) {
                                symscn = scn;
				continue;
			}
			if (strcmp(secname, ".strtab") == 0) {
                                strscn = scn;
				continue;
			}
		}
	}

	if ((symscn == NULL) || (strscn == NULL)) {
		printloc(stderr, UDI_MSG_FATAL, 2113, 
			 "Cannot find .symtab or .strtab in %s\n", fname);
	}

        if (symbols) {
		int index = 0;
		char *sym;
		while (symbols[index] != NULL) {
			sym = (char *)(symbols[index]);
#if defined (ELF64_ST_INFO)
			if (is_elf64) {
				Elf_Data *curdata = NULL;
	    			Elf64_Sym *cursym = NULL;
		    		Elf_Data *curstring = NULL;
				char *curstr;
	    			int i;
		    
		    		curdata = elf_getdata(symscn, curdata);
				cursym = (Elf64_Sym *) curdata->d_buf;
	    			curstring = elf_getdata(strscn, curstring);
		    		curstr = curstring->d_buf;
	
				for (i = 0; i < curdata->d_size / 
						sizeof(Elf64_Sym); i++) {
					if (strcmp(curstr + cursym[i].st_name, 
						   sym) == 0) {
						cursym[i].st_info = 
							ELF64_ST_INFO(
								STB_GLOBAL, 
								STT_OBJECT);
						elf_update(elf, ELF_C_NULL);
						break;
					}	
				}
			} else 
#endif /* ELF64_ST_INFO */
			{
				Elf_Data *curdata = NULL;
				Elf32_Sym *cursym = NULL;
				Elf_Data *curstring = NULL;
				char *curstr;
				int i;
	
				curdata = elf_getdata(symscn, curdata);
				cursym = (Elf32_Sym *) curdata->d_buf;
				curstring = elf_getdata(strscn, curstring);
				curstr = curstring->d_buf;
		    
				for (i = 0; i < curdata->d_size / 
						sizeof(Elf32_Sym); i++) {
		  			if (strcmp(curstr + cursym[i].st_name, 
						   sym) == 0) {
						cursym[i].st_info = 
							ELF32_ST_INFO(
								STB_GLOBAL, 
				                          	STT_OBJECT);
						elf_update(elf, ELF_C_NULL);
						break;
					}	
				}
			}
			index++;
		}
	}
				
	while ((scn = elf_nextscn(elf, (Elf_Scn *)scn)) != 0) {
                if ((shdr = (void *)elf_getshdr(scn)) != 0) {
                        secname = elf_strptr(elf, ndx, elf_getsname(shdr));
                        if (strcmp(secname, ".udiprops") == 0) {
				strcat (symname, modname);
				strcat (sizename, modname);
				propsscn = scn;
				
				/* mark it loadable */
				if (is_elf64) {
					((Elf64_Shdr *)shdr)->sh_flags |= 
						SHF_ALLOC;
					/* The symbol has to be global */
					add_elf64_symbol(STB_GLOBAL, strscn, 
					       symscn, symname, 
					       0, elf_ndxscn(propsscn));
					/* 
				 	 * We create a symbol at the end of the
				 	 * udiprops section.  This way we don't 
				 	 * have to add something to .data
				 	 */
        				elf_update(elf, ELF_C_NULL);
					add_elf64_symbol(STB_GLOBAL, strscn, 
					       symscn, sizename, 
					       ((Elf64_Shdr *)shdr)->sh_size,
					 	elf_ndxscn(propsscn));
				} else {
					((Elf32_Shdr *)shdr)->sh_flags |= 
						SHF_ALLOC;

					/* The symbol has to be global */
					add_elf_symbol(STB_GLOBAL, strscn, 
					       symscn, symname, 
					       0, elf_ndxscn(propsscn));
					/* 
				 	 * We create a symbol at the end of the
				 	 * udiprops section.  This way we don't 
				 	 * have to add something to .data
				 	 */
        				elf_update(elf, ELF_C_NULL);
					add_elf_symbol(STB_GLOBAL, strscn, 
					       symscn, sizename, 
					       ((Elf32_Shdr *)shdr)->sh_size,
					 	elf_ndxscn(propsscn));
				}
			}
		}
	}
        elf_update(elf, ELF_C_WRITE);
        elf_end(elf);
        close(fd);
}

#endif  /* defined UDI_ABI_ELF_UDIPROPS_SECTION */


#ifdef UDI_ABI_CAT_UDIPROPS

#ifndef OS_HAS_NO_CAT
/*
 * Attaches the UDI static properties to the object module image by means of
 * a simple concatenation operation.  The assumption is that this creates a
 * barnacle that doesn't bother COFF, a.out, or related tools too much and
 * that we can just search for "properties_version" in the file to find this
 * barnacle for get_abi_sprops later on.
 */
void
set_abi_sprops(char *src, char *dst) 
{
    char *catop;
    int rval;

    catop = optinit();
    optcpy(&catop, "cat ");
    optcat(&catop, dst);
    optcat(&catop, " ");
    optcat(&catop, src);
    optcat(&catop, " > ");
    optcat(&catop, dst);
    optcat(&catop, "-cat && mv ");
    optcat(&catop, dst);
    optcat(&catop, "-cat ");
    optcat(&catop, dst);
    if (udi_global.verbose) printf("    ++%s\n", catop);
    if ((rval = system(catop))) {
	printloc(stderr, UDI_MSG_ERROR, 1810, "error %d from cat of sprops\n",
		         rval);
	udi_tool_exit(rval);
    }
    free(catop);
}

#else /* OS_HAS_NO_CAT */




/* Concatenate one file onto the end of another. */
void
set_abi_sprops(char *src, char *dst) 
{
    int fds, fdd;
    char *ptr;
    char *buf[512];
    int sz;
    
    if ((fds = open(src, O_RDONLY)) < 0) {
	printloc (stderr, UDI_MSG_FATAL, 1811, "Unable to open file %s for "
		          " getting sprops\n", src);
    }
    
    if ((fdd = open(dst, O_APPEND|O_WRONLY, 0644)) < 0) {
	printloc(stderr, UDI_MSG_FATAL, 1812, "Unable to open output "
		         "props file %s", dst);
    }

    if (udi_global.verbose) printf("    ++append %s %s\n", src, dst, dst);
    
    /* Now, run through the src file, and copy to dst file */
    while((sz = read(fds, buf, 512)) != 0) {
	if (write(fdd, buf, sz) != sz) {
	    printloc(stderr, UDI_MSG_FATAL, 1813, "Error: Unable to write to "
		             "%s in cat request!\n", dst);
	}
    }
    
    close(fdd);
    close(fds);
}

#endif /* defined OS_HAS_NO_CAT */

#endif /* defined UDI_ABI_CAT_UDIPROPS */


#ifdef UDI_ABI_CAT_UDIPROPS

/*
 * Obtains from the end of the binary file, the UDI static properties,
 * which was added via a concatenation at the end of the file,
 * and outputs a text file equivalent in the file dest.
 */
void
get_abi_sprops(char *src, char *dst)
{
	FILE *srcfile, *dstfile;
	char input_line[514], match_str[] = "properties_version";
	int in_props, nextchar;

	srcfile = fopen(src, "r");
	if (srcfile == NULL)
		printloc(stderr, UDI_MSG_FATAL, 1811, "Unable to open file "
			         "%s for getting sprops\n", src);

	dstfile = fopen(dst, "w+");
	if (dstfile == NULL)
		printloc(stderr, UDI_MSG_FATAL, 1812, "Unable to open output "
			         "props file %s\n", dst);

	if (udi_global.verbose) printf("    ++extract %s > %s\n", src, dst);

	nextchar = 0;
	for (in_props = 0;
	     in_props < strlen(match_str) && nextchar != EOF; ) {
		if (nextchar == match_str[in_props]) in_props++;
		else in_props = 0;
		nextchar = fgetc(srcfile);
	}

	fwrite(match_str, strlen(match_str), 1, dstfile);
	fwrite(" ", 1, 1, dstfile);

	while (fgets(input_line, 513, srcfile)) {
		fputs(input_line, dstfile);
	}

	fclose(srcfile);
	fclose(dstfile);
}

#endif /* defined UDI_ABI_CAT_UDIPROPS */
