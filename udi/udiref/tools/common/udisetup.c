/*
 * File: tools/common/udisetup.c
 *
 * UDI Package Installation Utility
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
#include <ctype.h>
#include "udi_abi.h"
#include "y.tab.h"
#include "osdep.h"
#include "global.h"
#include "param.h"
#include "spparse.h"
#include "common_api.h"

#if !defined (OS_ALLOW_ARCHIVING)
#  define OS_ALLOW_ARCHIVING 1
#endif

char buf2[1024];

int output_redirected = 0;

/* antiparse; this routine adds additional backslashes to prevent the
 * C preprocessor from attempting to analyze/convert any of the escape
 * sequences that might be defined in the UDI_ATTR_STRING enumeration
 * properties or messages. Output is produced if fp is non-NULL */

void
antiparse(FILE *fp, char *orig_str)
{
    char *cp;
    int i;

    i = 0;
    for (cp = orig_str; *cp; cp++) {
	if (fp) fprintf(fp, "%c", *cp);
    buf2[i] = *cp; i++;
    if (*cp == '\\') {
        if (fp) fprintf(fp, "\\");
        buf2[i] = '\\'; i++;
    }
    }
    buf2[i] = '\0';
}


/* get_temp_bin_sprops
 *
 * Obtains a temporary sprops file for the UDI module(s) in the specified
 * binary directory (already ABI qualified).
 * Updates the input argument to point to the temporary sprops file.
 */

void
get_temp_bin_sprops(char **indir)
{
	char *bindir, *filename, *file, *rfile;
	void *dirhandle;

	bindir   = *indir;
	filename = optinit();

	optcpy(&filename, udi_global.bldtmpdir);
	rfile = os_tempname(filename, "uprops");
	optcatword(&udi_global.tempfiles, rfile);

	dirhandle = os_dirscan_start(bindir);
	while ((file = os_dirscan_next(dirhandle, file)) != NULL) {
	    if (strcmp(file, rfile) == 0) continue;
	    optcpy(&filename, bindir);
	    os_dirfile_name(&filename, file);
	    get_abi_sprops(filename, rfile);
	}
	os_dirscan_done(dirhandle);

	free(filename);
	optcpy(indir, rfile);
}


/* show_compinfo
 *
 * Called to display information about the component being installed.
 */

