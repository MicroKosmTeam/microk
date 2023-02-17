/*
 * File: tools/common/param.c
 *
 * UDI Utilities Typical Parameter Handling Code
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
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include "osdep.h"
#include "param.h"
#include "global.h"
#include "common.h"
#include "common_api.h"
#include "spparse.h"      /* for VER_SUPPORT, VER1_DIR */
#include "udi_abi.h"      /* for UDI_ABI_xxx */


/*
 * Standard Parameters Definitions
 */


#ifndef OS_BLDTMPDIR
#define OS_BLDTMPDIR "/tmp"
#endif


char *anyval[] = { NULL };

char *dist_values[] = { "both", "source", "binary", NULL };

int full_usage;

#define Flg(Var,Char,Desc) {&udi_global.Var, Flag,0,NULL, Char, Desc, anyval}

#define I18NSCANSTR	"%d:%c:%[^\\0]"

/*
 * These definitions need to be here, so they can get internationalized.
 * The format is "msg_num:message".  Various message numbers are used
 * in the below udi_util_params definition, during help messages.
 */
#define I18Nstr00 "1650: :Show usage info"
#define I18Nstr01 "1651: :Show FULL usage info"
#define I18Nstr02 "1652: :Show Version/Config info"
#define I18Nstr03 "1653: :Verbose output"
#define I18Nstr04 "1654: :Dryrun: With -v, echo commands only, no execution"
#define I18Nstr05 "1655: :Temporary directory for udi utilities to use"
#define I18Nstr06 "1656:!:Directory to direct resulting files to"
#define I18Nstr07 "1657: :Build Debug objects"
#define I18Nstr08 "1658: :Build Optimized objects"
#define I18Nstr09 "1659: :Build (update) only objects whose source has changed"
#define I18Nstr10 "1660:!:Udibuild is being done for a mapper"
#define I18Nstr11 "1661:~:Compiler to be used"
#define I18Nstr12 "1662:~:Additional compiler options"
#define I18Nstr13 "1663:~:Linker to be used"
#define I18Nstr14 "1664:~:Additional linker options"
#define I18Nstr15 "1665: :Specifies distribution type"
#define I18Nstr16 "1666: :<shortname>"
#define I18Nstr17 "1667: :Name of the driver being packaged"
#define I18Nstr18 "1668: :<release>"
#define I18Nstr19 "1669: :Name of the package alternative"
#define I18Nstr20 "1670: :<module>"
#define I18Nstr21 "1671: :Name of the package component"
#define I18Nstr22 "1672: :Overwrite any existing output package file"
#define I18Nstr23 "1673: :Update/combine into existing output package file"
#define I18Nstr24 "1674:!:Stop udisetup prior to install"
#define I18Nstr25 "1675: :User yes/no to confirm each package & component"
#define I18Nstr26 "1676:!:Quiet mode; minimal output on success"
#define I18Nstr27 "1677: :Show current UDI setup (no input processed)"
#define I18Nstr28 "1678:!:Udisetup is being done for a mapper"
#define I18Nstr29 "1679:~:Compiler to be used, with any additional options"
#define I18Nstr30 "1680:!:Name of the OS Specific package for this driver"
#define I18Nstr31 "1681: :[UDI_pkg_file]:The specific udi packaged file to run udisetup on"
#define I18Nstr32 "1682:!:Display info of driver shortname"
#define I18Nstr33 "1407:!:Udimkpkg is being done for a mapper"


