/*
 * File: tools/freebsd/osdep_make.c
 *
 * FreeBSD-specific target file generation.
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
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include "y.tab.h"
#include "spparse.h"
#include "../common/common_api.h"       /* Common utility functions */
#include "osdep.h" /* tools/linux/osdep.h */
#include <udi_sprops_ra.h>
 
#define UDI_VERSION 0x101
#include <udi.h>

char name[513];
int desc_index;

void
make_driver_glue(char *fname, FILE *tmpf)
{
			fprintf(tmpf, "
/* 
 * Glue for a driver. 
 * It would be kind to call this a kludge.   There's substantial impedance
 * between FreeBSD bus enumeration and UDI's bus enumeration.   They're both
 * comprehensive and they both do too good of a job of hiding needed 
 * information from the other.   This is also WAY too cozy with PCI for my
 * taste.  :-(
 */

#define _KERNEL 1
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <pci/pcivar.h> /* for pci_get_* */
   static int 
   my_probe(device_t dev)
   {
	char *name = \"%s\";
	unsigned int v = pci_get_vendor(dev);
	unsigned int d = pci_get_device(dev);
	unsigned int r = pci_get_revid(dev);

	printf(\"Probing for %%x %%x %%x\\n\", v, d, r);
	if (strcmp(name, \"whipit\") == 0) {
		device_set_desc(dev, \"whipit\");
		printf(\"Claiming whipit\\n\");
		return 0;
	}
	if (!strcmp(name, \"shrkudi\") && (v == 0x1011) && (d == 0x0009)) {
		device_set_desc(dev, \"osicom/shrk 10/100\");
		printf(\"Claiming dev %%x\\n\", dev);
		return 0;
	}
	if (!strcmp(name, \"eeeudi\") && (v == 0x8086) && (d == 0x1229)) {
		device_set_desc(dev, \"Intel Pro 10/100\");
		printf(\"Claiming dev %%x\\n\", dev);
		return 0;
	}
	return ENXIO;
   }

  static int my_attach(device_t dev)
  {
        extern char *_udi_sprops_scn_for_%s;
        extern char *_udi_sprops_scn_end_for_%s;
	extern void *udi_init_info;
        int result;
        int sprops_size;

        sprops_size = (int) &_udi_sprops_scn_end_for_%s -
                        (int)&_udi_sprops_scn_for_%s;

	result = _udi_driver_load(\"%s\", &udi_init_info,
			  (char *)&_udi_sprops_scn_for_%s,
			   sprops_size, dev);

	return result;
  }

  static int my_detach(device_t dev)
  {
	_udi_driver_unload(\"%s\");
  }

  static device_method_t my_methods[] = {
	DEVMETHOD(device_probe, my_probe),
	DEVMETHOD(device_attach, my_attach),
	DEVMETHOD(device_detach, my_detach),
	{ 0, 0 }
  };
  static driver_t my_driver = {\"%s\", my_methods, 0};
  static devclass_t my_devclass;
  DRIVER_MODULE(%s, pci, my_driver, my_devclass, 0, 0);
", name, name, name, name, name, name, name, name, name, name );

}

void
make_other_glue(char *fname, FILE *tmpf, char *env_if, char* symn)
{
	fprintf(tmpf, "
  #define _KERNEL 1
  #include <sys/types.h>
  #include <sys/conf.h>
  #include <sys/param.h>
  #include <sys/kernel.h>
  #include <sys/module.h>
  #include <sys/bus.h>

  static int
  udi_load(struct module *mod, int action, void *arg)
  {
	extern void *udi_init_info;
	extern void *udi_meta_info;
	extern char *_udi_sprops_scn_for_%s;
	extern char *_udi_sprops_scn_end_for_%s;
	int result;
	int sprops_size;

	sprops_size = (int) &_udi_sprops_scn_end_for_%s - 
			(int)&_udi_sprops_scn_for_%s;

	switch (action) {
	case MOD_LOAD:
		result = _udi_%s_load(\"%s\", &%s, 
				  (char *)&_udi_sprops_scn_for_%s,
				   sprops_size);
		break;
	case MOD_UNLOAD:
		result = _udi_%s_unload(\"%s\");
	}
  }
  DEV_MODULE(%s, udi_load, 0);
	\n", name, name, name, name, env_if, name, symn, name, env_if, name, name);
}

void
make_glue_file(void)
{
	FILE *tmpf;

	tmpf = fopen("glue.c", "w");
	/* FIXME: error */
	assert(tmpf != NULL);

	/*
	 * UDI Core environment is "preglued".   Do nothing further.
	 */
	if (0 == strcmp(name, "udi")) 
		goto done;

	switch (udi_global.bintype) {
		case BIN_DRIVER:
			make_driver_glue(name, tmpf);
			break;
		case BIN_META:
			make_other_glue(name, tmpf, "meta", "udi_meta_info");
			break;
		case BIN_MAPPER:
			make_other_glue(name, tmpf, "driver", "udi_init_info");
			break;
		default:
			abort();
	}
done:
	fclose(tmpf);
}


void
osdep_mkinst_files(int install_src, 
	propline_t ** input, 
	char *basedir, 
	char *tmpdir)
{
	int index = 0;
	char *tbuf = alloca(513);
	char *estr;
	char *args[100];

	while (input[index] != NULL) {
		if (input[index]->type == SHORTNAME) {
			strcpy(name, input[index+1]->string);
		} else if (input[index]->type == NAME) {     
                        desc_index = input[index+1]->ulong;
		}
		while (input[index] != NULL) index++;
                index++;
	}

	if (chdir(tmpdir)) {
		printloc(stderr, UDI_MSG_FATAL, 2105, 
			 "Cannot chdir to %s\n", tmpdir);
	}
	
	estr = optinit();

	optcpy(&estr, basedir);
	optcat(&estr, "/");
	optcat(&estr, udi_global.comp_zzz);

	prepare_elf_object(estr, name, NULL);

	make_glue_file();
#if 1
	args[0] = "cc";
	args[1] = "-c";
	args[2] = "-I.";
/* 
 * FIXME: this is to pick up bus_if.h and device_if.h.   I have no 
 * understanding why freebsd builds a billion invariant files to do this...
 */

	args[3] = "-I/usr/src/sys";
	args[4] = "-I/usr/src/sys/compile/GENERIC/";
	args[5] = "glue.c";
	args[6] = NULL;
#else
args[0] = "cat";
args[1] = "glue.c";
args[2] = NULL;
#endif
	os_exec(NULL, args[0], args);

	args[0] = "gensetdefs";
	if (0 == strcmp(name, "udi")) {
		args[1] = estr;
	} else {
		args[1] = "glue.o";
	}
	args[2] = NULL;
	os_exec(NULL, args[0], args);

	args[0] = "cc";
	args[1] = "-c";
	args[2] = "setdef0.c";
	args[3] = "setdef1.c";
	args[4] = NULL;
	
	os_exec(NULL, args[0], args);

	args[0] = "ld";
	args[1] = "-Bshareable";
	args[2] = "setdef0.o";
	args[3] = estr;
	args[4] = "glue.o";
	args[5] = "setdef1.o";

	optcpy(&estr, name);
	optcat(&estr, ".ko");

	args[6] = "-o";
	args[7] = estr;
	args[8] = NULL;
	os_exec(NULL, args[0], args);
#if 0
	if (0 != strcmp(name, "udi")) {
		args[0] = "objcopy";
		args[0] = "--localize-symbol=udi_init_info";
		
	}
#endif
	args[0] = "rm";
	args[1] = "setdef0.c";
	args[2] = "setdef1.c";
	args[3] = "setdef0.o";
	args[4] = "setdef1.o";
	args[5] = "setdefs.h";
	args[6] = "glue.c";
	args[7] = NULL;
#if 0
	os_exec(NULL, args[0], args);
#endif

	if (!udi_global.stop_install) {
		args[0] = "/usr/bin/install";
		args[1] = "-c";
		args[2] = estr;
		args[3] = "/modules/";
		args[4] = NULL;
		os_exec(NULL, args[0], args);
	}
}