static void
show_compinfo(void)
{
	char *supplier,
		*contact,
		*name,
		*shortname   = "XXX",
		*release_num = "--",
		*release_str = "unreleased",
		*pbreak      = "\n                                  ", /*34 spcs*/
		*category;
	int ii;
	
	supplier = optinit();
	contact  = optinit();
	name     = optinit();
	category = optinit();
	
	optcpy(&supplier, "unknown");
	optcpy(&contact,  "none");
	optcpy(&name,     "anonymous");
	
	ii = get_prop_type(SHORTNAME, 0);
	if (ii >= 0) shortname = udi_global.propdata[ii+1]->string;
	
	ii = get_prop_type(CATEGORY, 0);
	if (ii >= 0) 
		get_message(&category, udi_global.propdata[ii+1]->ulong,pbreak);
	
	ii = get_prop_type(CONTACT, 0);
	if (ii >= 0) 
		get_message(&contact, udi_global.propdata[ii+1]->ulong, pbreak);
	
	ii = get_prop_type(SUPPLIER, 0);
	if (ii >= 0) 
		get_message(&supplier, udi_global.propdata[ii+1]->ulong,pbreak);
	
	ii = get_prop_type(NAME, 0);
	if (ii >= 0) 
		get_message(&name, udi_global.propdata[ii+1]->ulong, pbreak);
	
	ii = get_prop_type(RELEASE, 0);
	if (ii >= 0) {
		release_num = udi_global.propdata[ii+1]->string;
		release_str = udi_global.propdata[ii+2]->string;
	}
	
	printloc(stdout, UDI_MSG_INFO, 1531, "........ShortNam: %s\n",shortname);
	printloc(stdout, UDI_MSG_INFO, 1525, "........Name    : %s\n",name);
	if (category && strlen(category))
		printloc(stdout, UDI_MSG_INFO, 1526, "........Category: %s\n",
			 category);
	/* Note: if category wasn't set then this is not a meta library and is
	 * a driver instead.  The driver should be scanned for *all* child ops
	 * specifications and each of these should be translated into a category.
	 * The get_mod_type/get_type_desc in tools/uw/osdep_make.c does this for
	 * a single version; use this as the model for the full version TBD.
	 */
	printloc(stdout, UDI_MSG_INFO, 1527, "........Release : %s\n",release_num);
	printp(release_str,
	       "                                  ",
	       "                                  ", 78);
	printloc(stdout, UDI_MSG_INFO, 1528, "........Supplier: %s\n",supplier);
	printloc(stdout, UDI_MSG_INFO, 1529, "........Contact : %s\n",contact);

	for (ii = get_prop_type(DEVICE, 0);
	     ii > -1;
	     ii = get_prop_type(DEVICE, ii+1)) {
		int jj;
		propline_t **pp;
		pp = udi_global.propdata + ii;
		get_message(&supplier, pp[1]->ulong, pbreak);

		for (jj = 3; pp[jj]; jj++)
			if (strcmp(pp[jj]->string, "bus_type") == 0 &&
			    strcmp(pp[jj+2]->string, "pci") == 0) {

				/* PCI-Bus specific output info */
				optcat(&supplier, " (PCI=");
				for (jj = 3; pp[jj]; jj++)
					if (strcmp(pp[jj]->string,
						   "pci_vendor_id") == 0) {
						sprintf(contact, "%04lx",
							pp[jj+2]->ulong);
						optcat(&supplier, contact);
					}
				for (jj = 3; pp[jj]; jj++)
					if (strcmp(pp[jj]->string,
						   "pci_device_id") == 0) {
						sprintf(contact, ":%04lx",
							pp[jj+2]->ulong);
						optcat(&supplier, contact);
					}
				for (jj = 3; pp[jj]; jj++)
					if (strcmp(pp[jj]->string,
						   "pci_subsystem_vendor_id") == 0) {
						sprintf(contact, "/%04lx",
							pp[jj+2]->ulong);
						optcat(&supplier, contact);
					}
				for (jj = 3; pp[jj]; jj++)
					if (strcmp(pp[jj]->string,
						   "pci_subsystem_id") == 0) {
						sprintf(contact, ":%04lx",
							pp[jj+2]->ulong);
						optcat(&supplier, contact);
					}
				optcat(&supplier, ")");
				break;
			}

		printloc(stdout, UDI_MSG_INFO, 1534, "........Device  : %s\n",
			 supplier);
	}

	for (ii = 0; ii < module_cnt; ii++)
		printloc(stdout, UDI_MSG_INFO, 1530, "........Module  : %s\n",
                         udi_global.propdata[module[ii]+1]->string);
	
	free(supplier);
	free(contact);
	free(name);
	free(category);
}


/* show_saved
 *
 * Called to display information about all the existing installed UDI packages.
 */