udiutil_param_t udiutil_params[] = {

    /* Parameters common to all UDI Tools */

    Flg(show_usage, "h", I18Nstr00), /* 1650: :Show usage info */
    Flg(show_usage, "?", I18Nstr00), /* 1650: :Show usage info */
    { &full_usage, Flag,
      0,
      NULL,
      "H",
      I18Nstr01, /* 1651: :Show FULL usage info */
      anyval
    },
/*  { &version_q, Flag,
      0,
      NULL,
      "V",
      I18Nstr02, *//* 1652: :Show Version/Config info *//*
      anyval
    },
 */
    Flg(verbose,    "v", I18Nstr03), /* 1653: :Verbose output */
    Flg(no_exec,    "n", I18Nstr04), /* 1654: :Dryrun: With -v, echo commands
				   only, no execution */
    { &udi_global.bldtmpdir, ExistingDirectory,
      OS_BLDTMPDIR,        /* default */
      "UDI_BLDTMPDIR",     /* env var override */
      "T",                 /* cmd line flag */
      I18Nstr05, /* 1655: :Temporary directory for udi utilities to use */
      anyval
    },
    { &udi_global.outdir, ExistingDirectory,
      ".",                 /* default */
      0,
      "o",                 /* cmd line flag */
      I18Nstr06, /* 1656:!:Directory to direct resulting files to */
      anyval
    },

#ifdef _UDIBUILD    
    /* Parameters specific to udibuild */

    Flg(debug,    "D", I18Nstr07), /* 1657: :Build Debug objects */
    Flg(optimize, "O", I18Nstr08), /* 1658: :Build Optimized objects */
    Flg(update,   "u", I18Nstr09), /* 1659: :Build (update) only objects whose
				      source has changed */
    Flg(make_mapper,  "m", I18Nstr10), /* 1660:!:Udibuild is being done for a
					  mapper */
    { &udi_global.cc, String,
      UDI_CC,    /* default */
      "UDI_CC",
      0,
      I18Nstr11, /* 1661:~:Compiler to be used */
      anyval
    },
    { &udi_global.ccopts, String,
      UDI_CCOPTS,    /* default */
      "UDI_CCOPTS",
      0,
      I18Nstr12, /* 1662:~:Additional compiler options */
      anyval
    },
    { &udi_global.link, String,
      UDI_LINK,    /* default */
      "UDI_LINK",
      0,
      I18Nstr13, /* 1663:~:Linker to be used */
      anyval
    },
    { &udi_global.linkopts, String,
      UDI_LINKOPTS,    /* default */
      "UDI_LINKOPTS",
      0,
      I18Nstr14, /* 1664:~:Additional linker options */
      anyval
    },
#endif

#ifdef _UDIMKPKG
    /* Parameters specific to udimkpkg */

    { &udi_global.dist_contents, NamedIndex,
      "both",                          /* default is both in distribution */
      NULL,                              /* no env override */
      "t",
      I18Nstr15, /* 1665: :Specifies distribution type */
      dist_values
    },
    { &udi_global.drv_xxx, String, I18Nstr16 /* 1666:~:<shortname> */,
      "UDI_DRV",
      "d",
      I18Nstr17, /* 1667: :Name of the driver being packaged */
      anyval
    },
    { &udi_global.alt_yyy, String, I18Nstr18 /* 1668:~:<release> */,
      "UDI_ALT",
      "a",
      I18Nstr19, /* 1669: :Name of the package alternative */
      anyval
    },
    { &udi_global.comp_zzz, String, I18Nstr20 /* 1670:~:<module> */,
      "UDI_COMP",
      "c",
      I18Nstr21, /* 1671: :Name of the package component */
      anyval
    },
    Flg(overwrite, "O", I18Nstr22), /* 1672: :Overwrite any existing output
				       package file */
    Flg(update,    "U", I18Nstr23), /* 1673: :Update/combine into existing
				  output package file */
    Flg(make_mapper,  "m", I18Nstr33), /* 1407:!:Udimkpkg is being done for a
					  mapper */
#endif

#ifdef _UDISETUP
    /* Parameters specific to udisetup */

    Flg(debug,        "D", I18Nstr07), /* 1657: :Build Debug objects */
    Flg(optimize,     "O", I18Nstr08), /* 1658: :Build Optimized objects */
    Flg(stop_install, "s", I18Nstr24), /* 1674:!:Stop udisetup prior to
					  install */
    Flg(query,        "c", I18Nstr25), /* 1675: :User yes/no to confirm each
					  package & component */
    Flg(quiet,        "q", I18Nstr26), /* 1676:!:Quiet mode; minimal output on
					  success */
    Flg(showsaved,    "S", I18Nstr27), /* 1627: :Show current UDI setup (no
					  input processed) */
    Flg(make_mapper,  "m", I18Nstr28), /* 1628:!:Udisetup is being done for a
					  mapper */
    Flg(disp_short,   "i", I18Nstr32), /* 1682:!:Display info of driver shortname */
    { &udi_global.cc, String,
      UDI_CC,    /* default */
      "UDI_CC",
      0,
      I18Nstr29, /* 1679:~:Compiler to be used, with any additional options */
      anyval
    },
    { &udi_global.link, String,
      UDI_LINK,    /* default */
      "UDI_LINK",
      0,
      I18Nstr13, /* 1663:~:Linker to be used */
      anyval
    },
    { &udi_global.linkopts, String,
      UDI_LINKOPTS,    /* default */
      "UDI_LINKOPTS",
      0,
      I18Nstr14, /* 1664:~:Additional linker options */
      anyval
    },
    { &udi_global.pkg_install, String, NULL,
      NULL,	/* No env override */
      "p",
      I18Nstr30, /* 1680:!:Name of the OS Specific package for this driver */
      anyval
    },
    { &udi_global.file_install, ExistingFiles, NULL,
      NULL,
      0,
      I18Nstr31, /* 1681: :[UDI_pkg_file]:The specific udi packaged file to
		    run udisetup on */
      anyval
    }
#endif
};

