/*
 * File: tools/common/spparse.h
 *
 * Common definitions used in the UDI tools
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

#include "global.h"

#define DEFAULT_INFILE	"udiprops.txt"
#define DEFAULT_PKGDIR	"."
#define DEFAULT_CFGFILE	"udicmds.cfg"
#define VER1_DIR	"udi-pkg.1"
#define VER_SUPPORT	"0x101"

#ifdef KWQ_OUT
typedef struct {
	int		type;
	char		*string;
	unsigned long	ulong;
} entries_t;
#endif

#define YYMAXDEPTH 1000

extern int yyparse();
void yyerror(char *s);
extern int yylvalue;
#ifdef OS_HAS_NO_LEX
extern unsigned char yytext[];
#endif
extern int lineno, linesize;
extern void endofline(int is_continued);
extern int entindex;

#define input_data udi_global.propdata

int rettoken(int token);

extern FILE *yyin;
#ifdef KWQ_OUT
extern char *progname;
#endif
extern char infile[513];
extern int input_num;
extern int input_size;
extern int use_src;
extern int module_ent;
extern propline_t entries[512];
extern int startdec;
extern char *cc;
extern char *ccopts;
extern char *ccincs;
extern char *ccudiincdir;
extern char *ccoutopts;
extern char *ccdbgopts;
extern char *ccoptimopts;
extern char *linker;
extern char *linkopts;
extern char *linkoutopts;
extern char *fileccopts;
extern char *proc;
extern int module[256];
extern int module_cnt;
extern char fileout[513];
extern char *outdir;
extern char *cfgfile;
extern char *ccfiles;
extern int num_ccfiles;
extern char *pkg;
extern char *propfile;
extern char *udi_filedir;
extern unsigned long prop_seen;

/* Used by exec_cmd */
#define COMPILE		3
#define LINK		4
#define ANYTHING	5
#define CHMOD		6
#define UNPAX		7
#define PAX		8


/* Use BuildString just like strcat except that this will verify that the
 * target string has enough storage capacity for the appendation and will
 * abort the executable if there isn't enough room. */

#define BuildString(S,A) {						\
	    if ((sizeof(S)-strlen(S)) <= strlen(A)) {			\
		fprintf(stderr,						\
			"Error: cannot append %s (%d)\n"		\
			"       to string     %s (%d of %d)\n",		\
			(A), strlen(A), (S), strlen(S), sizeof(S));	\
		exit(ENOMEM);						\
	    }								\
	    strcat((S),(A));						\
	}
