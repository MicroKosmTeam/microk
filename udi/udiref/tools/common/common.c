/*
 * File: tools/common/common.c
 *
 * UDI Tools Common Utility Functions
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

#include <locale.h>
#include <nl_types.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include "udi_abi.h"
#include "osdep.h"
#include "y.tab.h"
#include "global.h"
#include "param.h"
#include "spparse.h"
#include "common_api.h"

/* These are used by the locale routines */
nl_catd nlcat;


char *
optinit (void) 
{
	char *str;
	if ((str = calloc(1, 1)) == NULL) {
		printloc(stderr, UDI_MSG_FATAL, 1005, "unable to allocate "
				"memory for an option string in %s!\n",
				"optinit");
	}
	*str = 0;
	return (str);
}

void
optresize (char **src, int size)
{
	if ((*src = realloc(*src, size)) == NULL) {
		printloc(stderr, UDI_MSG_FATAL, 1005, "unable to reallocate "
			         "memory for an option string in %s!\n",
			         "optresize");
	}
}

void
optcpy (char **dst, char *src) 
{
	if ((*dst = realloc(*dst, strlen(src)+1)) == NULL) {
		printloc(stderr, UDI_MSG_FATAL, 1005, "unable to allocate "
				"memory for an option string in %s!\n",
				"optcpy");
	}
	strcpy(*dst, src);
}

void
optncpy (char **dst, char *src, int num) 
{
	if ((*dst = realloc(*dst, num+1)) == NULL) {
		printloc(stderr, UDI_MSG_FATAL, 1005, "unable to allocate "
				"memory for an option string in %s!\n",
				"optncpy");
	}
	strncpy(*dst, src, num);
}

void
optcat (char **dst, char *src) 
{
        int dst_was_empty = (*dst == NULL);

	if ((*dst = realloc(*dst, ((dst_was_empty ? 0 : strlen(*dst)) +
				   strlen(src) + 1))) == NULL) {
		printloc(stderr, UDI_MSG_FATAL, 1005, "unable to allocate "
				"memory for an option string in %s!\n",
				"optcat");
	}
	if (dst_was_empty) **dst = 0;
	strcat(*dst, src);
}

void
optncat (char **dst, char *src, int num) 
{
        int dst_was_empty = (*dst == NULL);

	if ((*dst = realloc(*dst, ((dst_was_empty ? 0 : strlen(*dst)) +
				   strlen(src) + 1))) == NULL) {
		printloc(stderr, UDI_MSG_FATAL, 1005, "unable to allocate "
				"memory for an option string in %s!\n",
				"optncat");
	}
	if (dst_was_empty) **dst = 0;
	strncat(*dst, src, num);
}

char * udi_filedir;

/* udi_tool_init
 *
 * Called on tool startup to prepare the global variables for this tool.
 */
void
udi_tool_init(char *toolname)
{
    memset(&udi_global, 0, sizeof(udi_global));
    udi_global.progname  = toolname;
    udi_global.udi_abi   = UDI_ABI_NAME;
    udi_global.bldtmpdir = optinit();
    udi_global.tempfiles = optinit();
    udi_global.tempdirs  = optinit();
    udi_global.drv_xxx   = optinit();
    udi_global.alt_yyy   = optinit();
    udi_global.comp_zzz  = optinit();
    udi_global.pkg_install = optinit();
    udi_global.propfile  = optinit();

    (void)setlocale(LC_ALL, "");
    nlcat = catopen("udicmds", NL_CAT_LOCALE);

/*     fileccopts = optinit(); */
    udi_filedir = optinit();

    optcat(&udi_global.tempfiles, " ");
    optcat(&udi_global.tempdirs,  " ");

    optcpy(&udi_global.propfile, DEFAULT_INFILE);
    UDI_Util_Init_Params();
}


/* udi_tool_exit
 *
 * Called on a success or failure exit path.  Responsible for cleaning up
 * all temporary files, etc. before actually exiting.
 */