/*
 * Miscellaneous support operations
 */

#ifndef ARRAYSIZE
#define ARRAYSIZE(Array)   (sizeof(Array)/sizeof(Array[0]))
#endif



/*
 * Parameter Management Code
 */


/* set_std_arg
 *
 * Internal support routine to set a parameter's udi_global value.
 */

static void
set_std_arg(udiutil_param_t *pp, char *pval, int flagval)
{
    int jj;
    int msgnum;
    char msgtype, msg[160];
    
    switch (pp->ptype) {
	
    case Flag:
	SET_PARAM_INT(pp, flagval);
	break;
	
    case NamedIndex:
	for (jj = 0; pp->pvalid[jj] != NULL; jj++)
	    if (strcmp(pval, pp->pvalid[jj]) == 0)
		break;
	SET_PARAM_INT(pp, jj);
	break;
	
    case Integer:
	assert(sizeof(int) > sizeof(pp->p_pvar));
	SET_PARAM_INT(pp, (pval ? strtol(pval, 0, 0) : 0));
	break;
	
    case String:
    case ExistingFile:
    case ExistingFiles:
    case ExistingDirectory:
    case NewDirectory:
    case ExistingOrNewDirectory:
	SET_PARAM_STR(pp, pval);  /* points to original storage space */
	break;

    default:
	msg[0] = msgnum = msgtype = 0;
	if (pp->phelp) {
        	sscanf(pp->phelp, I18NSCANSTR, &msgnum, &msgtype, msg);
		strcpy(msg, (const char *)getloc(msgnum, msg));
	}
	printloc(stderr, UDI_MSG_FATAL, 1600, "no handling for %c "
		 "type %d:\n\t%s\n",
		 *pp->pcmdline, pp->ptype, msg);
    }
}


/* set_nonstd_arg
 *
 * Another internal support routine to set a parameter's udi_global value.
 */
static int
set_nonstd_arg(char *pval, int *cnt)
{
    udiutil_param_t *pp;
    int ii, numfound;
    char *ptr;
    int msgnum;
    char msgtype, msg[160];

    /* Find the parameters which have no pcmdline, and are special cased */
    numfound = 0;
    for (ii = 0; ii < ARRAYSIZE(udiutil_params); ii++) {
	pp = &(udiutil_params[ii]);
	if (! pp->pcmdline) {  /* not an option flag, but an argument */
            if (pp->phelp) {
        	sscanf(pp->phelp, I18NSCANSTR, &msgnum, &msgtype, msg);
                ptr = strchr(msg, ':');
                if (msg[0] == '[' && ptr > msg && *(ptr-1) == ']') {
                    if (numfound == *cnt || pp->ptype == ExistingFiles) {
                        /* We found a matching one */
                        SET_PARAM_STRADD(pp, pval);
                        (*cnt)++;
                        return 1;
                    } else {
                        numfound++;
                    }
                }
            }
        }
    }
    return 0;
}


