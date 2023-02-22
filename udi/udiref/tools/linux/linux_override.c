/*
 * File: tools/linux/linux_overrides.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/utsname.h> /*new*/

#include "osdep.h"
#include "global.h"
#include "common_api.h"

/*
 * OS independent functions that we want to override.
 * The os_compile segment was derived from 
 * ../common/std_exec.c -r 1.8
 */

#define MAXARG 128

#ifdef OS_COMPILE_OVERRIDE
#ifndef OS_COMPILE_OPTIMIZE_FLAG
#define OS_COMPILE_OPTIMIZE_FLAG "-O3"
#endif
#ifndef OS_COMPILE_DEBUG_FLAG
#define OS_COMPILE_DEBUG_FLAG "-g"
#endif
void os_compile(char *cfile, char *ofile, char *ccopts, int debug, int optimize)
{
    char *ccline;
    char *ccoptptr;
    char *argv[MAXARG];
    char *kernel_src_env, kernel_inc[513]; /*new*/
    int ii = 0;
    int jj = 0;
    struct utsname utsbuf, *utsp = &utsbuf; /*new*/

    /* Get current running kernel version. */ /*new*/
    uname(utsp); /*new*/

    /* Borrow argv[0] for temporary storage... */
    ii++;
    argv[ii++] = "-c";
    argv[ii++] = "-I.";

    /* Add Linux kernel includes directory. */ /*new*/
    kernel_src_env = getenv("LINUX_KERNEL_SOURCE_BASE"); /*new*/
    if (kernel_src_env != NULL) { /*new*/
        strcpy(kernel_inc, "-I"); /*new*/
        strcat(kernel_inc, kernel_src_env); /*new*/
        strcat(kernel_inc, "/include/"); /*new*/
        argv[ii++] = kernel_inc; /*new*/
    } else { /*new*/
        strcpy(kernel_inc, "-I/lib/modules/"); /*new*/
        strcat(kernel_inc, utsp->release); /*new*/
        strcat(kernel_inc, "/build/include"); /*new*/
        argv[ii++] = kernel_inc; /*new*/
    } /*new*/

    /* Pick up libelf as configured for this host.  */ /*new*/
    argv[ii++] = "-I/usr/local/include"; /*new*/
    argv[ii++] = "-I/usr/include"; /*new*/

    argv[0] = getenv("UDI_PUB_INCLUDE");
    if (argv[0] == NULL) argv[0] = "/usr/include/udi/";

    argv[ii] = optinit();

    optcpy(&argv[ii], "-I");
    optcat(&argv[ii], argv[0]);

    /* In case the UDI_PUB_INCLUDE translated to multiple
       space-separated directories, scan and create -I arguments for
       each word. */
    for (jj = 0; argv[ii][jj] != 0; jj++) {
	if (argv[ii][jj] != ' ') continue;
	if (jj == 0) {
	    jj--;
	    argv[ii]++;   /* strip leading whitespace */
	    continue;
	}
	argv[ii][jj++] = 0;
	argv[ii+1] = optinit();
	while (argv[ii][jj] == ' ') jj++;
	if (argv[ii][jj] == 0) break;
	if (argv[ii][jj] != '-') optcpy(&argv[ii+1], "-I");
	optcat(&argv[ii+1], &(argv[ii][jj]));
	if (argv[ii][1] == 'I' && !os_file_exists(&(argv[ii][2]))) {
	    printloc(stderr, UDI_MSG_ERROR, 1704,
		             "cannot find UDI includes in %s;\n"
		             "   Try setting UDI_PUB_INCLUDE "
		             "environment variable.\n",
		             &(argv[ii][2]));
	    udi_tool_exit(4);
	}
	ii++;
	if (ii >= MAXARG) {
	    printloc(stderr, UDI_MSG_ERROR, 1705,
		             "too many compile directories (>= %d)\n", MAXARG);
	    udi_tool_exit(3);
	}
	jj = 0;
    }
    if (strlen(argv[ii])) {
	if (argv[ii][1] == 'I' && !os_file_exists(&(argv[ii][2]))) {
	    printloc(stderr, UDI_MSG_ERROR, 1704,
		             "can't find UDI includes in %s;\n"
		             "   Try setting UDI_PUB_INCLUDE "
		             "environment variable (%s).\n",
		             &(argv[ii][2]),
		             (argv[0] ? argv[0] : "undefined"));
	    udi_tool_exit(4);
	}
	ii++;
    }

    /* Make sure we pick up the right compiler and set of options */
    ccline = optinit();
    optcpy(&ccline, udi_global.cc);
    if (udi_global.make_mapper)
        ccoptptr = UDI_CCOPTS_MAPPER;
    else
        ccoptptr = udi_global.ccopts;

    /* Now turn the space-separated list of CC option words into
       individual arguments to exec with */
    if (strlen(ccopts) || ccoptptr != NULL) {
	argv[jj = ii] = optinit();
        if (ccoptptr != NULL) {
            optcpy(&argv[ii], ccoptptr);
        }
        if (strlen(argv[ii]) > 0 && strlen(ccopts)) optcat(&argv[ii], " ");
	if (strlen(ccopts)) optcat(&argv[ii], ccopts);
	ii++;
	for (argv[ii] = argv[ii-1]; *(argv[ii]) != 0; argv[ii]++)
	    if (*(argv[ii]) == ' ') {
		*(argv[ii]) = 0;
		while (*(++argv[ii]) == ' ');
		if (ii >= MAXARG) {
		    printloc(stderr, UDI_MSG_ERROR, 1706,
			     "too many compile options"
			     " (>= %d)\n", MAXARG);
		    udi_tool_exit(3);
		}
		argv[ii+1] = argv[ii] - 1;
		ii++;
	    }
    }
    while (strlen(argv[ii-1]) == 0) ii--;

    /* Finish up and perform the compilation */
    if (debug) argv[ii++] = OS_COMPILE_DEBUG_FLAG;
    if (optimize) argv[ii++] = OS_COMPILE_OPTIMIZE_FLAG;
    argv[ii++] = "-o";
    argv[ii++] = ofile;
    argv[ii++] = cfile;
    argv[ii++] = NULL;
    argv[0] = ccline;
    os_exec(NULL, argv[0], argv);
    free(ccline);
    /* We'll leak memory, but app cleanup shouldn't be far away... */
}
#endif


