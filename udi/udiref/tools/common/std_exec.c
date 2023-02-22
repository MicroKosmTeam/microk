/*
 * File: tools/common/std_exec.c
 *
 * UDI Tools Standard Operation Execution Functions
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

/* 
 * tempnam isn't defined in the C89 C library, but is defined in X/Open 
 # rev 3 (XPG3) from the same year.  Additionally, we use strdup from 
 * X/Open's XPG4.2 (a.k.a. unix 95) standard.
 * 
 * Tell the environment (in the X/Open-defined way) that we want those 
 * interfaces.
 */
 
#define _XOPEN_SOURCE 1
#define _XOPEN_SOURCE_EXTENDED 1
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "osdep.h"
#include "global.h"
#include "common_api.h"

extern void os_exec(char *indir, char *efile, char *argv[]);

#ifndef OS_TEMPNAME_OVERRIDE
char *os_tempname(char *dir, char *pfx) 
{
    char *newname = tempnam(dir, pfx);

    if (newname == NULL) {
	printloc(stderr, UDI_MSG_FATAL, 1009, "unable create a temp file "
			"name %s/%s*\n", dir, pfx);
    }
    return newname;
}
#endif


#ifndef OS_DELETE_OVERRIDE
#include <unistd.h>
void os_delete(char *file) 
{
    if (unlink(file)) {
	printloc(stderr, UDI_MSG_FATAL, 1008, "unable to unlink file %s\n",
		         file);
    }
}
#endif

void os_fullrm(char *fileordir)
{
	char *args[4];

	args[0] = "/bin/rm";
	args[1] = "-rf";
	args[2] = fileordir;
	args[3] = NULL;

	os_exec(0, args[0], args);
}

#ifndef OS_RMDIR_OVERRIDE
#include <unistd.h>
void os_rmdir(char *dir) 
{
    if (rmdir(dir)) {
	printloc(stderr, UDI_MSG_FATAL, 1011, "unable to delete "
		         "directory %s\n", dir);
    }
}
#endif
    

#ifndef OS_MKDIR_OVERRIDE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
void os_mkdir(char *dirname)
{
    if (mkdir(dirname, 0755)) {
	printloc(stderr,UDI_MSG_ERROR, 1408, "Unable to create directory %s\n",
		 dirname);
	udi_tool_exit(errno);
    }
}

#if !defined MAXPATHLEN
#define MAXPATHLEN 1024
#endif

/*
 *  Like os_mkdir(), but builds a complete pathname like 'mkdir -p' 
 *  or mkdirp(3G) which, unfortunately, isn't widely available.
 */
int os_mkdirp(char *pathname)
{
	char *pc;
	char pn[MAXPATHLEN] = "/\0";
	/* Copy incoming string since strtok writes to it. */
	char *path = strdup(pathname);

	/*
	 * A slightly deceiving error message, but the reality is if we
	 * can't allocate a 1K-ish string, we're hosed anyway...
 	 */
	if (path == NULL) {
	        printloc(stderr, UDI_MSG_FATAL, 1005, "Unable to allocate "
                 "memory for a string in %s!\n",
                 "parse_filename");
	}

	for(pc = strtok(path, "/"); pc; pc = strtok(NULL, "/")) {
		strcat (pn, pc);
		strcat (pn, "/");
		if (!access(pn, 0)) {
			continue;
		}
		os_mkdir(pn);
	}
	free(path);
}
#endif


#ifndef OS_FILE_EXISTS_OVERRIDE
#include <unistd.h>
int os_file_exists(char *filename) 
{
    return (access(filename, R_OK) == 0);
}
#endif


#ifndef OS_SUBDIR_NAME_OVERRIDE
void os_subdir_name(char **rootdir, char *subdir) 
{
    optcat(rootdir, "/");
    optcat(rootdir, subdir);
}
#endif


#ifndef OS_DIRFILE_NAME_OVERRIDE
void os_dirfile_name(char **dir, char *file) 
{
    optcat(dir, "/");
    optcat(dir, file);
}
#endif