/* __show_usage
 *
 * Routine called internally to dump the usage information for this utility
 * to stdout for the user's edification.
 */

static char *
word_fill(char *word, int len, char *fill)
{
    int ii, wlen, flen;
    static char fmtout[128];

    wlen = strlen(word);
    flen = strlen(fill);
    assert(len < 128);

    for (ii = 0; ii <= len; ii++)
	fmtout[ii] = (ii < wlen ? word[ii] : fill[ii % flen]);
    fmtout[ii] = 0;
    return fmtout;
}


static void
print_size(int size)
{
	int mb, kb, sb;

	fprintf(stderr, "%d", size);
	if (size >= 1024) {
		fprintf(stderr, " (");
		mb = size / 1024 / 1024;
		if (mb) fprintf(stderr, "%d MB", mb);
		sb = size - (mb * 1024 * 1024);
		kb = sb / 1024;
		if (kb) fprintf(stderr, "%s%d KB", (mb ? " " : ""), kb);
		sb = sb - (kb * 1024);
		if (sb) fprintf(stderr, "%s%d B", (mb || kb ? " " : ""), sb);
		fprintf(stderr, ")");
	}
}


static void
show_usageline(int full_usage)
{
    udiutil_param_t *pp;
    int ii;
    char *ptr;
    int msgnum;
    char msgtype, msg[160];

    printhloc(1608, "usage: %s [options]",
	udi_global.progname);
    for (ii = 0; ii < ARRAYSIZE(udiutil_params); ii++) {
	pp = &(udiutil_params[ii]);
	if (! pp->pcmdline) {
            if (pp->phelp) {
		msg[0] = msgnum = msgtype = 0;
                sscanf(pp->phelp, I18NSCANSTR, &msgnum, &msgtype, msg);
		strcpy(msg, (const char *)getloc(msgnum, msg));
		if (msgtype == '!' || msgtype == '~') {
			if (!full_usage) continue;
		}
                ptr = strchr(msg, ':');
                if (msg[0] == '[' && ptr > msg && *(ptr-1) == ']') {
                    *ptr = 0;
                    fprintf(stderr, " ");
                    printhloc(msgnum, msg);
                }
            }
        }
    }
    fprintf(stderr, "\n");
}


static void
show_help_usage(int full_usage)
{
    show_usageline(full_usage);
    printloc(stderr, 0, 1607, "         Try -h for help.\n");
}


