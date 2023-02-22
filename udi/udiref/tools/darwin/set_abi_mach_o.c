/*      
 * File: tools/darwin/set_abi_mach_o.c
 *              
 * UDI Package Properties Attach Functions
 */     

/*
 * Copyright 2001, Steven Bytnar <sbytnar@users.sourceforge.net>
 * Submitted for merger with udiref via BSD License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <mach-o/loader.h>
#include <mach-o/getsect.h>
#include <sys/stat.h>


/*#define STANDALONE for testing as a standalone binary */

#ifndef STANDALONE
#include "global.h"
#include "osdep.h"
#include "udi_abi.h" 
#include "common_api.h" 
#else

#if !defined(UDI_ABI_MACH_O_UDIPROPS_SECTION) && !defined(UDI_ABI_ELF_UDIPROPS_SECTION)
#error NO_ABI__UDIPROPS_SECTION_DEFINED
#endif

/*#define UDI_ABI_MACH_O_UDIPROPS_SECTION*/
struct udi_global {
	int verbose;
};
struct udi_global udi_global = { 1 };
#endif

#ifdef UDI_ABI_MACH_O_UDIPROPS_SECTION
/*
 * If a new section is added to a LC_SEGMENT, we must adjust all following
 * load command information. This is not feasible, especially if the
 * object module format changes.
 * The strategy this code takes is to create a simple OBJECT file with
 * 1 segment & 1 section, and then merge it into the final binary with
 * objcopy. The code that would otherwise be necessary to readjust
 * section information is punted to objcopy then.
 */

#if 0
/*  Mach-O file format: */
struct mach_o_file
{
	struct mach_header header;
	union {
		char *load_commands_size[this->header.sizeofcmds];
		struct {
			struct load_command[];
			char *load_command_data[this->load_cmds[].cmdsize];
		} load_cmds[some_constant];
	} all_load_cmds;
};
union {
	struct {
		struct segment_command[no_constant];
		struct section[this->segment_command[].nsects];
	} segment_cmds[no_constant]; /* til header.? */
} LC_SEGMENT_load_command;
#endif

struct simple_obj_file_t
{
	struct mach_header header;
	struct segment_command segment;
	struct section section;
};

void write_simple_obj_file_info(struct simple_obj_file_t *obj,
				unsigned long sizeof_simple_obj)
{
	struct mach_header *header= &obj->header;
	struct segment_command *segment= &obj->segment;
	struct section *section = &obj->section;

	/* Make sure the caller filled in the section info */
	assert(section->size != 0);
	assert(section->offset == sizeof(struct simple_obj_file_t));

	/* Erase any grunge */
	memset(obj, 0, sizeof(struct mach_header)
			+ sizeof(struct segment_command));

	/* Write the Mach-O header */
	header->magic = MH_MAGIC;
	/*
	 * TBD: Write a FAT header for this if we want
	 * to support FAT Mach-O.
	 */
#if defined(__ppc__)
	header->cputype = CPU_TYPE_POWERPC;
#elif defined(__i386__)
	header->cputype = CPU_TYPE_I386;
#endif
	header->cpusubtype = 0;
	header->filetype = MH_OBJECT;
	header->ncmds = 1;
	header->sizeofcmds = sizeof(struct segment_command)
				+ sizeof(struct section)
				+ section->size;
	header->flags = 0;
	
	/* Write the first load_command, a segment_command. */
	segment->cmd = LC_SEGMENT;
	segment->cmdsize = sizeof(struct segment_command)
				+ sizeof(struct section);
	strncpy(segment->segname, "", 16); /* first segment */
	/* Is this offset and size correct? Seems right. *shrug* */
	segment->fileoff = sizeof(struct simple_obj_file_t);
	segment->filesize = section->size;
	segment->maxprot = VM_PROT_READ;
	segment->initprot = VM_PROT_READ;
	segment->nsects = 1;
	segment->flags = 0;
}

#define SEG_UDIPROPS "__UDIPROPS"
#define SECT_UDIPROPS "__udiprops"
void write_udiprops_section_info(struct simple_obj_file_t *filedataptr,
				struct section *sectp,
				char *props_sectname,
				char *props,
				unsigned long props_size)
{
	strncpy(sectp->sectname, props_sectname, 16);
	strncpy(sectp->segname, SEG_UDIPROPS, 16);
	sectp->addr = 0;
	sectp->size = props_size;
	sectp->offset = sizeof(*filedataptr);
	sectp->align = 0; /* 2^0 = 1 byte alignment. We want everything to be butted back to back. */
	sectp->reloff = 0;
	sectp->nreloc = 0; /* no relocation entries */
	sectp->flags = 0; /*S_CSTRING_LITERALS;*/
	sectp->reserved1 = 0;
	sectp->reserved2 = 0;

	memcpy((char*)sectp + sizeof(struct section), props, props_size);
}


/*
 * Creates a UDI properties section in a Mach-O OBJECT file,
 * stuffs it full with the contents of src, and then merges
 * the fabricated object file with the Mach-O dst file using ld.
 *
 * Inputs:	src is a udiprops file.
 *		dst is a Mach-O file that we can link against.
 */