#ifndef OS_ISDIR_OVERRIDE
int os_isdir(char *filename)
{
    struct stat fstat;
    if (stat(filename, &fstat) == 0 && S_ISDIR(fstat.st_mode))
	return 1;
    return 0;
}
#endif


#ifndef OS_DIRSCAN_OVERRIDE
#include <dirent.h>
void *os_dirscan_start(char *directory)
{
    return opendir(directory);
}

char *os_dirscan_next(void *dirhandle, char *prevfile)
{
    struct dirent *direntp;
    do {
	direntp = readdir((DIR *)dirhandle);
    } while (direntp && ((strcmp(direntp->d_name, ".") == 0) ||
			 (strcmp(direntp->d_name, "..") == 0)));
    if (direntp) return direntp->d_name;
    return NULL;
}

void os_dirscan_done(void *dirhandle)
{
    closedir((DIR *)dirhandle);
}
#endif
    


#ifndef OS_EXEC_OVERRIDE
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
void os_exec(char *indir, char *efile, char *argv[]) 
{
    int status, ii;
    pid_t pid;

    if (udi_global.verbose || udi_global.no_exec) {
	    static char *prefixes[] = { "++", "--" };
	    char *pfx;
	    pfx = prefixes[(udi_global.no_exec ? 1 : 0)];
	    if (indir) fprintf(stdout, "    %scd %s\n", pfx, indir);
	    fprintf(stdout, "    %s", pfx);
	    for (ii = 0; argv[ii] != NULL; ii++)
		    fprintf(stdout, "%s ", argv[ii]);
	    fprintf(stdout, "\n");
    }
    if (udi_global.no_exec) return;

    switch ((pid = fork())) {
    case -1:
	printloc(stderr, UDI_MSG_ERROR, 1700,
		         "fork for `%s' failed (%d)\n",
		         argv[0], errno);
	udi_tool_exit(errno);
    case 0:
	if (indir)
	    if (chdir(indir)) {
		printloc(stderr, UDI_MSG_ERROR, 1701,
			         "unable to change directory %s\n", indir);
		exit(errno); /* child, therefore not udi_tool_exit */
	    }
	execvp(efile, argv);
	printloc(stderr, UDI_MSG_ERROR, 1702,
		         "exec for `%s' failed (%d)\n",
		         argv[0], errno);
	exit(errno);
    default: break;
    }
    while (wait(&status) != pid);
    if (!WIFEXITED(status) || (WIFEXITED(status) && WEXITSTATUS(status))) {
	printloc(stderr, UDI_MSG_ERROR, 1703,
		         "`%s' command failed (%d).\n",
		         argv[0], WEXITSTATUS(status));
	udi_tool_exit(WEXITSTATUS(status));
    }
}
#endif

#ifndef OS_COPY_OVERRIDE
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
void os_copy(char *dest, char *src)
{
	char *argv[7];
	int ii;

	ii = 0;
	argv[ii++] = "cp";
	if (strlen(src) == 0) {	  /* Don't issue illegal 'cp' arguments */
		return;
	}
	if (os_isdir(src)) argv[ii++] = "-r";  /* Copy input directory tree */
	argv[ii++] = src;
	argv[ii++] = dest;
	argv[ii++] = NULL;
	if (strcmp(src, dest)) {   /* Don't do "null" copies" */
		os_exec(NULL, "cp", argv);
	}
}
#endif


#ifndef OS_MKPAX_OVERRIDE
#include <unistd.h>
void os_mkpax(char *dest_pax, char *src_dir, char *pax_dir)
{
    int need_update, ii;
    char *outfile;
    char *argv[10];

    ii = 0;
    need_update = os_file_exists(dest_pax);
    if (dest_pax[0] != '/' && dest_pax[0] != '\\' && dest_pax[1] != ':')
	outfile = getcwd(NULL, 512);
    else
	outfile = optinit();
    os_dirfile_name(&outfile, dest_pax);

#ifdef OS_MKPAX_WITH_TAR
    argv[ii++] = "tar";
    argv[ii++] = (need_update ? "uf" : "cf");
    argv[ii++] = outfile;
#else
    argv[ii++] = "pax";
    argv[ii++] = "-w";
    argv[ii++] = "-x";
    argv[ii++] = "ustar";
    argv[ii++] = "-f";
    argv[ii++] = outfile;
    if (need_update) {
	argv[ii++] = "-u";
	argv[ii++] = "-a";
    }
#endif
    argv[ii++] = pax_dir;
    argv[ii++] = NULL;

    os_exec(src_dir, argv[0], argv);
    free(outfile);
}
#endif