#ifdef OS_LINK_OVERRIDE
/* In sync with -r 1.11 ../common/std_exec.c */
void os_link(char *exefile, char *objfile, char *expfile)
{
    char *argv[MAXARG];
    char *linkoptptr;
    int ii = 0;
    int jj;
    char *r;

    argv[ii++] = udi_global.link;

    /* Now turn the space-separated list of LD option words into
       individual arguments to exec with */
    linkoptptr = udi_global.linkopts;
    if (strlen(linkoptptr)) {
	argv[jj = ii] = optinit();
        optcpy(&argv[ii], linkoptptr);

        for(r = strtok(argv[ii], " "); r; r = strtok(NULL, " ")) {
                argv[ii++] = r;
                if (ii >= MAXARG) {
                        printloc(stderr, UDI_MSG_ERROR, 1707,
                                 "too many link files (>= %d)\n",
                                 MAXARG);
                        udi_tool_exit(3);
                }
        }
    }

    while (strlen(argv[ii-1]) == 0) ii--;
/*    argv[ii++] = "-r"; */
    argv[ii++] = "-o";
    argv[ii++] = exefile;
    argv[jj=ii] = optinit();
    optcpy(&argv[ii++], objfile);

    for (argv[ii] = argv[ii-1]; *(argv[ii]) != 0; argv[ii]++)
	if (*(argv[ii]) == ' ') {
	    *(argv[ii]) = 0;
	     while (*(++argv[ii]) == ' ');
             if (ii >= MAXARG) {
		printloc(stderr, UDI_MSG_ERROR, 1707,
			         "too many link files (>= %d)\n", MAXARG);
		udi_tool_exit(3);
	    }
	    argv[ii+1] = argv[ii] - 1;
	    ii++;
	}

    while (strlen(argv[ii-1]) == 0) ii--;
    argv[ii++] = NULL;
    os_exec(NULL, "ld", argv);

    /*
     * This allows us to build UDI compliant modules because older versions
     * of gcc (2.95.3 & younger) on IA32 do not fully implement the
     * -ffreestanding -fno-builtin flags.  GCC 3.0 snapshots get this
     * right, so this should be fixed post-3.0.
     */
    if (1) {
	char cmdstr[4096];
	/*
	 * TBD: What really should happen here, is we should check the "import"
	 * list based upon the udiprops.txt's requires lines. The tools would
	 * need a pretty big rewrite to handle this.
	 *
	 * The approach we will use instead is:
	 * if (sym.name == memset && sym.import == true)
	 *    objcopy --redefine-sym=memset=udi_memset <obj>.o
	 */
	sprintf(cmdstr, /* snprintf: sizeof(cmdstr), */
		/* get undefined objfile symbols. */
		"nm --undefined-only \"%s\" "
		, /* start of sprintf arguments */
		exefile); /* arg to nm */
	/* Work with the imported symbol list... */
	{
		int work_to_do = 0;
		FILE *bhidepipe;
		char fbuf2[2000];
		char *objcopy;

		objcopy = getenv("OBJCOPY");

		if ((objcopy == NULL) || (objcopy[0] == 0)) {
			objcopy = "objcopy";
		}

		ii = 0;
		argv[ii++] = objcopy;
		argv[ii++] = exefile;

		bhidepipe = popen(cmdstr, "r");
#if SHOW_IMPORTED_SYMS
		printf("This object file imports:\n");
#endif
		while (fgets(fbuf2, sizeof(fbuf2), bhidepipe)) {
#if SHOW_IMPORTED_SYMS
			printf("%s", fbuf2);
#endif
			if (strcmp(fbuf2, "memset\n") != 0) {
				/* Not found, continue. */
				continue;
			}
			argv[ii] = optinit();
			fbuf2[strlen(fbuf2)-1] = '\0';
			optcpy(&argv[ii], "--redefine-sym=memset=udi_memset");
			ii++;
			work_to_do = 1;
		}

		if (work_to_do) {
			argv[ii++] = NULL;
			os_exec(NULL, objcopy, argv);
		}
	}
    }

    /* If you need more debugging symbols, disable this code block. */
    if (1) {
	/*
	 * If we are controlling the symbols to export (i.e.  we're a driver
	 * and want to keep only udi_init_info or we're a meta and have
	 * specified only specific 'provides' or 'symbols' decls) control
	 * that now.
	 */
	if ((expfile != NULL) && (*expfile != 0)) {
		char cmdstr[4096];

#ifdef DEBUG
		printf("Exporting these symbols:\n");
		sprintf(cmdstr, "cat %s", expfile);
		system(cmdstr);
#endif

		/*
		 * Build a complex shell command....
		 * Make all globals static except for explicit exports.
		 */
		sprintf(cmdstr, /* snprintf: sizeof(cmdstr), */
		"( " /* Build the command arguments */
			"( "
				"nm \"%s\" | "	/* get objfile symbols. */
				"cut -c9- | "	/* make parsing easier. */
				"awk '/ [A-TV-Z] / {print $2}' "
			") " 		/* print out all non U-type symbols */
			"&& cat \"%s\" "	/* add in exported symbols */
		") | sort | uniq -u"	/* remove duplicate symbols (exports) */
			, /* start of sprintf arguments */
			exefile, /* arg to nm */
			expfile  /* arg to cat */);	
		/*
		 * The resultant command is objcopy -L sym1 -L sym2... for
		 * all symbols that are global that should be made static.
		 */
		{
			int work_to_do = 0;
			FILE *bhidepipe;
			char fbuf2[2000];
			char *objcopy;

			objcopy = getenv("OBJCOPY");

			if ((objcopy == NULL) || (objcopy[0] == 0)) {
				objcopy = "objcopy";
			}

			ii = 0;
			argv[ii++] = objcopy;
			argv[ii++] = exefile;

			bhidepipe = popen(cmdstr, "r");
			while (fgets(fbuf2, sizeof(fbuf2), bhidepipe)) {
				argv[ii] = optinit();
				fbuf2[strlen(fbuf2)-1] = '\0';
				optcpy(&argv[ii], "-L");
				optcat(&argv[ii], fbuf2);
				ii++;
				work_to_do = 1;
			}

			if (work_to_do) {
				argv[ii++] = NULL;
				os_exec(NULL, objcopy, argv);
			}
		}
	}
    }

    free(argv[jj]);
}
#endif
