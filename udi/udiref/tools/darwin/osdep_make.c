/*
 * File: tools/linux/osdep_make.c
 *
 * Darwin-specific target file generation.
 */

/*
 * $Copyright udi_reference:
 * 
 * 
 *    Copyright (c) 1995-2000; Compaq Computer Corporation; Hewlett-Packard
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

#define KWQ_INIT_HACK		/* Hack to get init.o in udi_env.o for msgs output */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include "y.tab.h"
#include "spparse.h"
#include "../common/common_api.h"	/* Common utility functions */
#include "osdep_make.h"

#define UDI_VERSION 0x101
#include <udi.h>

char *pkg;
char modtype[256];

#define ADDARG(Argval) {						      \
    args[ii++] = (Argval);						      \
    if (ii >= (sizeof(args) / sizeof(*args))) {				      \
	fprintf(stderr, "ERROR: Tried to set too many arguments (%d)\n", ii); \
	exit(3);							      \
    }									      \
    args[ii] = NULL;							      \
}

#ifdef OS_DELETE_OVERRIDE
void os_delete(char *file)
{
    fprintf(stderr, "unlink('%s') disabled\n");
    return;
    if (unlink(file)) {
        printloc(stderr, UDI_MSG_FATAL, 1008, "unable to unlink file %s\n",
                         file);
    }
}
#endif

#define input_data udi_global.propdata
#define entries_t propline_t

void get_mod_type(char *modtype)
{
    int index, index2;

    index = 0;
    while (input_data[index] != NULL) {
	if (input_data[index]->type == CHLDBOPS) {
	    index2 = 0;
	    while (input_data[index2] != NULL) {
		if (input_data[index2]->type == META &&
		    input_data[index + 1]->ulong ==
		    input_data[index2 + 1]->ulong) {
		    if (strcmp(input_data[index2 + 2]->string,
			       "udi_scsi") == 0) {
			strcpy(modtype, "scsi");
		    } else if (strcmp(input_data[index2 + 2]->string,
				      "udi_nic") == 0) {
			if (strcmp(modtype, "scsi")
			    != 0)
			    strcpy(modtype, "nic");
		    } else if (strcmp(input_data[index2 + 2]->string,
				      "udi_comm") == 0) {
			if (strcmp(modtype, "scsi")
			    != 0 && strcmp(modtype, "nic")
			    != 0)
			    strcpy(modtype, "comm");
		    } else if (strcmp(input_data[index2 + 2]->string,
				      "udi_vid") == 0) {
			if (strcmp(modtype, "scsi")
			    != 0 && strcmp(modtype, "nic")
			    != 0 && strcmp(modtype, "comm")
			    != 0)
			    strcpy(modtype, "vid");
		    } else if (strcmp(input_data[index2 + 2]->string,
				      "udi_snd") == 0) {
			if (strcmp(modtype, "scsi")
			    != 0 && strcmp(modtype, "nic")
			    != 0 && strcmp(modtype, "comm")
			    != 0 && strcmp(modtype, "vid")
			    != 0)
			    strcpy(modtype, "snd");
		    } else if (strcmp(input_data[index2 + 2]->string,
				      "udi_gio") == 0) {
			if (strcmp(modtype, "scsi")
			    != 0 && strcmp(modtype, "nic")
			    != 0 && strcmp(modtype, "comm")
			    != 0 && strcmp(modtype, "vid")
			    != 0 && strcmp(modtype, "snd")
			    != 0)
			    strcpy(modtype, "misc");
		    }
		    break;
		}
		while (input_data[index2] != NULL)
		    index2++;
		index2++;
	    }
	}
	while (input_data[index] != NULL)
	    index++;
	index++;
    }
}

#define GLUE_HEADER 1
#define GLUE_FOOTER 0