static void
show_saved(void)
{
	void *vendhandl, *shorthandle, *binhandle;
	char *vendname, *venddir, *shortdir, *bindir, *subdir, *outline, tt;
	int ii;

	subdir  = optinit();
	outline = optinit();
	bindir  = optinit();
	venddir = optinit();

	vendhandl = os_dirscan_start(OSDEP_PKG_ARCHIVE);
	if (vendhandl == NULL) return;
	vendname = NULL;
	while ((vendname = os_dirscan_next(vendhandl, venddir)) != NULL) {
		optcpy(&venddir, OSDEP_PKG_ARCHIVE);
		os_subdir_name(&venddir, vendname);
		shorthandle = os_dirscan_start(venddir);
		shortdir = NULL;
		while ((shortdir = os_dirscan_next(shorthandle,
						   shortdir)) != NULL) {

			optcpy(&outline, vendname);
			if (strlen(outline) > 15) {
				outline[14] = '>';
				outline[15] = 0;
			}
			while (strlen(outline) < 15) optcat(&outline, " ");

			optcatword(&outline, shortdir);
			if (strlen(outline) > 24) {
				outline[23] = '>';
				outline[24] = 0;
			}
			while (strlen(outline) < 24) optcat(&outline, " ");
			optcat(&outline, " ");

#define CheckSaved(Dir,Name, Nullname)				\
                        optcpy(&subdir, venddir);		\
			os_subdir_name(&subdir, shortdir);	\
			os_subdir_name(&subdir, Dir);		\
			if (os_isdir(subdir))			\
				optcat(&outline, Name);		\
			else					\
				optcat(&outline, Nullname);

			CheckSaved("src", "S", "_");
			CheckSaved("msg", "M", "_");
			CheckSaved("rfiles", "R", "_");

			optcpy(&bindir, venddir);
			os_subdir_name(&bindir, shortdir);
			os_subdir_name(&bindir, "bin");
			if (os_isdir(bindir)) {
				binhandle = os_dirscan_start(bindir);
				subdir = NULL;
				while ((subdir = os_dirscan_next(
					binhandle, subdir)) != NULL) {
					optcatword(&outline, subdir);
				}
			}

			optcpy(&subdir, venddir);
			os_subdir_name(&subdir, shortdir);
			os_subdir_name(&subdir, "src");
			if (os_isdir(subdir)) {
				os_dirfile_name(&subdir, DEFAULT_INFILE);
			} else {
				optcpy(&subdir, venddir);
				os_subdir_name(&subdir, shortdir);
				os_subdir_name(&subdir, "bin");
				os_subdir_name(&subdir, UDI_ABI_NAME);
				get_temp_bin_sprops(&subdir);
			}
			do_udiparse(subdir);

			ii = get_prop_type(RELEASE, 0);
			if (ii == -1)
				optcat(&outline, "NNN");
			else {
				int jj;
				for (jj = 3 -
					     strlen(udi_global.
						    propdata[ii+1]->string);
				     jj >= 0; jj--)
					optcat(&outline, " ");
				optcat(&outline,
				       udi_global.propdata[ii+1]->string);
			}

 			optcat(&outline, " - ");

			optcpy(&bindir, "");
			tt = '(';
			for (ii = get_prop_type(REQUIRES, 0);
			     ii != -1;
			     ii = get_prop_type(REQUIRES, ii+1)) {
				char *s;
				s = udi_global.propdata[ii+1]->string;
				if (strcmp(s, "udi_bridge") == 0) continue;
				if (strcmp(s, "udi_physio") == 0) {
					tt = '[';
					continue;
				}
				if (strncmp(s, "udi_", 4) == 0) {
					s += 3;
					*s = '+';
				} else {
					if (strlen(bindir))
						optcat(&bindir, " ");
				}
				if (strlen(s) && strcmp(s, "udi"))
					optcat(&bindir, s);
			}

			ii = get_prop_type(NAME, 0);
			if (ii == -1) {
				optcpy(&subdir, "???");
			} else {
				get_message(&subdir,
					    udi_global.propdata[ii+1]->ulong,
					    NULL);
			}

			optcat(&subdir, (tt == '(' ? " (" : " ["));
			subdir[strlen(subdir)-1] = tt;
			optcat(&subdir, bindir);
			optcat(&subdir, (tt == '(' ? ")" : "]"));

			optcpy(&bindir, outline);
			for (ii = 0; ii < strlen(bindir); ii++)
				bindir[ii] = ' ';

			printp(subdir, bindir, outline, 79);
			if (udi_global.verbose)
				show_compinfo();
		}
		os_dirscan_done(shorthandle);
	}
	os_dirscan_done(vendhandl);
}


