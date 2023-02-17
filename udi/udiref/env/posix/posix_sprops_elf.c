
/*
 * File: env/posix/posix_sprops_elf.c
 *
 * UDI Posix Static Properties Elf-object-format interface
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

/*-----------------------------------------------------------------------
 *
 * If the current environment is a Posix build which uses ELF object file
 * formats...
 *
 * This file may be used to read the static driver properties from the
 * current source file (drv->drv_osinfo->driver_filename == posix_me).
 */

#include <udi_env.h>
#include "obj_osdep.h"			/* from tools */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <udi_std_sprops.h>

#ifndef UDI_ABI_NON_ELF_UDIPROPS_SECTION
/* _udi_get_elf_sprops
 *
 * Given the name of an object file (either the Posix executable or
 * the name of the module) and the driver object name this routine
 * attempts to find the associated properties block for that module
 * by getting the .udiprops section from the object file and then
 * finding the properties section for the specified driver.
 *
 * Note that the linking stage for the various modules in the Posix
 * target will concatenate the .udiprops sections together.  This code
 * assumes:
 * 1) The udibuild has pre-processed the udiprops.txt file to create
 *    the .udiprops section so that the .udiprops is well formatted and
 *    highly regular (e.g. no comments or extraneous whitespace, etc.).
 * 2) Each property is a null terminated string within the block
 * 3) All properties for a module are locally clustered and the
 *    "properties_version" property is the separator between modules.
 * 4) The correct properties for the specified driver can be determined
 *    by comparing the driver name to the module or shortname properties.
 */

_udi_std_sprops_t *
_udi_get_elf_sprops(const char *exec_img_file,
		    const char *driver_obj_file,
		    const char *driver_name)
{
	int objfile, ndx;
	Elf *elf;
	char *cptr, *secname;
	const char *ofname;
	void *ehdr, *shdr;
	Elf_Scn *scn;
	Elf_Data *elfdata;
	extern char elf_class;
	_udi_std_sprops_t *sprops = NULL;

	ofname = driver_obj_file;
	if (ofname) {
		objfile = open(driver_obj_file, O_RDONLY);
	} else {
		objfile = -1;
	}
	if (objfile < 0) {
		ofname = exec_img_file;
		objfile = open(exec_img_file, O_RDONLY);
		if (objfile < 0) {
			fprintf(stderr,
				"Error: couldn't open %s or %s to obtain "
				"static properties for %s\n", exec_img_file,
				driver_obj_file, driver_name);
			exit(ENOENT);
		}
	}

	if (elf_version(EV_CURRENT) == EV_NONE) {
		fprintf(stderr, "Error: Elf library out of date!\n");
		exit(1);
	}

	elf = elf_begin(objfile, ELF_C_READ, (Elf *) 0);
	if (elf_kind(elf) != ELF_K_ELF) {
		fprintf(stderr, "WARNING: %s is not an ELF format file!\n",
			ofname);
	      pickup_toys:
		elf_end(elf);
		close(objfile);
		exit(EBADF);
	}

	cptr = elf_getident(elf, NULL);
	elf_class = cptr[EI_CLASS];
	if ((ehdr = (void *)elf_getehdr(elf)) == 0) {
		fprintf(stderr, "Error: Unable to get ELF file header "
			"info for %s!\n", ofname);
		goto pickup_toys;
	}

	ndx = elf_geteshstrndx(ehdr);
	cptr = NULL;
	scn = 0;
	while ((scn = elf_nextscn(elf, (Elf_Scn *) scn)) != 0) {
		if ((shdr = (void *)elf_getshdr(scn)) != 0) {
			secname = elf_strptr(elf, ndx, elf_getsname(shdr));
			if (udi_strcmp(secname, ".udiprops") == 0) {
				for (elfdata = elf_getdata(scn, NULL);
				     elfdata != NULL;
				     elfdata = elf_getdata(scn, elfdata)) {
					sprops = udi_std_sprops_parse(elfdata->
								      d_buf,
								      elfdata->
								      d_size,
								      driver_name);
				}
				if (cptr)
					break;
			}
		}
	}

	elf_end(elf);
	close(objfile);
	return sprops;
}
#endif /*UDI_ABI_NON_ELF_UDIPROPS_SECTION*/

STATIC int
_udi_module_is_meta(char *sprops_text, int sprops_size, char **provides)
{
	char *sprops;
	int idx = 0;
	int i;

	sprops = sprops_text;
	for (idx = 0; idx < sprops_size; i++) {
		if (strncmp(&sprops[idx], "provides ",9) == 0) {
			*provides = &sprops[idx+9];
			return TRUE;
		}
		idx = idx + strlen(&sprops[idx]) + 1;
	}
	*provides = NULL;
	return FALSE;
}

void *
_udi_module_load(char *name, void *init_info, _OSDEP_DRIVER_T driver_info)
{
	struct _udi_driver *driver;
	udi_boolean_t is_meta;
	extern char *posix_me;
	char *provided_meta;

	driver_info.sprops = _udi_get_elf_sprops(posix_me, NULL, name);
	is_meta = _udi_module_is_meta(driver_info.sprops->props_text, 
					driver_info.sprops->props_len, 
					&provided_meta);
	driver_info.sprops->is_meta = is_meta;

	if (is_meta) {
		char provided_metab[512];
		char *s = provided_meta;
		char *d = provided_metab;

		/* 
		 * Copy the 'provides' name from the sprops to a local
		 * buffer.   This is  temporary until we can add an
		 * 'sprops_get_provides...' facility.
		 */
		while (*s != ' ') {
			*d++ = *s++;
		}
		*d = '\0';

		return _udi_MA_meta_load(provided_metab, init_info, 
					 driver_info);
	}
	
	driver = _udi_MA_driver_load(name, init_info, driver_info);
	if (driver != NULL) {
		int r;
		r = _udi_MA_install_driver(driver);
		if (r != UDI_OK) {
			return NULL;
		}
	}
	return driver;
}

/*
 * This has a rather unpleasant interface.  Unloading things by name (and
 * thus using that to decide if it's a meta or a driver) is inflexible 
 * becuase we could have multiple modules with the same name or even a 
 * driver and a meta with the same name.  But there's too much existing 
 * code that is wired to use this interface...
 */
void
_udi_module_unload_by_name(char *name, void *init_info)
{
	_OSDEP_DRIVER_T driver_info;
	extern char *posix_me;
	udi_boolean_t is_meta;
	char *provided_meta;

	driver_info.sprops = _udi_get_elf_sprops(posix_me, NULL, name);
	is_meta = _udi_module_is_meta(driver_info.sprops->props_text, 
					driver_info.sprops->props_len, 
					&provided_meta);
	if (is_meta) {
		_OSDEP_DRIVER_T meta_driver_info;
		char provided_metab[512];
                char *s = provided_meta;
                char *d = provided_metab;

                /* 
                 * Copy the 'provides' name from the sprops to a local
                 * buffer.   This is  temporary until we can add an
                 * 'sprops_get_provides...' facility.
                 */
                while (*s != ' ') {
                        *d++ = *s++;
                }
                *d = '\0';

		meta_driver_info = _udi_MA_meta_unload(provided_metab);
		udi_std_sprops_free(meta_driver_info.sprops);
		udi_std_sprops_free(driver_info.sprops);
		return;
	}
	_udi_MA_destroy_driver_by_name(name);
	udi_std_sprops_free(driver_info.sprops);
}
