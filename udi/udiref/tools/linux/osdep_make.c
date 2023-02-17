/*
 * File: tools/linux/osdep_make.c
 *
 * Linux-specific target file generation.
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
#include <sys/stat.h>
#include <sys/utsname.h>
#include "y.tab.h"
#include "spparse.h"
#include "../common/common_api.h"	/* Common utility functions */
#include "osdep_make.h"
#include "osdep.h" /* tools/linux/osdep.h */
#include <udi_sprops_ra.h>

#define UDI_VERSION 0x101
#include <udi.h>

char *pkg;
char modtype[256];
char typedesc[80];

#define input_data udi_global.propdata
#define entries_t propline_t

/*
 * On UnixWare, this function returns a module type. On Linux, we return
 * a string suitable for determining what directory the Linux kernel
 * module should be placed in.
 */
/* UDI Module type to Linux Kernel Module directory/type translation */
#define  SCSI_DIR  "scsi"
#define  NET_DIR   "net"
#define  MISC_DIR  "misc"
#define  VIDEO_DIR "video"
#define  USB_DIR   "usb"
#define  BLOCK_DIR "block"
#define  IPV4_DIR  "ipv4"

const char *SCSI = SCSI_DIR;
const char *NIC  = NET_DIR;
const char *COMM = MISC_DIR;
const char *GIO  = MISC_DIR;
const char *SND  = MISC_DIR;
const char *VID  = VIDEO_DIR;
/* UDI -> Linux types of the future... */
/*const char *BLOCK = BLOCK_DIR; IDE/disk devices */
/*const char *USB  = USB_DIR; */
/*const char *PROTOCOL = IPV4_DIR; -- or ipv6 later */

void
get_mod_type(char *modtype)
{
	int index, index2;

	index = *modtype = 0;
	while (input_data[index] != NULL) {
		if (input_data[index]->type == CHLDBOPS) {
			index2 = 0;
			while (input_data[index2] != NULL) {
				if (input_data[index2]->type == META &&
						input_data[index+1]->ulong ==
						input_data[index2+1]->ulong) {
					if (strcmp(input_data[index2+2]->string,
							"udi_scsi") == 0) {
						strcpy(modtype, SCSI);
					} else if (strcmp(input_data[index2+2]->
								string,
								"udi_nic") == 0)
								{
						if (strcmp(modtype, SCSI)
								!= 0)
							strcpy(modtype, NIC);
					} else if (strcmp(input_data[index2+2]->
								string,
								"udi_comm") ==
								0) {
						if (strcmp(modtype, SCSI)
								!= 0 && strcmp(
								modtype, NIC)
								!= 0)
							strcpy(modtype, COMM);
					} else if (strcmp(input_data[index2+2]->
								string,
								"udi_vid") ==
								0) {
						if (strcmp(modtype, SCSI)
								!= 0 && strcmp(
								modtype, NIC)
								!= 0 && strcmp(
								modtype, COMM)
								!= 0)
							strcpy(modtype, VID);
					} else if (strcmp(input_data[index2+2]->
								string,
								"udi_snd") ==
								0) {
						if (strcmp(modtype, SCSI)
								!= 0 && strcmp(
								modtype, NIC)
								!= 0 && strcmp(
								modtype, COMM)
								!= 0 && strcmp(
								modtype, VID)
								!= 0)
							strcpy(modtype, SND);
					} else if (strcmp(input_data[index2+2]->
								string,
								"udi_gio") ==
								0) {
						if (strcmp(modtype, SCSI)
								!= 0 && strcmp(
								modtype, NIC)
								!= 0 && strcmp(
								modtype, COMM)
								!= 0 && strcmp(
								modtype, VID)
								!= 0 && strcmp(
								modtype, SND)
								!= 0)
							strcpy(modtype, GIO);
					}
					break;
				}
				while (input_data[index2] != NULL) index2++;
				index2++;
			}
		}
		while (input_data[index] != NULL) index++;
		index++;
	}
}

void
get_type_desc(char *typedesc, const char *modtype, const char *defaultdesc);

