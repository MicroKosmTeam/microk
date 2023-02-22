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
void
os_link(char *exefile,
	char *objfile,
	char *expfile)
{
	char *argv[MAXARG];
	int ii = 0;
	int jj;
	char fbuf[2000];
	char fbuf2[2000];
	char tmp[L_tmpnam];
	char tmp2[L_tmpnam];
	char *r;
	int first = 1;

	tmpnam(tmp);

	argv[ii++] = udi_global.link;
	argv[ii++] = "-r";
	argv[ii++] = "-o";
	argv[ii++] = tmp;

	argv[jj = ii] = optinit();
	optcpy(&argv[ii++], objfile);

	for(r = strtok(argv[ii-1], " "); r; r = strtok(NULL, " ")) {
		if (!first) {
			argv[ii++] = r;
		}
		first = 0;
		if (ii >= MAXARG) {
			printloc(stderr, UDI_MSG_ERROR, 1707,
				 "too many link files (>= %d)\n",
				 MAXARG);
			udi_tool_exit(3);
		}
	}

	while (strlen(argv[ii - 1]) == 0)
		ii--;

	if (udi_global.debug) {
		/* 	
		 * We need to do an additional link against libcrt to pick
		 * up the dwarf abbrev tables.
		 */
		argv[ii++] = "/usr/ccs/lib/libcrt.a";
	}

	/* 
	 * If we are controlling the symbols to export (i.e.  we're a driver
	 * and want to keep only udi_init_info or we're a meta and have 
	 * specified only specific 'provides' or 'symbols' decls) control
	 * that now.
 	 */

	if (*expfile) {
		argv[ii++] = "-B";
		argv[ii] = optinit();
		optcpy(&argv[ii], "export:");
		optcat(&argv[ii++], expfile);
	} 

	argv[ii++] = NULL;
	os_exec(NULL, "idld", argv);

	/*
	 * Unfortunately, we have to retain this bit of ugliness for mappers.
	 * They typically won't fall into the (if export) test above yet they
	 * may have dwarf tables in them.   So now that we've linked and 
	 * we know what symbols in the relocatable we need to suppress, 
	 * we may have to make another linker pass to inject the -Bhide's.
 	 */
	if (udi_global.debug) {
		int needs_hidden = 0;
		FILE *bhidepipe;

		ii = 0;
		argv[ii++] = udi_global.link;
		tmpnam(tmp2);

                snprintf(fbuf, sizeof(fbuf), "nm -h -p %s | awk '/ [A-Z] __abbr_table/ {print \"-Bhide=\" $3}'",  tmp);

                bhidepipe = popen(fbuf, "r");
                while (fgets(fbuf2, sizeof(fbuf2), bhidepipe)) {
			argv[ii] = optinit();
                        fbuf2[strlen(fbuf2)-1] = '\0';
                        optcpy(&argv[ii], fbuf2);
			ii++;
                        needs_hidden = 1;
                }

		if (needs_hidden) {
			argv[ii++] = tmp;
			argv[ii++] = "-r";
			argv[ii++] = "-o";
			argv[ii++] = tmp2;
			argv[ii++] = NULL;
			os_exec(NULL, udi_global.link, argv);
			/* 
			 * let the 'mv' below use this copy instead.
			 */
			unlink(tmp);
			strcpy(tmp, tmp2);
		}
	}

	ii = 0;
	argv[ii++] = "mv";
	argv[ii++] = tmp;
	argv[ii++] = exefile;
	argv[ii++] = NULL;
	os_exec(NULL, "mv", argv);

	free(argv[jj]);
}