static void
output_glue_code(FILE * fp, const char *modtype, int headerfooter)
{
/* headerfooter != 0 ? generate header : generate footer */
#if 0
    if (!strcmp(modtype, "scsi"))
	fprintf(fp, driver_glue);
    else if (!strcmp(modtype, "misc"))	/* gio */
	fprintf(fp, meta_glue);
    else
	fprintf(fp, "  return 0;\n");
#endif
    /* meta - udi_MA_meta_load,  driver - udi_MA_driver_load */
    /* mapper - udi_MA_mapper_load */

    /* Special case: We don't need to *_load for the udi_MA module. */

    if (udi_global.make_mapper == 0) {
	/* Not a mapper */
	if (!IS_META) {
	    /* For a driver */
#if 1	     /*DEBUG*/
		fprintf(fp, "printk(\"Glue code for driver '%s'\\n\");\n",
			modtype);
	    if (udi_global.verbose)
		printf("DRIVER GLUE generated\n");
#endif
	    if (headerfooter)
		fprintf(fp, driver_glue_head);
	    else
		fprintf(fp, driver_glue_foot);
	} else {
	    /* For a meta */
#if 1	     /*DEBUG*/
		fprintf(fp,
			"printk(\"Glue code for metalanguage '%s'\\n\");\n",
			modtype);
	    if (udi_global.verbose)
		printf("METALANGUAGE GLUE generated\n");
#endif
	    if (headerfooter)
		fprintf(fp, meta_glue_head);
	    else
		fprintf(fp, meta_glue_foot);
	}
    } else {
	/* Is a mapper */
#if 1	 /*DEBUG*/
	    fprintf(fp, "printk(\"Glue code for mapper '%s'\\n\");\n",
		    modtype);
	if (udi_global.verbose)
	    printf("MAPPER GLUE generated\n");
#endif
	if (headerfooter)
	    fprintf(fp, mapper_glue_head);
	else
	    fprintf(fp, mapper_glue_foot);
    }
}


/* 
 * This thing is a mess. It needs to be rewritten, because now it doesn't use either exec_cmd(LINK, ...)
 * or ADDARGS correctly. It works, but is really only a bunch of patches
 */

#undef input_data
char short_name[513];