void
show_usage(int full_usage)
{
	udiutil_param_t *pp;
	int ii;
	char *ptr;
	int msgnum;
	char msgtype, msg[160];
	static char blankspace[] = "            ";
	
	show_usageline(full_usage);
	
	for (ii = 0; ii < ARRAYSIZE(udiutil_params); ii++) {
		pp = &(udiutil_params[ii]);
		ptr = pp->phelp;
		msg[0] = msgtype = msgnum = 0;
		if (ptr) {
			sscanf(ptr, I18NSCANSTR, &msgnum, &msgtype, msg);
			strcpy(msg, (const char *)getloc(msgnum, msg));
		}
		if (msgtype == '!' || msgtype == '~') {
			if (!full_usage) continue;
		}
		if (pp->pcmdline)
			fprintf(stderr, "\t-%c %s %s\n", *pp->pcmdline,
			       word_fill(
				       (pp->ptype == Flag ? "" : 
					pp->ptype == Integer ? "int" :
					pp->ptype == NamedIndex ? "keywd" :
					pp->ptype == String ? "str" :
					pp->ptype == ExistingDirectory ? "dir":
					pp->ptype == NewDirectory ? "newdir" :
					pp->ptype == ExistingOrNewDirectory ?
					"[new]dir" :
					"!?!"), 12, ". "),
			       msg);
		else {
			if (msg[0] && msgtype != '~') {
				ptr = strchr(msg, ':');
				if (msg[0] == '[' && ptr > msg &&
				    *(ptr-1) == ']') {
					*ptr = 0; ptr++;
					fprintf(stderr, "\t%s %s\n",
					       word_fill(msg, 15, " ."), ptr);
				} else {
					printhloc(1609, "\t  option:     %s\n",
					       msg);
				}
			}
		}
		if (*pp->pvalid) {
			char **pv;
			printhloc(1610, "\t%s\t   keywd", blankspace);
			for (pv = pp->pvalid; *pv; pv++)
				fprintf(stderr, "%c %s",
				       (pv == pp->pvalid ? ':' : ','), *pv);
			fprintf(stderr, "\n");
		}
		if (pp->penvvar && msgtype != '~') {
			if (getenv(pp->penvvar))
				printhloc(1611, "\t%s\t   ENV Var: %s [=%s]\n", 
				       blankspace,
				       pp->penvvar,
				       getenv(pp->penvvar));
			else
				printhloc(1612, "\t%s\t   ENV Var: %s\n",
				       blankspace, pp->penvvar);
		}
		if (pp->pdef && msgtype != '~') {
			sscanf(pp->pdef, I18NSCANSTR, &msgnum, &msgtype, msg);
			strcpy(msg, (const char *)getloc(msgnum, msg));
			printhloc(1613, "\t%s\t   Default: %s\n", blankspace,
				msg);
		}
	}

	/* Print out environment variable overrides, not already mentioned */
	printhloc(1614, " --- Environment Variable Overrides ---\n");
	for (ii = 0; ii < ARRAYSIZE(udiutil_params); ii++) {
		pp = &(udiutil_params[ii]);
		ptr = pp->phelp;
		msg[0] = msgtype = msgnum = 0;
		if (ptr) {
			sscanf(ptr, I18NSCANSTR, &msgnum, &msgtype, msg);
			strcpy(msg, (const char *)getloc(msgnum, msg));
		}
		if (ptr == NULL || msgtype != '~') continue;
		if (pp->pcmdline) continue;
		if (pp->penvvar)
			fprintf(stderr, "\t%s %s\n",
				       word_fill(pp->penvvar, 30, " ."), msg);
		if (pp->pdef)
			printhloc(1615, "\t%s\t   Default: %s\n", blankspace,
				pp->pdef);
		if (getenv(pp->penvvar))
			printhloc(1616, "\t%s\t   Current: %s\n", blankspace,
					getenv(pp->penvvar));
	}

	printhloc(1617, " --- UDI Version: %s  ---  ABI: %s \"%s\" ---\n",
	       VER_SUPPORT, UDI_ABI_NAME, UDI_ABI_PROCESSORS);
	if (full_usage) {
#define SAPRNT(V) print_size(V); fprintf(stderr, "\n");
		printhloc(1618, "       Max Call Stack Size: ");
		SAPRNT(UDI_ABI_MAX_CALL_STACK);
		printhloc(1619, "      Max Module Data Size: ");
		SAPRNT(UDI_ABI_MAX_MOD_DATA);
		printhloc(1620, "      Max Module File Size: ");
		SAPRNT(UDI_ABI_MAX_MOD_FILE);
		printhloc(1621, "       Max Code per Driver: ");
		SAPRNT(UDI_ABI_MAX_DRV_CODE);
		printhloc(1622, "       Max Data per Driver: ");
		SAPRNT(UDI_ABI_MAX_DRV_DATA);
		printhloc(1623, "                Endianness: %s\n",
			  UDI_ABI_ENDIANNESS);
		printhloc(1624, "        Object File Format: %s\n",
			  UDI_ABI_OBJFMT);
		printhloc(1625, " Static Properties Binding: %s\n",
			  UDI_ABI_UDIPROPS);
		printhloc(1626, "    Package Directory Root: %s\n",
			  VER1_DIR);
	}
}