void
set_abi_sprops(char *src, char *dst)
{
	FILE *binfile;
	struct simple_obj_file_t *objfiledata;
	unsigned long objfiledata_size;
	char *props_data;
	int props_size;
	char binfilename[1024];

#if 1
	abi_make_string_table_sprops(src, &props_data, &props_size);

#else
	txtfile = fopen(src, "rb");
	assert(txtfile);
#endif
	tmpnam(binfilename);
	binfile = fopen(binfilename, "w+b");
	assert(binfile);

#if 0
	if (stat(src, &sb) != 0)
		perror("Failed stat src");
	props_size = sb.st_size;
#if ADD_NULL_TERMINATOR
	props_size += 1; /* with added null terminator */
	extras++;
#endif

	props_data = malloc(props_size);
	assert(props_data);

	size = fread(props_data, 1, props_size - extras, txtfile);
	fclose(txtfile);
	assert(size == props_size - extras);
#if ADD_NULL_TERMINATOR
	props_data[props_size - extras] = 0; /* add null terminator */
#endif

	while (size)
	{
		/* translate all newlines to terminators */
		if (props_data[size-1] == '\n')
			props_data[size-1] == 0;
		size--;
	}
#endif

	objfiledata_size = sizeof(struct simple_obj_file_t) + props_size;
	objfiledata = (struct simple_obj_file_t*)malloc(objfiledata_size);
	assert(objfiledata);


	write_udiprops_section_info(objfiledata,
                                &objfiledata->section,
                                SECT_UDIPROPS,
                                props_data,
                                props_size);

	write_simple_obj_file_info(objfiledata, objfiledata_size);

	fwrite(objfiledata, 1, objfiledata_size, binfile);

	free(objfiledata);
	free(props_data);
	fclose(binfile);

	/*
	 * At this point, we've created an output file, but
	 * it needs to be merged with the original binary now.
	 */
	{
	/* TBD: convert to os_exec equivalents. */
		char tmpname[1024];
		char big_command[1024];
		int err;
		tmpnam(tmpname);
		sprintf(big_command, "ld -r -o \"%s\" \"%s\" \"%s\" ",
			tmpname,
			dst,
			binfilename);
		if (udi_global.verbose) printf("    ++%s\n", big_command);
		err = system(big_command);
		assert(err == 0);

		sprintf(big_command, "mv \"%s\" \"%s\" ", tmpname, dst);
		if (udi_global.verbose) printf("    ++%s\n", big_command);
		err = system(big_command);
		assert(err == 0);
		unlink(tmpname);
	}
	unlink(binfilename);
}

/*
 * Pulls a UDI properties section out of a Mach-O OBJECT file,
 * and stores it in a text file named dst.
 *
 * Inputs:	src is a Mach-O file that has a UDIPROPS section.
 * 		dst is a udiprops file that we'll write to.
 */
void
get_abi_sprops(char *src, char *dst)
{
	FILE *srcfile, *dstfile;
	struct stat sb;
	char *srcdata, *sectdata;
	struct section thissect;
	unsigned long size;

	srcfile = fopen(src, "rb");
	assert(srcfile);
	dstfile = fopen(dst, "w+b");
	assert(dstfile);

	/* Read in the entire binary */
	if (stat(src, &sb) != 0)
		perror("Failed stat src");
	srcdata = malloc(sb.st_size);
	assert(srcdata);
	size = fread(srcdata, 1, sb.st_size, srcfile);
	assert(size == sb.st_size);

	/* Walk around the Mach-O file. */
	memcpy((void*)&thissect,
			getsectbynamefromheader((struct mach_header*)srcdata,
						 SEG_UDIPROPS, SECT_UDIPROPS),
			sizeof(thissect));
	if (udi_global.verbose)
	{
		printf("MACH-O SECTION INFO START:   addr:%ld   size:%ld  offset:%ld\n",
			thissect.addr,
			thissect.size,
			thissect.offset);
	}
	sectdata = ((char*)srcdata) + thissect.offset;
	size = thissect.size;
	fwrite(sectdata, 1, thissect.size, dstfile);
	if (udi_global.verbose)
	{
#if 0
		int lineno = 1;
		while (size)
		{
			int len;
			len  = strlen(sectdata) + 1;
			if (len != 1) printf("%d:%s\n", lineno, sectdata);
			lineno++;
			size -= len;
			sectdata += len;
		}
		printf("MACH-O SECTION INFO END\n");
#endif
	}
	free(srcdata);
	fclose(srcfile);
	fclose(dstfile);
}

void    
abi_add_section_sizes(char *objfile, char *modname,   
                      unsigned int *ttl_code_size,
                      unsigned int *ttl_data_size)
{
	/* TBD: Implement ELF-like section size checking. */
	(void)objfile;
	(void)modname;
	(void)ttl_code_size;
	(void)ttl_data_size;
}

#ifdef STANDALONE
int
main(int argc, char ***argv)
{
	set_abi_sprops("gencat", "gencat.o");
	get_abi_sprops("gencat.o", "gencat.txt");
	system("cat gencat.txt");
	return 0;
}
#endif

#endif /* UDI_ABI_MACH_O_UDIPROPS_SECTION */