void
udi_tool_exit(int exitval)
{
    int ii;
    extern void show_usage(void);
    struct stat statbuf;

    /* Delete temporary files in LIFO order */
    for (ii = strlen(udi_global.tempfiles); ii >= 0; ii--)
	if (udi_global.tempfiles[ii] == ' ') {
	    udi_global.tempfiles[ii] = 0;  /* recursion protection */
	    if (stat(&((udi_global.tempfiles[ii+1])), &statbuf) < 0
		&& errno == ENOENT)
		continue;
	    os_delete(&(udi_global.tempfiles[ii+1]));
	    ii = strlen(udi_global.tempfiles); /* recursion protection */
	}

    /* Delete temporary directories in LIFO order */
    for (ii = strlen(udi_global.tempdirs); ii >= 0; ii--)
	if (udi_global.tempdirs[ii] == ' ') {
	    udi_global.tempdirs[ii] = 0;   /* recursion protection */
	    if (stat(&((udi_global.tempdirs[ii+1])), &statbuf) < 0
		&& errno == ENOENT)
		continue;
  	    do_rmdirs(&(udi_global.tempdirs[ii+1]), 0);
	    ii = strlen(udi_global.tempdirs); /* recursion protection */
	}

    if (exitval == 909) {
	/* special case where udiprops.txt wasn't found. Give the user a
	 * little extra information. */
	show_usage();
	exitval = 2;
    }

    if (exitval != 0)
	fprintf(stderr, catgets(nlcat, 1, 1004, "\nExiting.\n"));
    catclose(nlcat);

    exit(exitval);
}


/* parse_pathname
 *
 * Returns a newly allocated string containing the path or NULL if no
 * path portion exists.  Also validates the path specification to make
 * sure it is legal.
 */

char *
parse_pathname(char *filespec)
{
    char *cp, *rstr;

    if (*filespec == '/') {
	printloc(stderr, UDI_MSG_FATAL, 1015, "Absolute path specifications "
		"are not allowed: %s\n", filespec);
    }

    for (cp = filespec + strlen(filespec); cp >= filespec; cp--)
	if (*cp == '/') break;
    cp++;

    if (cp == filespec) return NULL;

    rstr = malloc(cp - filespec + 1);
    if (rstr == NULL) {
	printloc(stderr, UDI_MSG_FATAL, 1005, "Unable to allocate "
                         "memory for a string in %s!\n",
		         "parse_pathname");
    }
    strncpy(rstr, filespec, cp - filespec);
    *(rstr + (cp - filespec)) = 0;

    for (; cp >= filespec; cp--)
	if (strncmp(cp, "/../", 4) == 0) {
	    printloc(stderr, UDI_MSG_FATAL, 1016, "no upward relative path "
		     "specifications are allowed: %s\n", rstr);
	}

    return rstr;
}


/* parse_filename
 *
 * Returns a newly allocatd string containing the file name portion of
 * the input filespec.
 */

char *
parse_filename(char *filespec)
{
    char *cp, *rstr;

    for (cp = filespec + strlen(filespec); cp >= filespec; cp--)
	if (*cp == '/')
	    break;
    cp++;

    rstr = malloc(strlen(cp) + 1);
    if (rstr == NULL) {
        printloc(stderr, UDI_MSG_FATAL, 1005, "unable to allocate "
		 "memory for a string in %s!\n",
		 "parse_filename");
    }
    strcpy(rstr, cp);
    return rstr;
}


/* udi_mktmpdir
 *
 * Creates a temporary directory and remembers that directory for
 * cleanup on udi_tool_exit, including the optional subdirectory
 * specification.  */
void
udi_mktmpdir(char **dirname, char *subdir)
{
    if (subdir) os_subdir_name(dirname, subdir);
    os_mkdir(*dirname);
    optcatword(&udi_global.tempdirs, *dirname);
}


/* udi_tmpcopy
 *
 * Copies a file to a destination location.  The destination is a
 * temporary file that will be removed on udi_tool_exit.
 */

void
udi_tmpcopy(char *dest, char *src)
{
	os_copy(dest, src);
	optcatword(&udi_global.tempfiles, dest);
}


/* udi_tmpcopyinto
 *
 * Copies a file into a destination directory.  The destination is a
 * temporary file that will be removed on udi_tool_exit.
 */

void
udi_tmpcopyinto(char *dest, char *src)
{
	char *outfile;
	char buf[1024] = {'\0'};
	char fbuf[1024] = {'\0'} ;
	char *oname = src;
	char *pname = src;

	outfile = optinit();
	optcpy(&outfile, dest);
	os_dirfile_name(&outfile, parse_filename(src));

	/* 
	 * Look for dirs in source component, mkdir dirs in dest as
	 * necessary.
 	 */

	while (pname = strchr(pname, '/')) {
		struct stat stats;
		pname ++;
		strncpy(buf, oname, pname - oname - 1);
		buf[pname-oname] = '\0';
		snprintf(fbuf, sizeof(fbuf), "%s/%s", dest, buf);
		if (stat(fbuf, &stats) < 0) {
			os_mkdir(fbuf);
			optcatword(&udi_global.tempdirs, fbuf);
		}
	}
	if (fbuf[0]) {
		/* 
		 * Bypass udi_tmpcopy here because we know we're deep inside
		 * a tmpdir and we know it will be recursively removed later.
		 */
		os_copy(fbuf, src);
	} else {
		udi_tmpcopy(outfile, src);
	}
	return;

}