/* install_bin
 *
 * Install a binary distribution into the current system.
 */

static void
install_bin(char *bindir)
{
	char *tdir;
	
	tdir = optinit();
	optcpy(&tdir, bindir);
	udi_mktmpdir(&tdir, "osdep");
	osdep_mkinst_files(0, udi_global.propdata, bindir, tdir);
	if (output_redirected) {
		void *dh;
		char *cf, *cs;
		dh = os_dirscan_start(tdir);
		cf = NULL;
		cs = optinit();
		while ((cf = os_dirscan_next(dh, cf)) != NULL) {
			optcpy(&cs, tdir);
			os_subdir_name(&cs, cf);
			os_copy(udi_global.outdir, cs);
		}
		os_dirscan_done(dh);
		free(cs);
		printf("Setup output written to: %s\n", udi_global.outdir);
	}
}


/* install_comp
 *
 * Install a specific UDI component from the named comp_zzz directory.
 */

static void
install_comp(char *compdir)
{
    char *file, *subdir, *bindir, *srcdir, *vendor, *savedir;
    int ii;

    file     = NULL;
    subdir   = optinit();
    bindir   = optinit();
    srcdir   = optinit();
    vendor   = optinit();
    savedir  = NULL;

    optcpy(&bindir, compdir);
    os_subdir_name(&bindir, "bin");
    if (os_isdir(bindir))
	os_subdir_name(&bindir, UDI_ABI_NAME);
    if (!os_isdir(bindir)) {
	*bindir = 0;
    }

    optcpy(&srcdir, compdir);
    os_subdir_name(&srcdir, "src");
    if (!os_isdir(srcdir)) {
	free(srcdir);
	srcdir = NULL;
    }

    /* First get the static properties for this component. */

    if (*bindir) {
	    optcpy(&subdir, bindir);
	    get_temp_bin_sprops(&subdir);
    } else if (srcdir) {
	    optcpy(&subdir, srcdir);
	    os_dirfile_name(&subdir, DEFAULT_INFILE);
    } else {
	    printloc(stderr, UDI_MSG_FATAL, 1514, "No bin or src "
		     "directory in %s\n", compdir);
    }
    do_udiparse(subdir);
    
    /* List info of driver shortname */
    if (udi_global.disp_short) {
	ii = get_prop_type(SHORTNAME, 0);
	printf("%s\n", udi_global.propdata[ii+1]->string);
	if (subdir)  free(subdir);
	if (srcdir)  free(srcdir);
	if (vendor)  free(vendor);
	if (savedir) free(savedir);
	return;
    }

    /* Identification of the component and the local system archive location */

    if (!udi_global.quiet) show_compinfo();

    /* Now obtain the output archive directory.  This is done by
       placing all the files in the vendor/shortname location under
       OSDEP_PKG_ARCHIVE.  The vendor string should have whitespace
       and non-alphanumeric or underscore characters removed. */

    if (OS_ALLOW_ARCHIVING && !output_redirected) {
	    savedir  = optinit();
	    optcpy(&savedir, OSDEP_PKG_ARCHIVE);
	    if (!os_isdir(savedir)) os_mkdir(savedir);
    }

    ii = get_prop_type(SUPPLIER, 0);
    if (ii == -1)
	    printloc(stderr, UDI_MSG_FATAL, 1532, "no supplier id!\n");
    get_message(&vendor, udi_global.propdata[ii+1]->ulong, NULL);
    for (file = vendor; *file != 0; file++) {
	    char *temp;
	    while (*file != 0 && 
		   isalpha((int)(*file)) == 0 &&
		   isdigit((int)(*file)) == 0 &&
		   *file != '_' &&
		   *file != '\n') {
		    /* Delete invalid character */
		    for (temp = file+1; *temp; temp++)
			    *(temp-1) = *temp;
		    *(temp-1) = 0;
	    }
	    if (*file == '\n') {
		    *file = 0;
		    break;
	    }
    }
    if (savedir) {
	    os_subdir_name(&savedir, vendor);
	    if (!os_isdir(savedir)) os_mkdir(savedir);
    }

    ii = get_prop_type(SHORTNAME, 0);
    if (ii == -1)
	    printloc(stderr, UDI_MSG_FATAL, 1533, "no shortname!\n");
    if (savedir) {
	    os_subdir_name(&savedir, udi_global.propdata[ii+1]->string);
	    if (!os_isdir(savedir)) os_mkdir(savedir);
    }


    /* Install directly from bin/<abi> if it exists.  If not, we look
       for src to create the proper files from, and if that doesn't
       exist... too bad. */

    if (*bindir) {
	    /* Rebuild bindir so that we can copy the bin heirarchy */
	    optcpy(&bindir, compdir);
	    os_subdir_name(&bindir, "bin");
	    if (savedir) os_copy(savedir, bindir);
	    os_subdir_name(&bindir, UDI_ABI_NAME);
	    install_bin(bindir);
    } else {
	    if (srcdir) {
		    char *argv[8];
		    ii = 0;
		    argv[ii++] = "udibuild";
		    if (udi_global.verbose) {
			    printloc(stdout, UDI_MSG_INFO, 1523, "Building "
				             "component from source...\n");
			    argv[ii++] = "-v";
		    }
		    argv[ii++] = "-T";
		    argv[ii++] = udi_global.bldtmpdir;
		    if (udi_global.debug) argv[ii++] = "-D";
		    if (udi_global.optimize) argv[ii++] = "-O";
		    argv[ii++] = NULL;
		    os_exec(srcdir, argv[0], argv);
		    optcpy(&subdir, srcdir);
		    os_subdir_name(&subdir, "bin");
		    if (!os_isdir(subdir))
			    printloc(stderr, UDI_MSG_FATAL, 1524, "src build failed");
		    if (savedir) os_copy(savedir, bindir);
		    os_subdir_name(&subdir, UDI_ABI_NAME);
		    if (!os_isdir(subdir))
			    printloc(stderr, UDI_MSG_FATAL, 1524, "src build failed");
		    install_bin(subdir);
	    } else {
		    printloc(stderr, UDI_MSG_FATAL, 1514, "No bin or src "
			     "directory in %s\n", compdir);
	    }
    }

    /* Now install the remaining portions of this component */

    if (srcdir && savedir)
	    os_copy(savedir, srcdir);

    optcpy(&subdir, compdir);
    os_subdir_name(&subdir, "msg");
    if (os_isdir(subdir) && savedir)
	    os_copy(savedir, subdir);

    optcpy(&subdir, compdir);
    os_subdir_name(&subdir, "rfiles");
    if (os_isdir(subdir) && savedir)
	    os_copy(savedir, subdir);

    free(bindir);
    if (subdir)  free(subdir);
    if (srcdir)  free(srcdir);
    if (vendor)  free(vendor);
    if (savedir) free(savedir);
}