void
get_type_desc(char *typedesc, const char *modtype, const char *defaultdesc)
{
	strcpy(typedesc, "UDI ");
	if (strcmp(modtype, SCSI_DIR) == 0)
		strcat(typedesc, "SCSI Host Bus Adapters");
	else if (strcmp(modtype, NET_DIR) == 0)
		strcat(typedesc, "Network Interface Cards");
	else if (strcmp(modtype, MISC_DIR) == 0)
		strcat(typedesc, "Miscellaneous");
	else if (strcmp(modtype, VIDEO_DIR) == 0)
		strcat(typedesc, "Video Cards");
	else if (strcmp(modtype, USB_DIR) == 0)
		strcat(typedesc, "USB Devices");
	else if (strcmp(modtype, BLOCK_DIR) == 0)
		strcat(typedesc, "Block Devices");
	else if (strcmp(modtype, IPV4_DIR) == 0)
		strcat(typedesc, "IPv4 protocol filter");
	else
		strcat(typedesc, defaultdesc);
}

extern int pv_count;

#define SHORTNAME_LENGTH 8 /* spec restriction. */

static void
output_glue_header(FILE * fp, char* modname)
{
	propline_t **prop_data = udi_global.propdata;
	const char *info_name = IS_META ? "udi_meta_info" : "udi_init_info";
	int index = 0, index2 = 0;
	char *ctmp = "", *meta = "";
	char name[SHORTNAME_LENGTH + 1];
    
    /* Special case: We don't need to _udi_module_load for the udi_MA module. */

    if (IS_DRIVER) {
	    /* 
	     * The following code generates code for a request_module to load
	     * appropriate mappers - based on UnixWare code to do the same thing.
	     */
	    while (prop_data[index] != NULL) {
  	        if (prop_data[index]->type == SHORTNAME) {
		    strncpy(name, prop_data[index]->string, SHORTNAME_LENGTH);
		    name[SHORTNAME_LENGTH] = 0;
		}
		/*
		 * Also do a request_module for each meta
		 */
		if (prop_data[index]->type == META) {
			fprintf(fp, "    if (requestdeps) request_module(\"%s\");\n",
				prop_data[index + 2]->string);
		}   
		while (prop_data[index] != NULL) index++;
		index++;
	    }
	    index = 0;
	    while (prop_data[index] != NULL) {
		if (prop_data[index]->type == CHLDBOPS ||
			    prop_data[index]->type == PARBOPS ) {
		    index2 = 0;
		    while (prop_data[index2] != NULL) {
		        if (prop_data[index2]->type == META &&
					prop_data[index2 + 1]->ulong ==
					prop_data[index + 1]->ulong) {
			    meta = prop_data[index2 + 2]->string;
   			    if (strcmp(meta, "udi_bridge") == 0 &&
					strcmp(name, "udiMbrdg") != 0)
				ctmp = "udiM_bridge";
		  	    if (strcmp(meta, "udi_gio") == 0 &&
					strcmp(name, "udiMgio") != 0 &&
					strcmp(name, "tstMgio") != 0 &&
					strcmp(name, "tstIgio") != 0)
				ctmp = "udiM_gio";
			    if (strcmp(meta, "udi_gio") == 0 &&
					strcmp(name, "tstIgio") == 0)
				ctmp = "tstM_gio";
			    if (strcmp(meta, "udi_scsi") == 0 &&
					strcmp(name, "udiMscsi") != 0 &&
	        		        strcmp(name, "tstIgio") != 0)
				ctmp = "udiM_scsi";
			    if (strcmp(meta, "udi_nic") == 0 &&
					strcmp(name, "udiMnic") != 0 &&
					strcmp(name, "tstIgio") != 0)
				ctmp = "udiM_nic";
		  	    if (*meta == '%')
				ctmp = meta+1;
			    fprintf(fp, "    if (requestdeps) request_module(\"%s\");\n", ctmp);
			}
			while (prop_data[index2] != NULL) index2++;
			    index2++;
		    }
		}
		while (prop_data[index] != NULL) index++;
		index++;
	    }
    }

    fprintf(fp, "\t{\n"
		"\t\textern char *_udi_sprops_scn_for_%s;\n", modname);
    fprintf(fp, "\t\textern char *_udi_sprops_scn_end_for_%s;\n", modname);
    if (IS_MAPPER)
	fprintf(fp, "\t\textern void mapper_init();\n");
    fprintf(fp, "\t\textern void *%s;\n", info_name);
    fprintf(fp, "\t\tint sprops_size = (int)((void *)&_udi_sprops_scn_end_for_%s"
		" - (void *)&_udi_sprops_scn_for_%s);\n", modname, modname);
    fprintf(fp, "\t\tmy_module_handle = _udi_module_load(&%s,"
		" (char *)&_udi_sprops_scn_for_%s, sprops_size, &__this_module);\n\n", 
		info_name, modname);

    if (IS_MAPPER) 
	fprintf(fp, "\t\tmapper_init();\n");
		
    fprintf(fp, module_glue_head);
}