/*
 * This function takes a string and counts the number of arguments that the
 * string contains. Arguments are separated by spaces, tabs, or the null
 * terminator (which of course also indicates the end of the string itself).
 */
int
count_args(char *str)
{
	int i;

	for (i = 0; strtok (i ? NULL : str, " \t"); i++);
	return i;
}

/* Copy a file from one place to another */
void
do_copy(char *src, char *dst) 
{
    int fds, fdd;
    struct stat fstat;
    char *ptr;
    char *dest = optinit();
    char *buf[512];
    int sz;

    if ((fds = open(src, O_RDONLY)) < 0) {
	printloc(stderr, UDI_MSG_FATAL, 1012, "unable to open file %s in copy "
		         "request!\n", src);
    }
    optcat(&dest, dst);
    if ((stat(dst, &fstat) == 0 && fstat.st_mode&S_IFDIR) || 
	/* have to check the following because Windows fails S_IFDIR
	   check for ".\\".  Sigh... */
	*(dst + strlen(dst) - 1) == '\\' ||
	*(dst + strlen(dst) - 1) == '/') {
	ptr = parse_filename(src);
	os_dirfile_name(&dest, ptr);
	free(ptr);
    }
    
    if ((fdd = open(dest, O_CREAT|O_TRUNC|O_WRONLY, 0644)) < 0) {
	printloc(stderr, UDI_MSG_FATAL, 1013, "unable to create %s in copy "
		         "request!\n", dest);
    }

    if (udi_global.verbose) printf("    ++copy %s --> %s\n", src, dest);
    
    /* Now, run through the src file, and copy to dest file */
    while((sz = read(fds, buf, 512)) != 0) {
	if (write(fdd, buf, sz) != sz) {
	    printloc(stderr, UDI_MSG_FATAL, 1014, "unable to write to %s "
		             "in copy request!\n", dest);
	}
    }
    
    close(fdd);
    close(fds);
    
    free(dest);
}


/* Upon being passed in a dir, remove contents from there on down */
int
do_rmdirs(char *startdir, int empty_only) 
{
        void *dirhandle;
	char *fname = NULL, *fpath;
	int empty;

	empty = 1;
	if (!os_file_exists(startdir)) return 0;

	if (os_isdir(startdir)) {
		fpath = optinit();
		dirhandle = os_dirscan_start(startdir);
		while ((fname = os_dirscan_next(dirhandle, fname)) != NULL)
		    if (strcmp(fname, ".") && strcmp(fname, "..")) {
			
			optcpy(&fpath, startdir);
			os_subdir_name(&fpath, fname);
			empty &= do_rmdirs(fpath, empty_only);
			
		    }
		os_dirscan_done(dirhandle);
		free(fpath);
	} else {
	    if (empty_only)
		return 0;
	}

	if (empty) {
	    if (os_isdir(startdir))
	        os_rmdir(startdir);
	    else
		os_delete(startdir);
	}

	return (empty_only ? empty : 1);
}


/* Prints a localized message to the specified stream,
 * typically, stderr or stdout.
 * We also pass a flag, indicating the message type,
 * UDI_MSG_FATAL, UDI_MSG_ERROR, UDI_MSG_WARN, UDI_MSG_INFO, or UDI_MSG_CONT.
 * arg0 = output stream, arg1 = message type,
 * arg2 = message number, arg3 = default message,
 * arg4+ = args to the message.
 */
void
printloc(
	FILE *stream,
	int flags,
	int msgnum,
	const char *format,
	...)
{
	va_list argp;

	/* Print the command name */
	fprintf(stream, catgets(nlcat, 1, 1000, "%s: "), udi_global.progname);

	/* Print a prefix to the message, if requested */
	switch(flags) {
	case UDI_MSG_INFO:
		fprintf(stream, catgets(nlcat, 1, 1001, "INFO: "));
		break;
	case UDI_MSG_WARN:
		fprintf(stream, catgets(nlcat, 1, 1002, "WARNING: "));
		break;
	case UDI_MSG_ERROR:
	case UDI_MSG_FATAL:
		fprintf(stream, catgets(nlcat, 1, 1003, "ERROR: "));
		break;
	default:
		break;
	}

	/* Get the args */
	va_start(argp, format);

	/* Print the message */
	vfprintf(stream, catgets(nlcat, 1, msgnum, format), argp);

	/* Wrap it up */
	va_end(argp);
	if (flags == UDI_MSG_FATAL) {
		udi_tool_exit(1);
	}
}

