/*
 * File: tools/solaris/link.c
 *
 * Solaris-specific linker and compile options
 *
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
#include <strings.h>

#include "osdep.h"
#include "global.h"
#include "common_api.h"

#define MAXARG 128
void
os_link(char *exefile,
	char *objfile,
	char *expfile)
{
	char *argv[MAXARG];
	volatile int ii = 0;
	int jj;
	char fbuf[2000];
	char fbuf2[2000];
	char *tmp, *tmp2;
	char *r, *last;
	FILE	*fpE, *fpM;
	int first;
	char *keep_map = getenv("UDI_KEEP_MAPFILE");

	tmp = tempnam(NULL, "udilk");

	argv[ii++] = udi_global.link;
	argv[ii++] = "-r";
	argv[ii++] = "-o";
	argv[ii++] = tmp;

	argv[jj = ii] = optinit();
	optcpy(&argv[ii++], objfile);

	for(r = strtok_r(argv[ii-1], " ", (char **)&last), first = 1; r; 
	    r = strtok_r(NULL, " ", (char **)&last), first = 0) {
		if (!first) {
			argv[ii++] = r;
		}
		if (ii >= MAXARG) {
			printloc(stderr, UDI_MSG_ERROR, 1707,
			 	"too many link files (>= %d)\n",
			 	MAXARG);
			udi_tool_exit(3);
		}
	}

	while (strlen(argv[ii - 1]) == 0)
		ii--;


#if 0
	if (udi_global.debug) {
		/* 	
		 * We need to do an additional link against libcrt to pick
		 * up the dwarf abbrev tables.
		 */
		argv[ii++] = "/usr/ccs/lib/libcrt.a";
	}
#endif

	/* 
	 * If we are controlling the symbols to export (i.e.  we're a driver
	 * and want to keep only udi_init_info or we're a meta and have 
	 * specified only specific 'provides' or 'symbols' decls) control
	 * that now.
	 * We can simply use the expfile to seed the Mapfile that ld -M requires
	 * Eliminate any symbols that are not in the export list
	 * For mappers (*expfile == NULL) we have to generate the loadmap 
	 * ourselves. We simply nm -gh the linked object, and then relink it
	 * using the determined loadmap
 	 */

	if (*expfile) {
		tmp2 = tempnam(NULL, "udiB");
		fpE = fopen(expfile, "r");
		if (fpE == (FILE *)NULL) {
			printloc(stderr, UDI_MSG_FATAL, 2108, 
				"Unable to open %s for reading!\n", expfile);
		}

		fpM = fopen(tmp2, "w+");
		if (fpM == (FILE *)NULL) {
			printloc(stderr, UDI_MSG_FATAL, 2103,
				"Unable to open %s for writing!\n", tmp2);
		}
		r = strrchr(exefile, '/');
		if (r == (char *)NULL) {
			r = exefile;
		} else {
			r++;
		}
		fprintf(fpM, "%s_1.1 {\n\tglobal:\n", r);
		while (fscanf(fpE, "%s", fbuf) > 0) {
			fprintf(fpM, "\t\t%s;\n", fbuf);
		}
		fprintf(fpM, "\tlocal:\n\t\t*;\n};\n");
		fclose(fpM);
		fclose(fpE);

		argv[ii++] = "-M";
		argv[ii] = optinit();
		optcpy(&argv[ii++], tmp2);
		argv[ii++] = "-B";
		argv[ii++] = "reduce";
	} 

	argv[ii++] = NULL;
	os_exec(NULL, "ld", argv);

	/*
	 * Unlink the generated mapfile
	 */
	if (*expfile && keep_map == (char *)NULL) {
		(void)unlink(tmp2);
		free(tmp2);
	}
	
	/*
	 * Generate the mapfile for the just-linked object. This allows us to
	 * make all symbols in a UDI binary global [the default behaviour].
	 * This only happens if there is no export file provided by the common
	 * code
	 */
	if (*expfile == NULL) {
		char *tmp3 = tempnam(NULL, "udiT");

		ii = 0;
		argv[ii++] = udi_global.link;
		argv[ii++] = tmp;
		argv[ii++] = "-r";
		argv[ii++] = "-o";
		argv[ii++] = tmp3;

		snprintf(fbuf, sizeof(fbuf), "nm -gh %s | grep -v UNDEF | grep -v __func__ | awk -F'|' '{ print $8 }'", tmp);

		fpE = popen(fbuf, "r");

		tmp2 = tempnam(NULL, "udiB");

		fpM = fopen(tmp2, "w+");
		if (fpM == (FILE *)NULL) {
			printloc(stderr, UDI_MSG_FATAL, 2103,
				"Unable to open %s for writing!\n", tmp2);
		}
		r = strrchr(exefile, '/');
		if (r == (char *)NULL) {
			r = exefile;
		} else {
			r++;
		}
		fprintf(fpM, "%s_1.1 {\n\tglobal:\n", r);
		while (fscanf(fpE, "%s", fbuf) > 0) {
			fprintf(fpM, "\t\t%s;\n", fbuf);
		}
		fprintf(fpM, "\tlocal:\n\t\t*;\n};\n");
		fclose(fpM);
		pclose(fpE);

		argv[ii++] = "-M";
		argv[ii] = optinit();
		optcpy(&argv[ii++], tmp2);
		argv[ii++] = "-B";
		argv[ii++] = "reduce";
		argv[ii++] = NULL;
		os_exec(NULL, udi_global.link, argv);

		/*
		 * Let the 'mv' use the new object <tmp3>
		 */
		strcpy(tmp, tmp3);
		free(tmp3);
	}

	ii = 0;
	argv[ii++] = "mv";
	argv[ii++] = tmp;
	argv[ii++] = exefile;
	argv[ii++] = NULL;
	os_exec(NULL, "mv", argv);

	free(argv[jj]);
	free(tmp);

}


