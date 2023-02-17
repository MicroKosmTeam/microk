/*
 * File: tools/common/udibuild.c
 *
 * UDI Driver Module Build Utility
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

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "udi_abi.h"
#include "y.tab.h"
#include "osdep.h"
#include "global.h"
#include "param.h"
#include "spparse.h"
#include "common_api.h"


void
get_obj_filename(char **objfile, char *moddir, char *module, char *src_c_file)
{
	char *pc;

	if (*objfile) free(*objfile);

	*objfile = optinit();
	optcpy(objfile, moddir);

	/* Get filename portion of src_c_file only.  We can scan for a '/'
	 * as a directory path element since this is the static properties
	 * specification which requires U*ix format. */
	for (pc = src_c_file + strlen(src_c_file) - 1; pc > src_c_file; pc--)
		if (*pc == '/') {
			pc++;
			break;
		}

	os_dirfile_name(objfile, pc);
	pc = *objfile + strlen(*objfile) - 1;
	*pc = 'o';
}

static
void
build_internal_headers(void)
{
	provides_line_t *pl;
	int i;
	char *pub_includes = getenv("UDI_PUB_INCLUDE");
	char pathname[1024];
	char fpathname[1024];
	int fname_start;

	if (pub_includes == NULL) 
		return;

	getcwd(fpathname, sizeof(fpathname) - 1);
	strcat(fpathname, "/");
	fname_start = strlen(fpathname);

	for (pl = udi_global.pl_list; pl; pl = pl->pl_next) {
		for (i = 0; i < pl->pl_num_includes; i++) {
			snprintf(pathname, sizeof(pathname), "%s/%s/%x.%02x",
				pub_includes,
				pl->pl_interface_name,
				pl->pl_version/256,
				pl->pl_version%256);
			os_mkdirp(pathname);
			strcat(pathname, "/");
			strcat(pathname, pl->pl_include_file[i]);
			
			/*
			 * If an OS doesn't have symlink(), this can become
	 		 * os_copy(pathname, pl->pl_include_file[i]);
			 * but since all the ones now represented do, I'd
			 * rather not build an abstraction to handle the 
			 * ones that don't.
			 */
			strcpy(fpathname + fname_start, pl->pl_include_file[i]);
			symlink(fpathname, pathname);
		}
	}
}

int
main(int argc, char **argv)
{
	int i, j, num_ccfiles, build_required;
	char *curmod;
	char *curfile, *objfile, *modfile, *moddir;
	char *ccfiles, *objfiles, *ccopts, *incpaths;
	char *symfile;
	int c;

	/* Initialize some globals and other operational variables. */

	udi_tool_init("udibuild");

	modfile = optinit();
	moddir  = optinit();

	/* Now read the udiprops file.  Do this before parsing command
           line parameters or looking for environment overrides
           because these overrides may supercede values defaulted from
           the udiprops contents. But we do peek ahead for the mapper
	   flag becuase we may need to modify parser behaviour. */

	opterr = 0;
	while ((c = getopt(argc, argv, "m")) != EOF) {
		switch (c) {
		case 'm': 
			udi_global.make_mapper = 1;
			break;
		default:
			break;
		}
	}
	opterr = 1;
	optind = 1;

	do_udiparse(NULL);

	/* Parse the command line and other environment overrides. */

	UDI_Util_Get_Params(argc, argv);

	set_binary_type();

	build_internal_headers();

	/* Do the compile(s) for each module */
	for (i = 0; i < module_cnt; i++) {

		curmod = udi_global.propdata[module[i]+1]->string;
	        objfiles = optinit();

		/* Create output directory */

		optcpy(&moddir, udi_global.outdir);
		os_subdir_name(&moddir, "bin");
		if (!os_file_exists(moddir)) {
		    os_mkdir(moddir);
		}
		os_subdir_name(&moddir, UDI_ABI_NAME);
		if (!os_file_exists(moddir)) {
		    os_mkdir(moddir);
		}

		/* Determine filepath for output file. */

		optcpy(&modfile, moddir);
		os_dirfile_name(&modfile, curmod);

		/* Get include paths specific to this module */

		incpaths = get_incdirs(curmod, 1);

		build_required = 0;
		objfile = NULL;

		/* Scan to see if we need to rebuild the target.
		 * If the udiprops.txt or any .h is newer, we
		 * rebuild everything, otherwise just rebuild
		 * newer .c files.
		 */

		if (udi_global.update && os_file_exists(modfile)) {
			ccfiles = get_ccfiles(curmod, 1);
			optcatword(&ccfiles, udi_global.propfile);
			num_ccfiles = count_args(ccfiles);
			curfile = ccfiles;

			for (j = 0; j < num_ccfiles; j++) {

				if (os_file_newer(curfile, modfile)) {
					if (curfile[strlen(curfile)-1] == 'c')
						build_required |= 1;
					else
						build_required += 2;
				}

				if (curfile[strlen(curfile)-1] == 'c') {
					get_obj_filename(&objfile, moddir,
							 curmod, curfile);
					if (os_file_exists(objfile)) {
						if (os_file_newer(curfile,
								  objfile))
							build_required |= 1;
					} else
						build_required |= 1;
				}

				/* Get to next file */
				while (*curfile != '\0') curfile++;
				curfile++;
			}
			free(ccfiles);
		} else
			build_required = 2;

		/* Now do the actual build of any and all required
                   files to obtain the module output file. */

		if (build_required) {

			ccfiles = get_ccfiles(curmod, 0);
			num_ccfiles = count_args(ccfiles);
			curfile = ccfiles;

			for (j = 0; j < num_ccfiles; j++) {

				/* Determine the target object file */

				get_obj_filename(&objfile, moddir, curmod,
						 curfile);
				/* Keep the object file to allow us to do
				 * update/incremental builds in the future.
				 * Otherwise, do the following:
				 *   optcat(&udi_global.tempfiles, " ");
				 *   optcat(&udi_global.tempfiles, objfile);
				 */
				optcat(&objfiles, " ");
				optcat(&objfiles, objfile);

				/* Determine if the object file needs 
				 * to be rebuilt. */

				if (build_required > 1 ||
				    os_file_newer(curfile, modfile) ||
				    !os_file_exists(objfile) ||
				    os_file_newer(curfile, objfile)) {

					/* Get the cc options */
					ccopts = get_ccopts(curmod, curfile);
					optcatword(&ccopts, incpaths);
					
				        /* Execute the compile */
					os_compile(curfile, objfile, ccopts,
						   udi_global.debug,
						   udi_global.optimize);
				}
					
				/* Get to next file */
				while (*curfile != '\0') curfile++;
				curfile++;
				
			}
			free(ccfiles);
		}

		if (build_required) {
		    symfile = get_symbols();
		    os_link(modfile, objfiles+1 /*skip first space*/, symfile);
		    if (*symfile) unlink(symfile);
		    free (symfile);
		} else if (udi_global.verbose)
		    printloc(stdout, UDI_MSG_INFO, 1301,
			     "    --Module %s up-to-date\n",
			     curmod);

		free(objfiles);
	}

	udi_tool_exit(0);
	return 0;
}