/* Prints localized help messages to stderr.
 * arg0 = the message number
 * arg1 = default message
 * arg2+ = args to the message.
 */
void
printhloc(
	int msgnum,
	const char *format,
	...)
{
	va_list argp;

	/* Get the args */
	va_start(argp, format);

	/* Print the message */
	vfprintf(stderr, catgets(nlcat, 1, msgnum, format), argp);

	/* Wrap it up */
	va_end(argp);
}

/* Get localized help message.
 * arg0 = the message number
 * arg1 = default message
 */
char *
getloc(
	int msgnum,
	const char *defmessage)
{
	return (catgets(nlcat, 1, msgnum, defmessage));
}


/* query_continue
 *
 * Called to query the user y/n to continue.  Returns non-zero if the
 * operation should continue.
 */

int
query_continue()
{
    static char answer[512], *firstblack;

    answer[0] = 0;

    while (answer[0] != 'y' &&
	   answer[0] != 'Y' &&
	   answer[0] != 'n' &&
	   answer[0] != 'N') {
	printf("('y' or 'n')? ");
	if (fgets(answer, 512, stdin) == NULL) {
	    printloc(stderr, UDI_MSG_FATAL, 1015, "unable to get user input");
	}
	for (firstblack = answer; firstblack < answer + 512; firstblack++)
	    if (*firstblack != ' ' && *firstblack != '\t') {
		answer[0] = *firstblack;
		break;
	    }
    }

    if (answer[0] == 'y' || answer[0] == 'Y') return 1;
    return 0;
}


/* get_prop_type
 *
 * Called to get the next property of the indicated type, starting at
 * the specified index into the static properties data.  Returns the
 * index of the next property in the global data or -1 if not found.
 */

int
get_prop_type(int type, int index)
{
	while (udi_global.propdata[index] != NULL) {
		if (udi_global.propdata[index]->type == type)
			return (index);
		while (udi_global.propdata[index] != NULL) index++;
		index++;
	}
	return (-1);
}


/* get_message
 * 
 * Called to get the message string translation for the specified
 * message number.  Returns an updated opt string containing the
 * message translation.
 */

void
internal_get_message(char **msgstr, int msgnum, char *pbreak)
{
	int ii;

	/* Note: this function is currently implemented in a very
	   simplistic manner.  It simply scans the properties for the
	   first matching message number.  What it really needs to do is
	   (a) handle message files as well (although it will need more
	   info from the caller since it needs to find the msg
	   subdirectory in this installation package), and (b) handle
	   locale info relative to the current locale so that the proper
	   message translation is selected.  This is all TBD for now... */
	
	for (ii = get_prop_type(MESSAGE, 0);
	     ii != -1;
	     ii = get_prop_type(MESSAGE, ii+1)) {
		if (udi_global.propdata[ii+1]->ulong != msgnum) continue;
		
		/* The message has been found.  Now it needs to be decoded. */
		parse_message(msgstr, udi_global.propdata[ii+2]->string,
			      pbreak);
		return; 
	}
	optcat(msgstr, "<message not found>");
}

void
get_message(char **msgstr, int msgnum, char *pbreak)
{
	optcpy(msgstr, "");
	internal_get_message(msgstr, msgnum, (pbreak ? pbreak : "\n"));
}


/* parse_message
 *
 * Called to perform escape translation and formatting of a raw message string.
 * This routine does *not* do any format substitutions (%_).
 * The translation is appended to the input msgstr.
 */

void
parse_message(char **msgstr, char *msg, char *pbreak)
{
        int jj, kk;

	for (jj = kk = 0; jj < strlen(msg); jj++) {
	    if (msg[jj] != '\\') continue;
	    msg[jj] = 0;
	    optcat(msgstr, msg+kk);
	    msg[jj] = '\\';
	    switch (msg[jj+1]) {
	    default:
		kk = jj++;
		break;
#define _Msg_Switch(A,B) case A: msg[jj] = B; msg[jj+1] = 0; \
                                 optcat(msgstr, msg+jj); \
                                 msg[jj++] = '\\'; msg[jj] = A; \
                                 kk = jj+1; break;
	    _Msg_Switch('_', ' ');
	    _Msg_Switch('H', '#');
	    _Msg_Switch('\\', '\\');
	    case 'p':
		optcat(msgstr, pbreak);
		jj++;
		kk = jj + 1;
		break;
	    case 'm':
		internal_get_message(msgstr, strtoul(msg+jj+2, 0, 0), pbreak);
		jj++;
		while (msg[jj+1] >= '0' && msg[jj+1] <= '9') jj++;
		kk = jj+1;
		break;
	    }
	}
	optcat(msgstr, msg+kk);
}