#ifndef OS_GETPAX_OVERRIDE
#include <unistd.h>
void os_getpax(char *src_pax, char *dest_dir)
{
    int ii;
    char *argv[10];

    ii = 0;

    udi_tmpcopyinto(dest_dir, src_pax);

#ifdef OS_GETPAX_WITH_TAR
    argv[ii++] = "tar";
    argv[ii++] = "xf";
    argv[ii++] = src_pax;
#else
#ifdef OS_GETPAX_WITH_CPIO
    argv[ii++] = "cpio";
#ifdef OS_CPIO_OPTIONS
    OS_CPIO_OPTIONS;
#endif
    argv[ii++] = "-icdumI";
    argv[ii++] = src_pax;
#else
    /* Note: most OS's will define OS_GETPAX_WITH_TAR if they have an
     * actual tar utility.  This is because the udimkpkg is usually
     * used in Update mode in case multiple components are being built
     * into a single package.  The downside to this is that may
     * implementations will just append the new version to the end
     * instead of replacing existing files, thus the pax archive file
     * just grows and grows...

     * Beyond the file size drawbacks involved, the other problem is
     * that pax -r -u complains about "Newer file exists" when
     * unpacking one of these archives, and there's apparently *NO*
     * way to turn that off.  This results in an error-value exit from
     * pax and the operation (usually udisetup) fails.  Sigh. 
     */
    argv[ii++] = "pax";
    argv[ii++] = "-r";
    argv[ii++] = "-u";
    argv[ii++] = "-f";
    argv[ii++] = src_pax;
#endif
#endif
    argv[ii++] = NULL;

    os_exec(dest_dir, argv[0], argv);
}
#endif


#define MAXARG 128


#ifndef OS_COMPILE_OVERRIDE
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
    int ii = 0;
    int jj = 0;

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
#ifdef UDI_CCOPTS_MAPPER
    if (udi_global.make_mapper)
        ccoptptr = UDI_CCOPTS_MAPPER;
    else
#endif
        ccoptptr = udi_global.ccopts;

    /* Now turn the space-separated list of CC option words into
       individual arguments to exec with */
    if ((ccopts != NULL && strlen(ccopts)) || ccoptptr != NULL) {
	argv[jj = ii] = optinit();
        if (ccoptptr != NULL) {
            optcpy(&argv[ii], ccoptptr);
        }
        if (strlen(argv[ii]) > 0 && (ccopts != NULL && strlen(ccopts)))
		optcat(&argv[ii], " ");
	if (ccopts != NULL && strlen(ccopts)) optcat(&argv[ii], ccopts);
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


#ifndef OS_LINK_OVERRIDE
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
    free(argv[jj]);
}
#endif


#ifndef OS_FILE_NEWER_OVERRIDE
#include <sys/stat.h>
#include <unistd.h>
int
os_file_newer(char *first, char *second)
{
    struct stat fstat, sstat;

    /* First is allowed to not exist (it's not newer), but second must exist */
    if (stat(first, &fstat)) {
	    if (errno != ENOENT) {
		    char whereami[512];
		    printloc(stderr, UDI_MSG_ERROR, 1708,
		                     "unable to obtain file %s "
		                     "modification date (%d, %s).\n",
		                     first, errno, getcwd(whereami, 512));
		    udi_tool_exit(errno);
	    }
	    return 0;
    }
    if (stat(second, &sstat)) {
	    char whereami[512];
	    printloc(stderr, UDI_MSG_ERROR, 1708,
		             "unable to obtain file %s "
		             "modification date (%d, %s)\n",
		             second, errno, getcwd(whereami, 512));
    }
    return (fstat.st_mtime > sstat.st_mtime);
}
#endif


#ifdef OS_CODEFILE
#include OS_CODEFILE
#endif