#undef input_data

void
osdep_get_kernel_headers_version(const char *kern_source_base,
	struct utsname *utsp, char *tmpdir)
{
    FILE *tmpf;
    char *nmstr; /* used by optinit/etc. */
    static char kernverc[1024], kernvertxt[1024];
    char kernel_inc[513];
    char *args[100];

    strcpy(kernel_inc, kern_source_base);
    strcat(kernel_inc, "/include");
    utsp->release[0] = 0;
    /* Do some execing on a fabricated file... */
    sprintf(kernverc, "%s/kernver", tmpdir);
    strcpy(kernvertxt, kernverc);
    strcat(kernverc, ".c");
    strcat(kernvertxt, ".txt");
    tmpf = fopen(kernverc, "w");
    if (tmpf == NULL) {
	printf("Failed to open kernel version temp file\n");
	udi_tool_exit(EBADF);
    }
    fprintf(tmpf, "#include <linux/version.h>\n");
    fclose(tmpf);

    /* do this: cc -dM -E kernver.c -o kernver.txt */
    nmstr = optinit();
    optcpy(&nmstr, udi_global.cc);
    optcat(&nmstr, " -dM -E ");
    optcat(&nmstr, kernverc);
    optcat(&nmstr, " -o ");
    optcat(&nmstr, kernvertxt);
    optcat(&nmstr, " -I ");
    optcat(&nmstr, kernel_inc);
    if (!udi_global.verbose) {
	optcat(&nmstr, " >/dev/null 2>&1");
    }
    args[0] = "/bin/sh";
    args[1] = "-c";
    args[2] = nmstr;
    args[3] = NULL;
    os_exec(NULL, args[0], args);
    free(nmstr);

    /* now parse the output file for the kernel version */
    tmpf = fopen(kernvertxt, "r");
    if (tmpf == NULL) {
	printf("Failed to open kernel version output file\n");
	udi_tool_exit(EBADF);
    }
    do {
	int result;
	char token[256]; /* the longest token we'll eat */
	result = fscanf(tmpf, "%s", &token[0]); /* eat strings */
	if (result == EOF) {
	    token[0] = 0;
	    break;
	}
	if (result > 255) {
	    printf("Tokenization failed. String too long.\n");
	    udi_tool_exit(E2BIG);
	}
	if (strcmp(token, "UTS_RELEASE") == 0) {
	    int version_length;
	    /* get the following quoted string */
	    result = fscanf(tmpf, "%s", &token[0]);
	    if (result == EOF || result > 255) {
		printf("Failed to parse the version string\n");
		udi_tool_exit(E2BIG);
	    }
	    /* got it... strip leading & trailing quotes */
	    version_length = strlen(token) - 2;
	    strncpy(utsp->release, &token[1],
		(SYS_NMLN < version_length ? SYS_NMLN : version_length));
	    utsp->release[(SYS_NMLN < version_length ? SYS_NMLN : version_length)] = 0;
	    break;
	}
    } while (1);
    fclose(tmpf);
}