/* install_alt
 *
 * Install all the comp_zzz components in the named alt_yyy directory.
 */

static void
install_alt(char *altdir)
{
    char *comp, *compdir;
    void *dirhandle;

    comp      = NULL;
    compdir   = optinit();
    dirhandle = os_dirscan_start(altdir);

    while ((comp = os_dirscan_next(dirhandle, comp)) != NULL) {

	optcpy(&compdir, altdir);
	os_subdir_name(&compdir, comp);

	if (os_isdir(compdir)) {
	    if (!udi_global.quiet)
		printloc(stdout, UDI_MSG_INFO, 1522,
			         "......Component : %s\n", comp);
	    install_comp(compdir);
	}

    }

    os_dirscan_done(dirhandle);
    free(compdir);
}


/* install_drv
 *
 * Install the first/only alt_yyy components in the named drv_xxx directory.
 */

void
install_drv(char *drvdir)
{
    char *alt1, *alt2;
    void *dirhandle;

    dirhandle = os_dirscan_start(drvdir);
    alt1 = os_dirscan_next(dirhandle, NULL);
    if (alt1 == NULL) {
	printloc(stderr, UDI_MSG_FATAL, 1510, "unable to change to UDI "
		         "driver directory: alt_yyy\n");
    }
    alt2 = os_dirscan_next(dirhandle, NULL);
    if (alt2 != NULL) {
	printloc(stderr, UDI_MSG_FATAL, 1516, "multiple alt_yyy "
		         "in package not implemented\n");
	/* What should really happen here is that for each alt, all
           the comp_zzz components should be checked for their
           requires definitions and those requires matched against the
           current environment's capabilities.  Then, the best alt
           should be chosen for the install_alt below. 
	   This needs some check_alt functionality and also some environment
	   input for the best choice and is therefore TBD at this point. */
    }

    alt2 = optinit();
    optcpy(&alt2, drvdir);
    os_subdir_name(&alt2, alt1);

    if (!os_isdir(alt2)) {
	printloc(stderr, UDI_MSG_FATAL, 1510, "unable to change to UDI "
		         "driver directory: %s\n", alt2);
    }

    if (!udi_global.quiet)
	printloc(stdout, UDI_MSG_INFO, 1521, "....Alternative : %s\n", alt1);

    install_alt(alt2);

    os_dirscan_done(dirhandle);
    free(alt2);
}