/*
 * Solaris compiler hooks
 */

void os_compile(char *cfile, char *ofile, char *ccopts, int debug, int optimize)
{
    char *ccline;
    char *ccoptptr;
    char *l_ccopts;
    char *argv[MAXARG];
    int ii = 0;
    int jj = 0;
    char *r, *last;
    int first;

    /* Borrow argv[0] for temporary storage... */
    ii++;
    argv[ii++] = "-c";
    argv[ii++] = "-I.";

    argv[0] = getenv("UDI_PUB_INCLUDE");
    if (argv[0] == NULL) argv[0] = "/usr/include/udi/";

    argv[ii] = optinit();

    optcpy(&argv[ii], "-I");
    optcat(&argv[ii], argv[0]);

    /* In case the UDI_PUB_INCLUDE translated to multiple
       space-separated directories, scan and create -I arguments for
       each word. */
    for (r = strtok_r(argv[ii], " ", (char **)&last), first =1; r; 
    	 r = strtok_r(NULL, " ", (char **)&last), first = 0) {
	if (!first) {
		argv[ii] = optinit();
		optcpy(&argv[ii], "-I");
		optcat(&argv[ii], r);
	}
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
    }
    /* Adjust argv index if we've been through the loop above */
    if (!first) {
    	ii--;
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
    ccoptptr = optinit();
#ifdef UDI_CCOPTS_MAPPER
    if (udi_global.make_mapper)
	optcpy(&ccoptptr, UDI_CCOPTS_MAPPER);
    else
#endif
	if (udi_global.ccopts)
	    optcpy(&ccoptptr, udi_global.ccopts);

    /* Now turn the space-separated list of CC option words into
       individual arguments to exec with */
    if (ccoptptr != NULL && strlen(ccoptptr)) {
    	/* Turn ccoptptr into a list of arguments */
	for (r = strtok_r(ccoptptr, " ", (char **)&last), first = 1; r;
	     r = strtok_r(NULL, " ", (char **)&last), first = 0) {
		argv[ii++] = r;
		if (ii >= MAXARG) {
		    printloc(stderr, UDI_MSG_ERROR, 1706,
			     "too many compile options"
			     " (>= %d)\n", MAXARG);
		    udi_tool_exit(3);
		}
	}
    	while (strlen(argv[ii-1]) == 0) ii--;
    } 
    if (ccopts != NULL && strlen(ccopts)) {
	/* Turn ccopts into a list of arguments */
	l_ccopts = optinit();
	optcpy(&l_ccopts, ccopts);
	for (r = strtok_r(l_ccopts, " ", (char **)&last), first = 1; r;
	     r = strtok_r(NULL, " ", (char **)&last), first = 0) {
	     	argv[ii++] = r;
		if (ii >= MAXARG) {
		    printloc(stderr, UDI_MSG_ERROR, 1706,
			     "too many compile options"
			     " (>= %d)\n", MAXARG);
		    udi_tool_exit(3);
		}
	}
    	while (strlen(argv[ii-1]) == 0) ii--;
    }

    /* Finish up and perform the compilation */
    if (debug) {
	argv[ii] = optinit();
	optcpy(&argv[ii], OS_COMPILE_DEBUG_FLAG); ii++;
	for (r = strtok_r(argv[ii-1], " ", (char **)&last), first = 1; r;
	     r = strtok_r(NULL, " ", (char **)&last), first = 0) {
	     	if (!first) {
			argv[ii++] = r;
		}
	}
    }
    if (optimize) {
	argv[ii] = optinit();
	optcpy(&argv[ii], OS_COMPILE_OPTIMIZE_FLAG); ii++;
	for (r = strtok_r(argv[ii-1], " ", (char **)&last), first = 1; r;
	     r = strtok_r(NULL, " ", (char **)&last), first = 0) {
	     	if (!first) {
			argv[ii++] = r;
		}
	}
    }
    argv[ii++] = "-o";
    argv[ii++] = ofile;
    argv[ii++] = cfile;
    argv[ii++] = NULL;
    argv[0] = ccline;
    os_exec(NULL, argv[0], argv);
    free(ccline);
    /* We'll leak memory, but app cleanup shouldn't be far away... */
}
