/*
 * File: tools/freebsd/link.c
 *
 * GNU LD-specific linker suppression to suppress globals.   Borrowed 
 * wholescale from the Linux functions of the same name since it uses
 * binutils just like we do.
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

#include "osdep.h"
#include "global.h"
#include "common_api.h"

#define MAXARG 128

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