/*
 * This could be done by the common code just before 
 * osdep_mkinst_files is called.
 */
extern void saveinput();
void ResolveMessageFiles(int install_src);
void ResolveMessageFiles(int install_src)
{
	char *spfname, *tstr;
	int numinp;
	int i;
	extern int parsemfile;
	entries_t **inp_data = udi_global.propdata;

	/*
	 * Parse and bring in any message files
	 */
	spfname = optinit();
	input_num--;	/* Remove the last terminator */
	numinp = input_num;
	i = 0;
	while (i < numinp) {
		parsemfile = 1;
		if (inp_data[i]->type == MESGFILE) {
			entries[0].type = MSGFSTART;
			entries[0].string = "message_file_start";
			entries[1].type = STRING;
			entries[1].string = strdup(inp_data[i+1]->string);
			entindex = 2;
			saveinput();
			inp_data = udi_global.propdata;

			fclose(yyin);
			if (install_src)
				optcpy(&spfname, "../msg/");
			else
				optcpy(&spfname, "../../msg/");
			optcat(&spfname, inp_data[i+1]->string);
#if DEBUG
			printf("Reading mesgfile: %s\n", spfname);
#endif
			yyin = fopen(spfname, "r");
			if (yyin == NULL) {
				printloc(stderr, UDI_MSG_FATAL, 1202, "Cannot "
						"open %s!\n", spfname);
			}
			optcpy(&propfile, spfname);
			prop_seen = 0;
			yyparse();
			inp_data = udi_global.propdata;
		}
		while (inp_data[i] != NULL) i++;
		i++;
	}

	free(spfname);
	entindex = parsemfile = 0;
	saveinput();
	inp_data = udi_global.propdata;

	/* Loop through the messages, and completely resolve them */
	i = 0;
	tstr = optinit();
	while (inp_data[i] != NULL) {
		if (inp_data[i]->type == MESSAGE ||
				inp_data[i]->type == DISMESG) {
			tstr[0] = 0;
			parse_message (&tstr, inp_data[i+2]->string, "\n");
			if (strcmp(tstr, inp_data[i+2]->string) != 0) {
				free(inp_data[i+2]->string);
				inp_data[i+2]->string = strdup(tstr);
			}
#if 0
			else printf("External msg resolved: %s\n", tstr);
#endif
		}
		while (inp_data[i] != NULL) i++;
		i++;
	}
	free(tstr);
}