void
osdep_mkinst_files(int install_src, entries_t ** input_data, char *basedir,
		   char *tmpdir)
{
    char def[513], mod[513], moddir[513], out[513], basemod[513], tmpmod[513],
	 modname[513];
    int modtypeindex, descindex;
    int index, ii, version;
    char *args[10];
    FILE *tmpf;
    struct utsname utsbuf, *utsp = &utsbuf;
    char* wherewasi = getcwd(NULL, 0);

    printf("Darwin osdep_mkinst_files unimplemented\n");
    return;

    chdir(basedir);
    
    ii = 0;


    /* Pre-scan for shortname */
    index = 0;
    short_name[0] = 0;
    while (input_data[index] != NULL) {
	if (input_data[index]->type == SHORTNAME) {
		strcpy(short_name, input_data[index+1]->string);
	}
	while (input_data[index] != NULL) index++;
		index++;
    }
    if (short_name[0] == 0)
    {
	printloc(stderr, UDI_MSG_FATAL, 1029, "unable to find SHORTNAME\n");
    }

    /* Scan for module declarations */
    index = 0;
    while (input_data[index] != NULL) {
	if (input_data[index]->type == MODULE) {
	    /*
	     * Install the loadable modules
	     */

	    strcpy(modname, input_data[index + 1]->string);
	    
	    sprintf(mod, "%s/%s.o", tmpdir, modname);

	    get_mod_type(modtype);

#ifndef KWQ_INIT_HACK
	    if (strcmp("udi_MA", modname) !=0 ) {
#else	    	    
	    if (strcmp("udi_mgmt", modname) == 0) continue;
#endif
	    sprintf(basemod, "%s/init", tmpdir);
	    strcpy(tmpmod, basemod);
	    strcat(basemod, ".c");
	    strcat(tmpmod, ".o");
	    tmpf = fopen(basemod, "w");
	    if (udi_global.verbose)
		printf("\n GENERATING GLUE for module named %s, "
		       "type: %s\n", modname, modtype);
	    if (strcmp("udi_mgmt", modname) != 0) {

		if (strcmp("udi_MA", modname) != 0) {
		    fprintf(tmpf, glueincl);
		    fprintf(tmpf, gluehead);
		    output_glue_code(tmpf, modtype, GLUE_HEADER);
		    fprintf(tmpf, gluefoot);
		    output_glue_code(tmpf, modtype, GLUE_FOOTER);
		}

	    } else {
#ifndef KWQ_INIT_HACK
    		fprintf(tmpf, glueincl);
		fprintf(tmpf, gluehead);
		fprintf(tmpf, "    return 0;}\n");
		fprintf(tmpf, gluefoot);
		fprintf(tmpf, "    return;}\n");
		sprintf(mod, "%s/udi_mgmt_sprops.o", tmpdir);
#endif
	    }
	    if (udi_global.verbose)
		printf("\n\n");
	    fclose(tmpf);
	    strcpy(def, "-DUDI_MOD_NAME=\"");
	    if (strcmp("udi_mgmt", modname))
		strcat(def, modname);
	    else
		strcat(def, "udi_mgmt_sprops");
	    strcat(def, "\"");
	    args[0] = def;
	    os_compile(basemod, tmpmod, def, 0, 0);
/*kwq: debug?	    os_copy("/tmp", "init.c"); */

	    ii = 0;
	    args[ii++] = "ld";
	    args[ii++] = "-r";
	    args[ii++] = "-o";
	    args[ii++] = mod;
	    sprintf(out, "%s/%s", basedir, modname);
	    args[ii++] = out;
	    args[ii++] = tmpmod;
	    args[ii++] = NULL;
	    os_exec(NULL, args[0], args);
#ifndef KWQ_INIT_HACK
	    } else {
	     
	       ii=0;
	      args[ii++] = "ln";
	      args[ii++] = "-s";
	      sprintf(out, "%s/%s", basedir, modname);
	      args[ii++] = out;
	      args[ii++] = mod; 
	      args[ii++] = NULL;
	      os_exec(NULL, args[0], args);
	    }
#endif	    

#ifdef UDI_ABI_ELF_UDIPROPS_SECTION
            prepare_elf_object(mod, short_name, NULL);
#endif

	    if (!udi_global.stop_install)
/* We need to do this in order to install udi_MA. */
	    {
		sprintf(basemod, "%s/%s", basedir, modname);
		sprintf(tmpmod, "%s/%s", tmpdir, modname);

		uname(utsp);
		strcpy(moddir, "/lib/modules/");
		strncat(moddir, utsp->release, _SYS_NAMELEN);
		strcat(moddir, "/misc/");
		ii = 0;

		ADDARG("/usr/bin/install");
		ADDARG(mod);
		ADDARG(moddir);

		os_exec(NULL, args[0], args);



	    }
/*	} else if (input_data[index]->type == CATEGORY) {
	    modtypeindex = input_data[index + 1]->ulong;
	} else if (input_data[index]->type == NAME) {
	    descindex = input_data[index + 1]->ulong;
	} else if (input_data[index]->type == PROPVERS) {
	    version = input_data[index + 1]->ulong;
	} else if (input_data[index]->type == REGION) {
*/
	}

	while (input_data[index] != NULL)
	    index++;
	index++;
    }

    /*
     * Now that the UDI modules have been installed as loadable modules,
     * run depmod so that the dependencies are figured out by the kernel.
     * This alleviates the hassle from us.
     */
    if (!udi_global.stop_install) {
	ii = 0;
	ADDARG("/sbin/depmod");
	ADDARG("-a");

	/* Unfortunately we can't do a redirect to /dev/nul here! :( */
	os_exec(NULL, args[0], args);
    }
    
    chdir(wherewasi);
    
    free(wherewasi);
}
