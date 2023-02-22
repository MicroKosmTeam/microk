/*
 * File: tools/common/udimkpkg.c
 *
 * UDI Package Creation Utility
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


int
main(int argc, char **argv)
{
	int i, bin_fnd;
	unsigned int drvr_code_size = 0, drvr_data_size = 0;
	char *pkgdir, *propsfile;
	char *tmp, *bindir, *srcdir, *extradir, *msgdir;
	char *shortname = NULL, *abimodname;
	FILE *fp;
	int c;

	/* Initialize some globals and other operational variables. */

	udi_tool_init("udimkpkg" /*argv[0]*/);

	pkgdir = optinit();
	bindir = optinit();
	srcdir = optinit();
	extradir = optinit();
	msgdir = optinit();
	abimodname = optinit();
	propsfile = optinit();

	/* Now read the udiprops file.  Do this before parsing command
           line parameters or looking for environment overrides
           because these overrides may supercede values defaulted from
           the udiprops contents. But we do look ahead for 'mapper' flag. */

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

	/* Make sure all the required files are present. */
	/* Message, extra (readable), and required source files are
	 * checked below as we actually perform the packaging operations.
	 * The binary modules are primarily checked here because if this
	 * is a binary distribution then they may need to run udibuild 
	 * before we can go much further. */

	bin_fnd = 0;
	if (DIST_BINARY) {
	    bin_fnd = 1;
	    for(i = 0; i < module_cnt; i++) {
		optcpy(&abimodname, "bin");
		os_subdir_name(&abimodname, UDI_ABI_NAME);
		os_dirfile_name(&abimodname, udi_global.propdata
				[module[i]+1]->string);
		if (!os_file_exists(abimodname)){
		    if (DIST_BOTH) {
			bin_fnd = 0;
			break;
		    } else {
			printloc(stderr, UDI_MSG_ERROR, 1406,
				"module %s not found; "
				"try udibuild first.\n",
				abimodname);
				udi_tool_exit(2);
		    }
		}
	    }
	}

	/* If there was no binary, and we want to package both, package src */
	if (DIST_BOTH && !bin_fnd) udi_global.dist_contents = 1;

	/* If this is a binary distribution, tack on the .udiprops
	* section to the primary module binary file. This is done in
	* the ABI-specified manner after generating a "clean" version
	* of the props. */

	if (DIST_BINARY) {
		propsfile = os_tempname(udi_global.bldtmpdir, "udim.");
		/* First, create a temp file, and put the .udiprops in it */
		if ((fp = fopen(propsfile, "w")) == NULL) {
		    printloc(stderr, UDI_MSG_ERROR, 1401, "unable to "
                                     "open temp file %s!\n", propsfile);
		    udi_tool_exit(2);
		}
		optcatword(&udi_global.tempfiles, propsfile);
		i = 0;
		while (udi_global.propdata[i] != NULL &&
		       udi_global.propdata[i]->type != MSGFSTART) {
		    optcpy(&srcdir, udi_global.propdata[i]->string);
		    i++;
		    while(udi_global.propdata[i] != NULL) {
			optcatword(&srcdir, udi_global.propdata[i]->string);
			i++;
		    }
		    optcat(&srcdir, "\n");
		    fputs(srcdir, fp);
		    i++;
		}
		fclose(fp);
	}

	/* Create an appropriate temporary directory where we will
           build the distribution hierarchy prior to packaging it
           up. */

	tmp = os_tempname(udi_global.bldtmpdir, "udipk");
	optcpy(&udi_global.bldtmpdir, tmp);
	udi_mktmpdir(&udi_global.bldtmpdir, NULL);

	optcpy(&pkgdir, udi_global.bldtmpdir);
	
	optcpy(&tmp, udi_global.bldtmpdir);
	udi_mktmpdir(&tmp, VER1_DIR);
	udi_mktmpdir(&tmp, udi_global.drv_xxx);
	udi_mktmpdir(&tmp, udi_global.alt_yyy);
	udi_mktmpdir(&tmp, udi_global.comp_zzz);
	optcpy(&udi_global.bldtmpdir, tmp);

	optcpy(&bindir, tmp);
	if (DIST_BINARY) {
	    udi_mktmpdir(&bindir, "bin");
	    udi_mktmpdir(&bindir, UDI_ABI_NAME);
	}

	optcpy(&srcdir, tmp);
	udi_mktmpdir(&srcdir, "src");

	optcpy(&extradir, tmp);
	udi_mktmpdir(&extradir, "extra");

	optcpy(&msgdir, tmp);
	udi_mktmpdir(&msgdir, "msg");

	/* Do packaging for those things requested */

	if (DIST_SOURCE) udi_tmpcopyinto(srcdir, udi_global.propfile);

	i = 0;
	while (udi_global.propdata[i] != NULL) {
	    char *outdir;

	    outdir = NULL;

	    switch (udi_global.propdata[i++]->type) {

	    case MODULE:
		if (DIST_BINARY) {
		    /* Copy module file into destination bin directory,
		       removing the -<abi> suffix while doing so. */
		    abimodname = optinit();
		    optcpy(&abimodname, "bin");
		    os_subdir_name(&abimodname, UDI_ABI_NAME);
		    os_dirfile_name(&abimodname,
				    udi_global.propdata[i]->string);
		    optcpy(&tmp, bindir);
		    os_dirfile_name(&tmp, udi_global.propdata[i]->string);
		    udi_tmpcopy(tmp, abimodname);
		    if (i == module_ent+1) /* add .udiprops section */
			set_abi_sprops(propsfile, tmp);
		    if (!udi_global.make_mapper)
			abi_add_section_sizes(tmp,
					      udi_global.propdata[i]->string,
					      &drvr_code_size,
					      &drvr_data_size);
		}
		break;

	    case SRCFILES:
		if (DIST_SOURCE) outdir = srcdir;
		break;

	    case PROVIDES:
		i += 2;
		outdir = srcdir;
		break;

	    case READFILE:
		outdir = extradir;
		break;

	    case MESGFILE:
		outdir = msgdir;
		break;

	    default: break;
	    }
	    if (outdir)
		while (udi_global.propdata[i] != NULL) {
		    udi_tmpcopyinto(outdir, udi_global.propdata[i]->string);
		    i++;
		}
	    while (udi_global.propdata[i] != NULL) i++;
	    i++;
	}


	/* Verify that what we just prepared fits within ABI expectations */
	
	if (drvr_code_size > UDI_ABI_MAX_DRV_CODE) {
		printloc(stderr, UDI_MSG_WARN, 1825,
			 "combined driver code sections contain 0x%x bytes; "
			 "exceeding ABI guarantee of 0x%x bytes\n",
			 drvr_code_size, UDI_ABI_MAX_DRV_CODE);
	} else {
		if (drvr_code_size > (UDI_ABI_MAX_DRV_CODE * 
				      ABI_SIZE_WARN_THRESH)) {
			printloc(stderr, UDI_MSG_WARN, 1826,
				 "combined driver code sections "
				 "contain 0x%x bytes; "
				 "approaching ABI guarantee of 0x%x bytes\n",
				 drvr_code_size, UDI_ABI_MAX_MOD_CODE);
		}
	}
	
	if (drvr_data_size > UDI_ABI_MAX_DRV_DATA) {
		printloc(stderr, UDI_MSG_WARN, 1827,
			 "combined driver data sections contain 0x%x bytes; "
			 "exceeding ABI guarantee of 0x%x bytes\n",
			 drvr_data_size, UDI_ABI_MAX_DRV_DATA);
	} else {
		if (drvr_data_size > (UDI_ABI_MAX_DRV_DATA * 
				      ABI_SIZE_WARN_THRESH)) {
			printloc(stderr, UDI_MSG_WARN, 1828,
				 "combined driver data sections "
				 "contain 0x%x bytes; "
				 "approaching ABI guarantee of 0x%x bytes\n",
				 drvr_data_size, UDI_ABI_MAX_DRV_DATA);
		}
	}


	/* Take everything from the temporary dir, and put it in pax format */

	i = 0;
	while (udi_global.propdata[i] != NULL) {
		if (udi_global.propdata[i]->type == SHORTNAME) {
			shortname = udi_global.propdata[i+1]->string;
			break;
		}
		while (udi_global.propdata[i] != NULL) i++;
		i++;
	}
	optcpy(&tmp, udi_global.outdir);
	os_dirfile_name(&tmp, shortname);
	optcat(&tmp, ".udi");

	if (os_file_exists(tmp)) {
	    if (udi_global.overwrite)
		os_delete(tmp);
	    else if (!udi_global.update) {
		printloc(stderr, UDI_MSG_ERROR, 1405,
			 "file %s already exists\n"
			"    Please select OVERWRITE or "
			"UPDATE/COMBINE option.\n",
			tmp);
		udi_tool_exit(17);
	    }
	}

	/* Before creating the pax, remove any empty directories */
	do_rmdirs(pkgdir, 1);

	os_mkpax(tmp, pkgdir, VER1_DIR);

	udi_tool_exit(0);
	return 0; /* never executed, but keeps compilers happy */
}