void
osdep_mkinst_files(int install_src, entries_t ** input_data, char *basedir,
		   char *tmpdir)
{
    char def[513], mod[513], moddir[513], out[513], basemod[513], tmpmod[513],
	 modname[513];
    char module_description[513];
    char module_author[513];
    char **symbols;
    char *kernel_src_env;
    char kernel_src[513];
    int modtypeindex = -1, descindex = -1;
    int index, index2, index3, ii, version, numsym=0;
    char *args[100];
    char *nmstr; /* used by optinit/etc. */
    FILE *tmpf;
    struct utsname utsbuf, *utsp = &utsbuf;
    char running_kern_ver[SYS_NMLN];
    char* wherewasi = getcwd(NULL, 0);
    long udi_ver = -1;

    chdir(basedir);

    ResolveMessageFiles(install_src);
    /*
     * To resolve message files, we restart bison with the possibility of
     * reallocating input_data. The next assignment is needed in order to
     * pick up the possibly-realloced input_data.
     */
    input_data = udi_global.propdata;

	/* if this was a source-only package, rebuild it. */
	if (install_src) {
		args[0] = "udibuild";
		args[1] = "-o";
		args[2] = tmpdir;
		ii = 3;
		if (udi_global.debug) {
			args[ii] = "-D";
			ii++;
		} else if (udi_global.optimize) {
			args[ii] = "-O";
			ii++;
		}
		args[ii] = NULL; 
		ii++;
		os_exec(basedir, args[0], args);
	}

    /* Get the running kernel version. */
    uname(utsp);
    strncpy(running_kern_ver, utsp->release, SYS_NMLN);
    if (udi_global.verbose) {
        printf("    Current running kernel version:'%s'\n", utsp->release);
    }

    /* Allow the user to override the kernel sources in /usr/src/linux. */
    kernel_src_env = getenv("LINUX_KERNEL_SOURCE_BASE");
    if (kernel_src_env) {
        strcpy(kernel_src, kernel_src_env);
    } else {
	snprintf(kernel_src, sizeof(kernel_src), "/lib/modules/%s/build", 
		 utsp->release);
    }

    /* Get the version of the default or chosen kernel source base. */
    osdep_get_kernel_headers_version(kernel_src, utsp, tmpdir);

    /*
     * If the chosen kernel source base was invalid, attempt to choose the
     * /usr/src/linux instead.
     */
    if (kernel_src_env && utsp->release[0] == 0) {
        printf("Failed to find a valid source base at:'%s'.\n"
		"Trying /usr/src/linux.\n", kernel_src);
        strcpy(kernel_src, "/usr/src/linux");
        osdep_get_kernel_headers_version(kernel_src, utsp, tmpdir);
    }

    /* If we haven't found valid kernel sources here, we're not going to find any. */
    if (utsp->release[0] == 0) {
	printloc(stderr, UDI_MSG_FATAL, 2000, "No valid kernel sources were found.\n");
    }

    /* We have a valid kernel source base, check if it matches the running kernel version. */
    if (strncmp(running_kern_ver, utsp->release, SYS_NMLN) != 0) {
	printf("WARNING: Using kernel sources at: '%s'\n", kernel_src);
	printf("WARNING: whose kernel version is: '%s'\n", utsp->release);
	printf("WARNING: The running kernel version is: '%s'.\n", running_kern_ver);
	printf("WARNING: The installed UDI module(s) will not be available to the\n");
	printf("WARNING: current running kernel.\n");
	printf("WARNING: One way to fix this is to re-synchronize your kernel headers at\n");
	printf("WARNING: /usr/src/linux to match the current running kernel.\n\n");
    }

    if (udi_global.verbose) {
	printf("Using kernel sources at: '%s'\n", kernel_src);
	printf("whose kernel version is: '%s'\n", utsp->release);
    }

    /* If they don't exist, make sure the default is a null string. */
    strcpy(module_author, "");
    strcpy(module_description, "");

    /* Collect miscellaneous static properties info... */
    index = 0;
    while (input_data[index] != NULL) {
        if (input_data[index]->type == REQUIRES) {
            if (!strcmp(input_data[index+1]->string, "udi")) {
                udi_ver = (long)input_data[index+2]->ulong;
		if (udi_global.verbose)
                    printf("REQUIRES: UDI_VERSION 0x%lx\n", udi_ver);
            }
        } else if (input_data[index]->type == CONTACT) {
            char *name;
            modtypeindex = input_data[index + 1]->ulong;
	    name = optinit();
	    get_message(&name, modtypeindex, NULL);
            if (udi_global.verbose)
                printf("CONTACT: %s\n", name);
            free(name);
        } else if (input_data[index]->type == SUPPLIER) {
            char *name;
            modtypeindex = input_data[index + 1]->ulong;
	    name = optinit();
	    get_message(&name, modtypeindex, NULL);
            strcpy(module_author, name);
            free(name);
        } else if (input_data[index]->type == CATEGORY) {
            char *name;
            modtypeindex = input_data[index + 1]->ulong;
	    name = optinit();
	    get_message(&name, modtypeindex, NULL);
            if (udi_global.verbose)
                printf("CATEGORY: %s\n", name);
            free(name);
        } else if (input_data[index]->type == NAME) {
	    char *name;
            descindex = input_data[index + 1]->ulong;
	    name = optinit();
	    get_message(&name, descindex, NULL);
            strcpy(module_description, name);
            free(name);
        } else if (input_data[index]->type == PROPVERS) {
            version = input_data[index + 1]->ulong;
            if (udi_global.verbose)
                printf("PROPVERS: version = 0x%X\n", version);
        } else if (input_data[index]->type == REGION) {
            if (udi_global.verbose)
            	printf("REGION\n");
	} else if (input_data[index]->type == SYMBOLS) {
	    numsym++;	
	}    
        while (input_data[index] != NULL) index++;
        index++;
    }
    
    ii = 0;

    /* Process all module declarations */
    index = 0;
    while (input_data[index] != NULL) {
	if (input_data[index]->type == MODULE) {
	    /*
	     * Install the loadable modules
	     */

	    strcpy(modname, input_data[index + 1]->string);
	    
	    sprintf(mod, "%s/%s.o", tmpdir, modname);

	    get_mod_type(modtype);

            /* We can assume the NAME property's already been parsed. */
            if (descindex == -1) {
                /* But just in case, default to Miscellaneous. */
                strcpy(modtype, MISC_DIR);
            }
	    get_type_desc(typedesc, modtype, input_data[descindex]->string);
	     if (strcmp(modname, "udi") &&
	         strcmp(modname, "env_test")) {
            	/* Build glue code for everyone else. */
		    sprintf(basemod, "%s/init", tmpdir);
		    strcpy(tmpmod, basemod);
		    strcat(basemod, ".c");
		    strcat(tmpmod, ".o");
		    tmpf = fopen(basemod, "w");
		    if (udi_global.verbose)
			printf("\n GENERATING GLUE for module named %s, "
			       "type: %s for kernel dir: %s\n",
			       modname, typedesc, modtype);
		    fprintf(tmpf, glueincl);
	            /* Include UDI headers requested by this driver/mapper. */
	            fprintf(tmpf, "#define UDI_VERSION 0x%lx\n", udi_ver);
	            fprintf(tmpf, "#include <udi.h>\n\n");

		    if (module_author[0])
			fprintf(tmpf, "MODULE_AUTHOR(\"%s\");\n", module_author);
		    if (module_description[0])
			fprintf(tmpf, "MODULE_DESCRIPTION(\"%s\");\n", module_description);
		    fprintf(tmpf, gluehead);
		    output_glue_header(tmpf, modname);
		    fprintf(tmpf, gluefoot);
		    if (IS_MAPPER)
			fprintf(tmpf, "{\nextern void mapper_deinit();\n"
			      "mapper_deinit();\n}\n}\n");
		    else if (IS_DRIVER) 
			/* We're making a driver... don't export anything! */
			/* TBD: this breaks any driver that "provides" a
			 *      library interface. Is that a valid combination?
			 */
#if 0 /*!DEBUG*/
			fprintf(tmpf, "\nEXPORT_NO_SYMBOLS;\n");
#else
			fprintf(tmpf, "}\n");
#endif
		    else fprintf(tmpf,"}\n");
		    if (udi_global.verbose)
			printf("\n\n");
		    fclose(tmpf);
#define DEBUG_GLUECODE 1
#if DEBUG_GLUECODE
		    os_copy("/tmp", basemod); 
#endif
		    strcpy(def, /*UDI_CCOPTS_MAPPER*/ "-DUDI_MOD_NAME=\"");
		    strcat(def, modname);
		    strcat(def, "\"");
		    if (!udi_global.make_mapper) {
			/* Making a driver... don't versionize drivers! */
			strcat(def, " -D__NO_VERSION__ -DEXPORT_SYMTAB");
		    }
		    {
                /* Compile the glue with the flags appropriate for a mapper. */
                	int was_mapper = udi_global.make_mapper;
		        udi_global.make_mapper = 1;
		        os_compile(basemod, tmpmod, def, /*debug*/0, 
							 /*optimize*/1);
		        udi_global.make_mapper = was_mapper;
		    }
#if DEBUG_GLUECODE
		    os_copy("/tmp", tmpmod); 
#endif

/*
 * FIX ME:
 * This part here causes breakage if the user specified a path relative .udi package.
 */
		    sprintf(out, "%s/%s %s", basedir, modname, tmpmod);
	    /*
	     * TODO: this linker invocation needs to have the 3rd argument 
	     * argument filled with the export list.
	     */
        	    os_link(mod, out, NULL);
	    } else {
		snprintf(out, sizeof(out), "%s/%s", basedir, modname);
		os_copy(mod, out);
	    }

	    symbols = calloc(numsym + 1, sizeof(char *));
	    index2 = index3 = 0;
	    while (input_data[index2] != NULL) {
		   if (input_data[index2]->type == SYMBOLS) {
			symbols[index3++] = input_data[index2 + 1]->string;
		   }
		   while (input_data[index2] != NULL) index2++;
   		   index2++;
	    }	   

	    prepare_elf_object(mod, modname, (const char *)symbols);

	    free(symbols);

	    if (!udi_global.stop_install)
	    {

		/*sprintf(basemod, "%s/%s", basedir, modname);*/
		/*sprintf(tmpmod, "%s/%s", tmpdir, modname);*/
		/* TBD: use modtype instead of hardcoded 'misc' dir.
		 *     The current problem is that modtype comes out to
		 *     null for the environment.
		 */
		sprintf(moddir, "/lib/modules/%s/misc/", utsp->release);

		if (udi_global.verbose) {
		    printf("Kernel header version: '%s'\n", utsp->release);
		    printf("Kernel modules will be stored in: %s\n", moddir);
		    printf("Kernel module type is: %s\n", modtype);
		}

		/* SuSE 6.4 PowerPC ignores -c. */
		nmstr = optinit();
		optcpy(&nmstr, "(/bin/mkdir -p ");
		optcat(&nmstr, moddir);
		optcat(&nmstr, "&& /usr/bin/install -c ");
		optcat(&nmstr, mod);
		optcat(&nmstr, " ");
		optcat(&nmstr, moddir);
		optcat(&nmstr, ")");
		if (!udi_global.verbose) {
		    optcat(&nmstr, " >/dev/null 2>&1");
		}
		if (udi_global.verbose) {
			printf("Install command: %s\n", nmstr);
			fflush(stdout);
			fflush(stderr);
		}
		args[0] = "/bin/sh";
                args[1] = "-c";
                args[2] = nmstr;
                args[3] = NULL;
                os_exec(NULL, args[0], args);
                free(nmstr);
	    }

	}

	while (input_data[index] != NULL)
	    index++;
	index++;
    }

    /*
     * Now that the UDI modules have been installed as loadable modules,
     * run depmod so that the dependencies are figured out by the kernel.
     * TBD: If we're 2.3 or newer of depmod, use:
     *      depmod -k/dev/ksyms <module> 
     *   or if exists: $LINUX_KERNEL_SOURCE_BASE/System.map
     *      depmod -k$LINUX_KERNEL_HEADER_BASE/System.map <module>
     *   or if older than 2.3
     *      depmod -a
     */
    if (!udi_global.stop_install) {
	if (udi_global.verbose)
		printf("Configuring dependencies.\n");
	nmstr = optinit();
	optcpy(&nmstr, "/sbin/depmod ");
        if (udi_global.make_mapper) {
            /* let mappers bind to any kernel symbol */
	    optcat(&nmstr, "-a");
        } else {
	    /* Drivers can only bind to the UDI environment module */
	    /* Add the path to udi_MA */
	    strcpy(out, moddir);
	    strcat(out, "udi_MA.o ");
	    optcat(&nmstr, out);

	    /* Add the path to the currently generated module */
	    strcat(moddir, modname); /* put it together */
	    strcat(moddir, ".o"); /* give it a .o */
	    optcat(&nmstr, moddir);
	}
	if (!udi_global.verbose) {
		optcat(&nmstr, " >/dev/null 2>&1");
	}

	ii = 0;
	args[ii++] = "/bin/sh";
        args[ii++] = "-c";
        args[ii++] = nmstr;
        args[ii++] = NULL;
        os_exec(NULL, args[0], args);
	free(nmstr);
    }
    
    chdir(wherewasi);
    
    free(wherewasi);
}

