/*
 * File: tools/common/global.h
 * 
 * UDI Utilities Global Definitions
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


#ifndef _udi_global_h
#define _udi_global_h


/*
 * Global Variables common to all tools
 *
 * Must all be contained in the "udi_global" struct.
 *
 * Note: String parameters not stored inline below must be initialized
 * with optinit; optarg will be used to write the values.  Strings
 * which come from or relate to udiprops parameters are necessarily
 * shorter than the maximum 512 byte udiprops line length.
 */

typedef struct {
    int type;
    char *string;
    unsigned long ulong;
} propline_t;

/*
 * A descriptor of all the 'requires' data.
 */
typedef struct requires_line {
	char * rl_meta;
	unsigned long rl_version;
	struct requires_line *rl_next;
} requires_line_t;

/*
 * A descriptor of each 'provides' line.
 */
typedef struct provides_line {
	struct provides_line *pl_next;
	char * pl_interface_name;
	unsigned long pl_version;
	unsigned long pl_num_includes;
	/* 
	 * A null terminated variable length array.
	 */
	char *pl_include_file[1];
} provides_line_t;

/*
 * What kind of binary are we generating?
 */
typedef enum {
	BIN_INVALID,
	BIN_DRIVER,
	BIN_MAPPER,
	BIN_META
} bin_type_t;

struct {
    /* Common to all tools */
    char *progname;        /* Name of this program file */
    char *propfile;        /* Name of properties input file */
    propline_t **propdata; /* Data read from properties input file */
    char *udi_abi;         /* Name of the ABI */
    char *bldtmpdir;       /* Temporary directory for udi utilities to use */
    char *tempfiles;       /* Space separated list of temporary files */
    char *tempdirs;        /* Space separated list of temporary directories */
    char  *outdir;         /* The directory where files will end up (def: .) */
    int   show_usage;      /* Set to display usage information */
    int   verbose;         /* Be verbose about our actions? */
    int   no_exec;         /* Dryrun, don't actually exec operations. */
    bin_type_t bintype;	   /* Binary type (META, MAPPER, or DRIVER) */
    requires_line_t *rl_list; /* start of linked list of `requires' data. */
    requires_line_t *rl_tail; /* the last valid member on that list. */
    provides_line_t *pl_list; /* start of linked list of `provides' data. */
    provides_line_t *pl_tail; /* the last valid member on that list. */
	
    /* udibuild specifics */
    int   debug;           /* Non-zero to build debug objects (e.g. cc -g) */
    int   optimize;        /* Non-zero to build optimized objectgs (cc -O) */
    char  *cc;             /* The compiler to be used */
    char  *ccopts;         /* The compiler options to be used */
    char  *link;             /* The linker to be used */
    char  *linkopts;         /* The linker options to be used */
    /*below->   update;*/  /* Non-zero to build only when sources changed */
    /* udimkpkg specifics */
    int   dist_contents;   /* Used for DIST_SOURCE & DIST_BINARY tests */
    char  *drv_xxx;        /* Driver name being packaged (def: shortname) */
    char  *alt_yyy;        /* Alternate being packaged (def: release) */
    char  *comp_zzz;       /* Component being packaged (def: module) */
    int   overwrite;       /* Overwrite existing output package file? */
    int   update;          /* Update/combine with any existing package file? */
    /* udisetup specifics */
    int   stop_install;    /* Stop udisetup, just prior to install */
    int   query;           /* Confirm y/n w/user for each package/driver */
    int   quiet;           /* Quiet mode; minimal output on success */
    int   udipkg_dir;      /* Install a UDI directory package */
    char  *arch;           /* The specific architecture being installed on */
    char  *proc;           /* The specific processor being installed on */
    char  *pkg_install;    /* The OS Specific package name for this driver */
    char  *filedir;        /* The directory to install files to */
    char  *file_install;   /* The specific UDI packaged file to install */
    int   make_mapper;     /* Set by -m opt of udisetup to install a mapper */
    int   showsaved;       /* Show currently setup modules only */
    char  *disp_short;     /* Display info on the driver shortname */
} udi_global;