/* install_pkg
 *
 * Install all the drv_xxx driver in the named pkg directory.
 */

void
install_pkg(char *pkgfile)
{
    char *tpaxdir = NULL;
    char *tmppdir, *tmpfile, *drvdir;
    void *dirhandle;

    /* Determine a temporary location to unpack this package */
    
    tmppdir = os_tempname(udi_global.bldtmpdir, "udis");
    tpaxdir = optinit();
    optcpy(&tpaxdir, tmppdir);
    tmppdir = optinit();
    optcpy(&tmppdir, pkgfile);
    tmppdir[strlen(tmppdir)-4] = 0;
    udi_mktmpdir(&tpaxdir, NULL);
    udi_mktmpdir(&tpaxdir, tmppdir);
    
    os_getpax(pkgfile, tpaxdir);
    
    /* Now look through the unpacked stuff to process each
       alt/comp/drv element */
    
    optcpy(&tmppdir, tpaxdir);   
    os_subdir_name(&tmppdir, VER1_DIR);
    if (!os_isdir(tmppdir)) {
	printloc(stderr, UDI_MSG_ERROR, 1507, "unable to open UDI "
		 "distribution format package directory\n");
	udi_tool_exit(2);
    }
    
    dirhandle = os_dirscan_start(tmppdir);
    tmpfile   = NULL;
    drvdir    = optinit();

    while ((tmpfile = os_dirscan_next(dirhandle, tmpfile)) != NULL) {
		
	    optcpy(&drvdir, tmppdir);
	    os_subdir_name(&drvdir, tmpfile);

	    if (os_isdir(drvdir)) {
		if (!udi_global.quiet)
		    printloc(stdout, UDI_MSG_INFO, 1519,
			             "..Driver        : %s\n", tmpfile);
		if (udi_global.query) {
		    printloc(stdout, UDI_MSG_INFO, 1520,
			             "Install Driver %s ", tmpfile);
		    if (!query_continue()) continue;
		}
		install_drv(drvdir);

	    }
    }

    os_dirscan_done(dirhandle);
    
    free(tpaxdir);
    free(tmppdir);
    free(drvdir);
}


/**
 ** Main Function for udisetup
 **/