/* invalid_value
 *
 * Function called internally to generate the proper conniptions when
 * a parameter is specified with an invalid value.
 */

static void
invalid_value(udiutil_param_t *pp)
{
    printloc(stderr, UDI_MSG_ERROR, 1601, "invalid value for parameter %c\n",
	             *pp->pcmdline);
    show_help_usage(0);
}



/* UDI_Util_Init_Params
 *
 * This is the main routine called by the utility to initialize the
 * parameters to their default values before any other parsing is
 * performed.
 */

void
UDI_Util_Init_Params(void)
{
    udiutil_param_t *pp;

    for (pp = udiutil_params + ARRAYSIZE(udiutil_params) - 1;
	 pp >= udiutil_params;
	 pp--)
	set_std_arg(pp, pp->pdef, 0);
}

/* UDI_Util_Get_Params
 *
 * This is the main routine called by the utility to initialize the
 * udi_global variables from various defaults, environment variables,
 * and command line arguments.  */

void
UDI_Util_Get_Params(int argc, char **argv)
{
    int jj, opti, found, nonstd_cnt, c;
    char opts[80], *tparam, *pval, *words, *ptr;
    udiutil_param_t *pp;
    int msgnum;
    char msgtype, msg[160];

    opti = 0;

    /*
     * As a special case, we prescan for the presence of a mapper
     * flag as it may need to alter the behaviour of the overrides.
     */

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

#if defined (OS_OVERRIDE_PARAMS)
    OS_OVERRIDE_PARAMS();
#endif /* OS_OVERRIDE_PARAMS */

    /*
     * Apply environment variable overrides.
     */

    for (pp = udiutil_params + ARRAYSIZE(udiutil_params) - 1;
	 pp >= udiutil_params;
	 pp--) {
	char *pval;
	if (pp->penvvar) {
	    pval = getenv(pp->penvvar);
	    if (pval)
		set_std_arg(pp, pval, 1);
	}
	/* Also initialize the opts string for later getopt call */
	if (pp->pcmdline) {
	    opts[opti++] = *pp->pcmdline;
	    if (pp->ptype != Flag)
		opts[opti++] = ':';
	}
    }
    opts[opti] = 0;


    /*
     * And now parse the command line options...
     */

    while ((jj = getopt(argc, argv, opts)) != EOF) {
        found = 0;
	for (pp = udiutil_params + ARRAYSIZE(udiutil_params) - 1;
	     pp >= udiutil_params;
	     pp--)
	    if (pp->pcmdline && jj == pp->pcmdline[0]) {
		set_std_arg(pp, optarg, 1);
                found = 1;
		break;
            }
	if (!found) {
	    show_help_usage(full_usage);
	    udi_tool_exit(22);
	}
    }
    /* Get the remaining non-optioned options */
    nonstd_cnt = 0;
    for ( ; optind < argc; optind++) {
        /* Search the parameters for ones having no line option prefix */
        found = set_nonstd_arg(argv[optind], &nonstd_cnt);
        if (!found) {
	    show_help_usage(full_usage);
	    udi_tool_exit(22);
        }
    }

    /*
     * Now validate the parameter
     */

    for (pp = udiutil_params + ARRAYSIZE(udiutil_params) - 1;
	 pp >= udiutil_params;
	 pp--) {
	if (*pp->pvalid)
	    switch (pp->ptype) {

	    case Flag: break; /* valid values are ignored for flags */

	    case Integer:
		for (jj = 0; pp->pvalid[jj] != NULL; jj++)
		    if (PARAM_INT(pp) == strtol(pp->pvalid[jj], 0, 0))
			break;
		if (*pp->pvalid == NULL) {
		    invalid_value(pp);
		    udi_tool_exit(22);
		}
		break;

	    case NamedIndex:
		for (jj = 0; pp->pvalid[jj] != NULL; jj++) ;
		if (PARAM_INT(pp) >= jj) {
		    invalid_value(pp);
		    udi_tool_exit(22);
		}
		break;

	    case String:
		for (jj = 0; pp->pvalid[jj] != NULL; jj++)
		    if (strcmp(PARAM_STR(pp), pp->pvalid[jj]) == 0)
			break;
		if (*pp->pvalid == NULL) {
		    invalid_value(pp);
		    udi_tool_exit(22);
		}
		break;

	    case ExistingDirectory:
	    case ExistingFile:
 	    case ExistingFiles:
	    case NewDirectory:
	    case ExistingOrNewDirectory:
		break;  /* No list of valid values for these */
	    }

	/* Now perform active validation for the remaining parameters */

	switch (pp->ptype) {
	default: break;  /* No special massage for normal params */

        case ExistingFile:
	case ExistingDirectory:
	case NewDirectory:
	case ExistingOrNewDirectory:
	    if (strlen(PARAM_STR(pp)) == 0) {
		printloc(stderr, UDI_MSG_ERROR, 1602,
			 "must specify a valid %s for %c\n",
			 (pp->ptype==ExistingFile)?"file":"directory",
			 *pp->pcmdline);
		if (udi_global.show_usage) show_usage(full_usage);
		else show_help_usage(full_usage);
		udi_tool_exit(20);
	    }
	    /* fall thru */
        case ExistingFiles:
	    words = optinit();
	    optcpy(&words, PARAM_STR(pp));
	    for (pval = strtok(words, " \t\n");
		 pval;
		 pval = strtok(NULL, " \t\n")) {

		switch (pp->ptype) {
		default: break; /* compiler satiation */

		case ExistingFile:
		case ExistingFiles:
		    /* If "all" then ignore stat */
		    msg[0] = msgnum = msgtype = 0;
		    if (pp->phelp) {
        		sscanf(pp->phelp, I18NSCANSTR, &msgnum, &msgtype, msg);
			strcpy(msg, (const char *)getloc(msgnum, msg));
		    }
		    ptr = strchr(msg, ':');
		    if (!pp->pcmdline && ptr > msg && msg[0] == '[' &&
                        *(ptr-1) == ']' &&
			strcmp(pval, "all") == 0)
			continue;
		    /* Check if file exists */
		    if (os_file_exists(pval)) break;
		    /* Show usage information */
		    if (!pp->pcmdline && ptr > msg && msg[0] == '[' &&
                        *(ptr-1) == ']') {
                        tparam = optinit();
                        optcpy (&tparam, msg);
                        *(strchr(tparam, ':')) = 0;
                        printloc(stderr, UDI_MSG_ERROR, 1603,
				 "need an existing file for %s; %s "
				 "invalid\n", tparam, pval);
                        free(tparam);
		    } else {
			printloc(stderr, UDI_MSG_ERROR, 1604,
				 "need an existing file for -%c; %s invalid\n",
				 *pp->pcmdline, pval);
		    }
		    if (udi_global.show_usage) show_usage(full_usage);
		    else show_help_usage(full_usage);
		    udi_tool_exit(2);

		case ExistingDirectory:
		    if (os_isdir(pval)) break;
		    printloc(stderr, UDI_MSG_ERROR, 1605,
			     "need an existing directory for -%c; %s invalid\n",
			     *pp->pcmdline, pval);
		    if (udi_global.show_usage) show_usage(full_usage);
		    else show_help_usage(full_usage);
		    udi_tool_exit(2);

		case NewDirectory:
		    if (!os_isdir(pval) && !os_file_exists(pval)) break;
		    printloc(stderr, UDI_MSG_ERROR, 1606,
			     "directory %s already exists!\n",
			     PARAM_STR(pp));
		    if (udi_global.show_usage) show_usage(full_usage);
		    else show_help_usage(full_usage);
		    udi_tool_exit(17);

		case ExistingOrNewDirectory:
		    break;
		}
	    }
	    free(words);
	}
    }
    
    if (full_usage && !udi_global.show_usage)
	    udi_global.show_usage = 1;

    if (udi_global.show_usage) {
	    show_usage(full_usage);
	    udi_tool_exit(0);
    }
}