/* dist_contents interpretation */
/* expects: char *dist_values[] = { "both", "source" "binary", NULL }; */
#define DIST_BOTH   (udi_global.dist_contents == 0)
#define DIST_SOURCE (udi_global.dist_contents == 0 || \
                     udi_global.dist_contents == 1)
#define DIST_BINARY (udi_global.dist_contents == 0 || \
                     udi_global.dist_contents == 2)

/* Type interpretation */

#define IS_DRIVER	(udi_global.bintype == BIN_DRIVER)
#define IS_MAPPER	(udi_global.bintype == BIN_MAPPER)
#define IS_META		(udi_global.bintype == BIN_META)


/* 
 * Global/common function prototypes
 */

/* udi_tool_init
 *
 * Called immediately on tool startup to prepare to run this tool.
 */
void udi_tool_init(char *toolname);


/* udi_tool_exit
 *
 * Called on exit (success or failure) from a UDI tool to perform any needed
 * cleanup operations.
 */
void udi_tool_exit(int exitval);


/* parse_pathname
 * parse_filename
 *
 * All file specifications internal to the udiprops.txt are based on a
 * Unix-style specification (e.g. /usr/bin/ls).  None of the
 * udiprops.txt specifications are directory-only; all are expected to
 * be full file specifications.
 *
 * The parse_pathname routine returns a newly allocated string
 * containing the path but not the filename in the input argument; the
 * parse_filename conversely returns a newly allocated string
 * containing the file but not the path from the input argument. 
 */
char *parse_pathname(char *filespec);
char *parse_filename(char *filespec);


/* do_udiparse
 *
 * Called to read and parse the udiprops file.  (Yacc/Lex parser module).
 */
void do_udiparse(char *propfile);


/* os_delete
 *
 * Called to delete a file.
 */
void os_delete(char *file);


/* os_fullrm
 *
 * Called to recursively delete a directory or file
 */
void os_fullrm(char *fileordir);


/* do_rmdirs
 *
 * Called to remove any directories in the directory tree rooted
 * at the passed location.  If empty_only is non-zero, this routine
 * will remove only directories that are empty; if zero, all contents
 * will be removed along with the directory itself.
 * Returns non-zero if entire tree was empty,
 * or zero if only some (or none) of the tree was trimmed (used to allow
 * recursive implementation of this function).
 */
int do_rmdirs(char *startdir, int empty_only);


/* os_rmdir
 *
 * Called to delete an empty directory.  */
void os_rmdir(char *dir);


/* os_tempname
 *
 * Called to generate a unique name for a temporary file.  Returns a
 * pointer to a string that may be free()'d when done.
 */
char *os_tempname(char *dir, char *prefix);


/* os_mkdir
 *
 * Creates the named directory.  Prints error to stdout and calls
 * udi_tool_exit on failure.  On success the directory has been
 * created.  Fails if directory already exists.
 */
void os_mkdir(char *dirname);


/* os_subdir_name
 *
 * Adds a subdirectory to the current directory specification using 
 * OS-specific directory syntax.
 */
void os_subdir_name(char **rootdir, char *subdir);


/* os_dirfile_name
 *
 * Adds a specific filename reference to a directory path specification.
 */
void os_dirfile_name(char **dir, char *file);


/* udi_mktmpdir
 *
 * Creates a temporary directory that will be removed on udi_tool_exit.
 */
void udi_mktmpdir(char **dirname, char *subdir);


/* os_file_exists
 *
 * Returns non-zero if the specified file (or subdirectory) exists.
 */
int os_file_exists(char *filename);


/* os_isdir
 *
 * Returns non-zero if if the specified path is a directory.
 */
int os_isdir(char *path);


/* os_dirscan_start, os_dirscan_next, os_dirscan_done
 *
 * Called to scan all filenames in the specified directory, one at a time.
 */
void *os_dirscan_start(char *directory);
char *os_dirscan_next(void *dirhandle, char *prevfile);
void  os_dirscan_done(void *dirhandle);



/* os_exec
 *
 * Executes a generic OS operation in a sub-shell in the specified directory
 * and waits for its completion.  indir may be NULL to perform the operation
 * in the current working directory.
 */