int
main(int argc, char **argv)
{
	void *dirhandle;
	char *pkgfile, *pkgfiles;
	char cwd[1024], origwd[1024];
	int c;

	getcwd(origwd, sizeof(origwd));

	/* Initialize some globals and other operational variables */

	udi_tool_init("udisetup");

	/* Now read the udiprops file.  Do this before parsing command
           line parameters or looking for environment overrides
           because these overrides may supercede values defaulted from
           the udiprops contents. But as a special case (becuase we may
	   need to modify the parser's behaviour) we look ahead for 
	   the -m (mapper) flag. */

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

	/*
	 * Reset getopt's error checking and index.
	 */
	opterr = 1;
	optind = 1;

	do_udiparse(NULL);

	/* Get the tool configuration parameters */
	UDI_Util_Get_Params(argc, argv);

	/* If a filename was passed in, change to the appropriate directory,
	   if necessary, and reset the file name to the file portion */
	if (*udi_global.file_install) {
		char *ptr;

		ptr = strrchr (udi_global.file_install, '/');
		if (ptr) {
			char path[1024];
			char *basename = ptr+1;
			size_t dirname_len = basename - udi_global.file_install;
			/* Reset the output file dir, if necessary */
			if (*udi_global.outdir && *udi_global.outdir != '/') {
				strcpy(path, origwd);
				strcat(path, "/");
				strcat(path, udi_global.outdir);
				optcpy(&udi_global.outdir, path);
			}
			/* Copy dirname to path, omitting separator. */
			strncpy(path, udi_global.file_install, dirname_len);
			/* Null terminate path. */
			path[dirname_len] = '\0';
			optcpy(&udi_global.file_install, basename);

			/* Change to the directory the file is in */
			chdir(path);
		}
	}

	set_binary_type();

#if 1
/* #warning kwq: should udisetup be able to be redirected to an output directory? */
        /* See if we are installing to a directory */
	if (udi_global.outdir && strcmp(udi_global.outdir, ".")) {
/* 		udi_global.outdir = os_tempname(udi_global.bldtmpdir, "udis"); */
/* 		optcpy(&udi_global.bldtmpdir, udi_global.outdir); */
		output_redirected = 1;
	}
#endif

	if (udi_global.showsaved) {
		show_saved();
		/* No install if this option specified */
		udi_tool_exit(0);
	}

	/* If no specific package files were specified on the command line,
	 * scan the current directory for all .udi or .UDI files.
	 */

	pkgfiles = optinit();
	if (udi_global.file_install && strlen(udi_global.file_install)) {
	    optcpy(&pkgfiles, udi_global.file_install);
	} else {
	    dirhandle = os_dirscan_start(".");
	    pkgfile   = NULL;
	    while ((pkgfile = os_dirscan_next(dirhandle, pkgfile)) != NULL)
		if (strlen(pkgfile) > 4 &&
		    (strcmp(pkgfile + strlen(pkgfile) - 4, ".udi") == 0 ||
		     strcmp(pkgfile + strlen(pkgfile) - 4, ".UDI") == 0))
		optcatword(&pkgfiles, pkgfile);
	    os_dirscan_done(dirhandle);
	}

	/* Now for each module file, scan it for the available drivers in
	 * the package file to install.
	 */
	getcwd(cwd, 1024);
	for (pkgfile = strtok(pkgfiles, " ");
	     pkgfile;
	     pkgfile = strtok(NULL, " ")) {

	    if (strlen(pkgfile) == 0) continue;

	    if (!udi_global.quiet)
		printloc(stdout, UDI_MSG_INFO, 1517, "Package file    : %s\n",
			         pkgfile);
	    if (udi_global.query) {
		printloc(stdout, UDI_MSG_INFO, 1518,
			         "Install from package file: %s ", pkgfile);
		if (!query_continue()) continue;
	    }
	    install_pkg(pkgfile);
	    chdir(cwd);

	}

	chdir(origwd);

	udi_tool_exit(0);
	return 0;  /* A happy compiler is a quiet compiler */
}
