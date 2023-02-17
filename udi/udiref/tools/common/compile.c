 
/*
 * File: tools/common/compile.c
 *
 * UDI Tools Utility Functions for source file compilation.
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
#include "y.tab.h"
#include "global.h"


/* Obtain the source files for the compile, for the specified module */
char *
get_ccfiles(char *modname, int include_h_files) 
{
	char *ccfiles;
	int i, j, k, modfound;

	ccfiles = optinit();
	i = modfound = 0;

	while (udi_global.propdata[i] != NULL) {
		if (udi_global.propdata[i]->type == MODULE) {
			if (modfound == 0 && strcmp(udi_global.propdata[i+1]->string,
					modname) == 0) {
				modfound = 1;
			} else if (modfound == 1) {
				break;
			}
		} else if (modfound) {
			if (udi_global.propdata[i]->type == SRCFILES) {
				j = i+1;
				while (udi_global.propdata[j] != NULL) {
					k = strlen(udi_global.propdata[j]->string) - 2;
					if (k >= 0 && *(udi_global.propdata[j]->
							string+k) == '.' &&
							(include_h_files ||
							 *(udi_global.propdata[j]->string
							+k+1) == 'c')) {
						optcatword(&ccfiles,
							   udi_global.propdata[j]->string);
					}
					j++;
				}
			}
		}
		while(udi_global.propdata[i] != NULL)
			i++;
		i++;
	}
	return(ccfiles);
}

/* Obtain all the different directories used with .h specifications on
   the source_files lines to supply for automatic -I inputs to the
   compiler. */

static int
incdir_matches(char *incdir1, char *incdirline)
{
	if (strlen(incdirline) == 0) return 0;
	if (*(incdirline+1) == '-' && *(incdirline+2) == 'I')
		incdirline += 2;
	if (strlen(incdirline) == 0) return 0;
	return (!strncmp(incdirline+1, incdir1, strlen(incdir1)));
}


char *
get_incdirs(char *modname, int with_I_flag)
{
	char *incdirs, *ptr, *endptr;
	int i, j, k, modfound;
	char * pub_include_dir;
	requires_line_t *rl;

	incdirs = optinit();
	i = modfound = 0;

	while (udi_global.propdata[i] != NULL) {
	    if (udi_global.propdata[i]->type == MODULE) {
		if (modfound == 0 && strcmp(udi_global.propdata[i+1]->string,
					    modname) == 0) {
		    modfound = 1;
		} else if (modfound == 1) {
		    break;
		}
	    } else if (modfound) {
		if (udi_global.propdata[i]->type == SRCFILES) {
		    for (j = i+1; udi_global.propdata[j] != NULL; j++) {
			ptr = udi_global.propdata[j]->string;
			k = strlen(ptr) - 2;
			if (k >= 0 && *(ptr+k) == '.' && (*(ptr+k+1) == 'h')) {
			    ptr = parse_pathname(ptr);
			    if (ptr) {
				for (endptr = incdirs + strlen(incdirs);
				     endptr > incdirs;
				     endptr--)
				    if (*endptr == ' ') {
					    if (incdir_matches(ptr, endptr))
						    break;
				    }
				if (incdir_matches(ptr, endptr)) {
				    free(ptr);
				    continue;  /*already found*/
				}
				if (with_I_flag)
				    optcat(&incdirs, " -I");
				else
				    optcat(&incdirs, " ");
				optcat(&incdirs, ptr);
				free(ptr);
			    }
			}
		    }
		}
	    }
	    while(udi_global.propdata[i] != NULL)
		i++;
	    i++;
	}

	pub_include_dir = getenv("UDI_PUB_INCLUDE");
	if (pub_include_dir == NULL) {
		pub_include_dir = "/usr/include/udi";
	}

	/*
	 * For each `requires', add
	 * -I$(UDI_PUB_INCLUDE)/$(META)/$(VERSION_IN_HEX)
	 */

	for (rl=udi_global.rl_list; rl; rl=rl->rl_next) {
		char nbuf[100];
		snprintf(nbuf, sizeof(nbuf), " -I%s/%s/%x.%02x ", 
			pub_include_dir, rl->rl_meta,
			rl->rl_version/256, rl->rl_version&255);
		optcat(&incdirs, nbuf); 
	}
	
	if (strlen(incdirs)) return(incdirs+1);
	return incdirs;
}


/* Obtain the options for the compile, for the specified C file */
char *
get_ccopts(char *modname, char *file)
{
	int i, j;
	char *fileccopts;
	int srcflg, modflg, optidx;

	i = srcflg = modflg = optidx = 0;

	fileccopts = optinit();

	while (udi_global.propdata[i] != NULL) {
		if (udi_global.propdata[i]->type == MODULE) {
			if (! modflg) modflg = 1;
			if (modflg && strcmp(udi_global.propdata[i+1]->string,
					modname) == 0) {
				modflg = 2;
			} else if (modflg == 2) {
				break;
			}
		} else if (udi_global.propdata[i]->type == COMPOPTS && !srcflg) {
			if (udi_global.propdata[i+1] == NULL)
				optidx = 0;
			else
				optidx = i;
		} else if (modflg != 1 && (udi_global.propdata[i]->type == REQUIRES ||
				udi_global.propdata[i]->type == SRCREQS)) {
			/* Add the appropriate -I option */
#if 0
		    {  char tmpstr[10];
/*Note: the below adds the version # to the includes dir, but doesn't take into account the interface name */
			if (*fileccopts != 0) optcat(&fileccopts, " ");
			optcat (&fileccopts, "-I");
			optcat (&fileccopts, ccudiincdir);
			optcat (&fileccopts, "/");
			if (*(udi_global.propdata[i+1]->string) != '%') {
				optcat (&fileccopts,
					udi_global.propdata[i+1]->string);
			} else {
				optcat (&fileccopts, "internal/");
				optcat (&fileccopts,
					(udi_global.propdata[i+1]->string)+1);
			}
			optcat (&fileccopts, "/");
			sprintf(tmpstr, "%lx", (unsigned int) udi_global.propdata[i+2]->ulong/256);
			optcat (&fileccopts, tmpstr);
			optcat (&fileccopts, ".");
			sprintf(tmpstr, "%02lx", (unsigned int) udi_global.propdata[i+2]->ulong%256);
			optcat (&fileccopts, tmpstr);
		    }
#endif
		} else if (udi_global.propdata[i]->type == SRCFILES) {
			i++;
			while (udi_global.propdata[i] != NULL) {
				if (strcmp(udi_global.propdata[i]->string, file) == 0) {
					srcflg++;
					break;
				}
				i++;
			}
		}
		while(udi_global.propdata[i] != NULL) {
			i++;
		}
		i++;
	}
	/* Add the appropriate options line */
	if (optidx) {
		/* Go through module-specific options */
		j = optidx + 1;
		while (udi_global.propdata[j] != NULL) {
			optcatword(&fileccopts,
				   udi_global.propdata[j]->string);
			j++;
		}
	}
	return fileccopts;
}