void os_exec(char *indir, char *efile, char *argv[]);


/* os_copy
 *
 * Copies a file.  Brute force do_copy may be used if needed.
 */
void os_copy(char *dest, char *src);
void do_copy(char *src, char *dest);


/* udi_tmpcopy
 *
 * Copies a file to a destination that will be removed on udi_tool_exit.
 */
void udi_tmpcopy(char *dest, char *src);


/* udi_tmpcopyinto
 *
 * Copies a file into a destination directory.  The destination is a
 * temporary file that will be removed on udi_tool_exit.
 */
void udi_tmpcopyinto(char *dest, char *src);


/* os_mkpax
 *
 * Creates a named PAX archive from the pax directory tree specified,
 * relative to the src_dir.
 */
void os_mkpax(char *dest_pax, char *src_dir, char *pax_dir);


/* os_getpax
 *
 * Unpacks from a pax archive.
 */
void os_getpax(char *src_pax, char *dest_dir);


/* os_compile
 *
 * Copies a UDI C source file for udibuild.
 * The ccopts argument is a space separated list of the static properties
 * specified compile options (if any).
 */
void os_compile(char *cfile, char *ofile, char *ccopts, int debug, int optimize);


/* os_link
 *
 * Links a group of object files together into a single UDI module.
 * The objfiles argument is a space separated list of the object files
 * to be linked together.
 */
void os_link(char *exefile, char *objfiles, char *expfile);


/* os_file_newer
 *
 * Returns boolean indication of whether the first file has been
 * modified more recently than the second file.
 */
int os_file_newer(char *first, char *second);


/* Functions to initialize, copy, and concatenate strings,
 * reallocating the target to the proper size each time.  */
char *optinit(void);
void optcpy(char **dst, char *src);
void optncpy(char **dst, char *src, int num);
void optcat(char **dst, char *src);
void optncat(char **dst, char *src, int num);
#define optcatword(dst, src) { if (*(dst) && *(*(dst))) optcat(dst, " "); \
                               optcat(dst, src); }



/* count_args
 *
 * Counts the number of whitespace-separated words in the input string
 */
int count_args(char *str);


/* get_ccfiles
 *
 * Gets the C source files which correspond to the specified module
 * name.  Returns a space-separated list of files.  If include_h_files
 * is non-zero then header files are included in the return list as well.
 */
char *get_ccfiles(char *modname, int include_h_files);


/* get_incdirs
 *
 * Gets the directories associated with any header files listed on the
 * source_files lines.  Only unique directory names will be returned
 * and this will be used for automatically supplied -I include
 * specifications when compiling.  If with_I_flag is non-zero then
 * each directory will automatically be preceeed by the "-I" flag for
 * compile line inclusion.
 */
char *get_incdirs(char *modname, int with_I_flag);


/* get_ccopts
 *
 * Returns the compile_options specification as a series of
 * space-separated words for the specified module and source file to
 * be built.
 */
char *get_ccopts(char *modname, char *file);


/* set_abi_sprops
 *
 * Called by udimkpkg internals to attach the specified properties
 * file to the object module in an ABI-specific manner.
 */
void set_abi_sprops(char *propsfile, char *modulefile);


/* query_continue
 *
 * Called to query the user y/n to continue.  Returns non-zero if the
 * operation should continue.
 */

int query_continue();


/* get_prop_type
 *
 * Called to get the next property of the indicated type, starting at
 * the specified index into the static properties data.  Returns the
 * index of the next property in the global data or -1 if not found.
 */

int get_prop_type(int type, int index);


/* get_message
 * 
 * Called to get the message string translation for the specified
 * message number.  Returns an updated opt string containing the
 * message translation (including parse_message handling).
 */

void get_message(char **msgstr, int msgnum, char *pbreak);


/* parse_message
 *
 * Called to perform escape translation and formatting of a raw message string.
 * This routine does *not* do and format substitutions (%_).
 * The translation is appended to the input msgstr.
 */

void parse_message(char **msgstr, char *msg, char *pbreak);


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

void printp(char *pstring, char *indent, char *firstindent, int linelen);

void set_binary_type(void);

#endif /* _udi_global_h */