/* printp
 *
 * Outputs the specified paragraph.  The first string is the
 * paragraph.  No format substitution is made.  The second string is
 * the indent string which is used to indent lines.  The third
 * string is the indent to be used for the first line only.
 * The last argument is a number which is the maximum right
 * margin.  The paragraph is output as one or more lines that are
 * broken at whitespace so as to not exceed the right margin.
 */

void
printp(char *pstring, char *indent, char *firstindent, int linelen)
{
    char *line, tempchar;
    int ii, jj, kk;

    line = optinit();
    optcpy(&line, firstindent);

    for (ii = jj = 0; ii < strlen(pstring); ii++) {
	if (ii - jj + strlen(line) >= linelen) {
	    kk = ii;
	    while (ii - jj + strlen(line) >= linelen ||
		   (pstring[ii] != ' ' &&
		    pstring[ii] != '\t' &&
		    ii - jj > 10)) ii--;
	    if (pstring[ii] != ' ' && pstring[ii] != '\t') ii = kk;
	    tempchar = pstring[ii];
	    pstring[ii] = 0;
	    optcat(&line, pstring + jj);
	    pstring[ii] = tempchar;
	    while (pstring[ii] == ' ' || pstring[ii] == '\t') ii++;
	    jj = ii--;
	    printf("%s\n", line);
	    optcpy(&line, indent);
	}
    }
    if (ii > jj) printf("%s%s\n", line, pstring+jj);
    free(line);
}

void
set_binary_type()
{
	int i = 0;

       /* Figure out what type of binary we are creating */
	udi_global.bintype = 0;
	if (udi_global.make_mapper == 0) {
		while (udi_global.propdata[i] != NULL) {
			if (udi_global.propdata[i]->type == PROVIDES) {
				udi_global.bintype = BIN_META;
				return;
			}
			while (udi_global.propdata[i] != NULL) i++;
			i++;
		}
		udi_global.bintype = BIN_DRIVER;
	} else {
		udi_global.bintype = BIN_MAPPER;
	}
}

char *
get_symbols ()
{
	char *symfile;
	int i, got_sym;
	FILE *fp = NULL;

	/* Get the name of the temporary file */
	symfile = os_tempname("/tmp", "udix.");

	/* Create the file, based on the driver type */
	if (IS_MAPPER) {
		optcpy(&symfile, "");
		return(symfile);
	}
	if (IS_DRIVER) {
		if ((fp = fopen(symfile, "w")) == NULL) {
			/* Failed to open up a new symbol file */
	    		printloc(stderr, UDI_MSG_FATAL, 1017, "unable to open "
				"new symbol file %s!\n", symfile);
		}
		if (getenv("DO_POSIX")) {
			fprintf(fp, "%s_init_info\n", getenv("DO_POSIX"));
		} else {
			fprintf(fp, "udi_init_info\n");
		}
		if (fp) {
			fclose(fp);
		}
		return(symfile);
	}
	if (IS_META) {
		/* Now, go through the array, looking for symbols */
		i = got_sym = 0;
		while (udi_global.propdata[i] != NULL) {
			if (udi_global.propdata[i]->type == SYMBOLS) {
				if (!got_sym) {
					fp = fopen(symfile, "w");
					if (fp == NULL) {
	    					printloc(stderr, UDI_MSG_FATAL,
							1018,
							"unable to open new "
							" symbol file %s!\n",
							symfile);
					}
				}
				got_sym = 1;
				fprintf(fp, "%s\n", udi_global.propdata[i+1]->
						string);
			}
			while (udi_global.propdata[i] != NULL) i++;
			i++;
		}
		if (got_sym) {
			if (getenv("DO_POSIX")) {
				fprintf(fp, "%s_init_info\n", 
					getenv("DO_POSIX"));
			} else {
				fprintf(fp, "udi_meta_info\n");
			}
			fclose(fp);
		} else {
			optcpy(&symfile, "");
		}
		return(symfile);
	}
	optcpy(&symfile, "");
	return(symfile);
}
