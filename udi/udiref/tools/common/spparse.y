%{
/*
 * File: tools/common/spparse.y
 *
 * UDI Static Properties Parser Yacc file
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
#include <string.h>
#include <stdlib.h>
#include "global.h"
#include "spparse.h"
#include "common.h"

propline_t entries[512];
int cnt;
int module[256];
int module_cnt;
int parsemfile;

extern int entindex;
extern int use_src;
extern int lineno, linesize;
extern void saveinput();
extern void verify_device_enum(int type);
extern void verify_string(size_t length);
extern void verify_filename();
extern void verify_filespec();
extern void maketext(int index);
extern void endofline(int is_continued);
static void requires(void);
static void provides(void);


/** Used to verify the proper presence and combinations of properties **/
#define P_PROP_VER     (1U<<0)
#define P_SUPPLIER     (1U<<1)
#define P_CONTACT      (1U<<2)
#define P_NAME         (1U<<3)
#define P_SHORTNAME    (1U<<4)
#define P_RELEASE      (1U<<5)
#define P_REQUIRES     (1U<<6)
#define P_MODULE       (1U<<7)
#define P_REGION       (1U<<8)
#define P_PROVIDES     (1U<<9)
#define P_SYMBOLS      (1U<<10)
#define P_CATEGORY     (1U<<11)
#define P_CFGCHOICE    (1U<<12)
#define P_CHILD_BOPS   (1U<<13)
#define P_PARENT_BOPS  (1U<<14)
#define P_INT_BOPS     (1U<<15)
#define P_MULTIPARENT  (1U<<16)
#define P_LOCALE       (1U<<17)
#define P_MESSAGE      (1U<<18)
#define P_DISMESG      (1U<<19)
#define P_MSGFSTART    (1U<<20)
#define P_META         (1U<<21)
#define P_ENUMERATES   (1U<<22)
#define P_DEVICE       (1U<<23)
#define P_CUSTOM       (1U<<24)
#define P_PIOSERLIMIT  (1U<<25)
#define P_READABLE     (1U<<26)
#define P_NSI          (1U<<27)
#define PTYPE_MSG_FILE (1U<<28)
#define PTYPE_LIBRARY  (1U<<29)
#define PTYPE_DRIVER   (1U<<30)
#define PTYPE_INVALID_RESERVED_NOUSE (1U<<31)

#define PTYPE_MASK (PTYPE_DRIVER | PTYPE_LIBRARY | PTYPE_MSG_FILE)


 static struct 
 {
         unsigned long if_present;
         unsigned long allow;
         unsigned long require;
 } prop_comb[] = {
         /* All libraries and drivers must have these */
         {~PTYPE_MSG_FILE, ~0U,        (P_PROP_VER | P_SUPPLIER |
                                        P_CONTACT | P_NAME |
                                        P_SHORTNAME | P_RELEASE | P_REQUIRES)},
         /* only one of the following are allowed ... */
         {P_PROP_VER,    ~P_PROP_VER,     0},
         {P_SUPPLIER,    ~P_SUPPLIER,     0},
         {P_NAME,        ~P_NAME,         0},
         {P_SHORTNAME,   ~P_SHORTNAME,    0},
         {P_RELEASE,     ~P_RELEASE,      0},
         {P_MULTIPARENT, ~P_MULTIPARENT,  0},
         {P_PIOSERLIMIT, ~P_PIOSERLIMIT,  0},
         /******************** DRIVER ********************/
         /* Things that must be in a driver */
         {P_PARENT_BOPS, ~0U,             P_DEVICE},
         {P_DEVICE,      ~0U,             P_PARENT_BOPS},
         {PTYPE_DRIVER,  ~0U,             P_REGION},
         /******************** LIBRARY ********************/
         /* Things that must be in a library */
         {PTYPE_LIBRARY, ~0U,             P_PROVIDES},
         /******************** MESSAGE FILE ********************/
         /* Things that must be in a message file */
         {PTYPE_MSG_FILE,  (P_PROP_VER |
                            P_LOCALE |
                            P_MESSAGE |
                            P_DISMESG |
                            P_MSGFSTART),                   0},
         /* end of list */
         {0, 0, 0} };

unsigned long prop_seen;

static struct
{
        unsigned long ptype;
        unsigned long p_mask;
} ptype_set[] = {
        { PTYPE_DRIVER,  (P_META | P_CHILD_BOPS | P_DEVICE | P_ENUMERATES |
                          P_MULTIPARENT |
                          P_READABLE | P_CUSTOM | P_REGION |
                          P_PIOSERLIMIT | P_NSI | P_PARENT_BOPS |
                          P_INT_BOPS) },
        { PTYPE_LIBRARY,  (P_PROVIDES | P_SYMBOLS | P_CATEGORY) },
        { PTYPE_MSG_FILE, 0  /* determined before we start parsing */ }
};


static void prop_p_names(char **optstr, unsigned long propmask);
static void check_prop(unsigned long new_prop);

#define SAW_PROP(Prop) check_prop(Prop)

#define END_CHK_PROP() {                                                     \
    int ecp_ii;                                                              \
    for(ecp_ii=0; prop_comb[ecp_ii].if_present;ecp_ii++)                     \
        if ((prop_comb[ecp_ii].if_present & prop_seen) &&                    \
            ((prop_seen & prop_comb[ecp_ii].require) !=                      \
             prop_comb[ecp_ii].require)) {                                   \
            char *reqp;                                                      \
            reqp = optinit();                                                \
            prop_p_names(&reqp, (prop_comb[ecp_ii].require &                 \
                                 ~(prop_comb[ecp_ii].require & prop_seen))); \
            printloc(stderr, UDI_MSG_FATAL, 1204,                            \
                     "Required property missing: %s (%x/%x)\n",              \
                     reqp, prop_comb[ecp_ii].require, prop_seen);            \
        } }
 
static int prop_ver_seen;
static int supplier_seen;
static int contact_seen;
static int name_seen;
static int shortname_seen;
static int release_seen;
static int requires_seen;
static int module_seen;
static int region0_seen;
static int provides_seen;

void fatal(const char *msg, ...);
void verify_region(void);
void verify_symbols(void);
void verify_compopts(void);
void verify_choices(void);
void verify_custom(void);
void verify_pioserlim(void);
void verify_provides(void);
void do_udiparse(char *propfile);

void set_type_shortname(void);
void set_type_module(void);

#define VSTRING		1
#define VUBIT32		2
#define VBOOLEAN	3
#define VARRAY		4

int verify_type(int type, propline_t *ent);
void verify_attr_string(int num);
void verify_message_num(unsigned long num);
int yylex(void);

%}

%start statement

%union {
	char		*string;
}

%token <string> CATEGORY 260
%token <string> CFGCHOICE 261
%token <string> CHLDBOPS 262
%token <string> COMPOPTS 263
%token <string> CONTACT 264
%token <string> CUSTOM 265
%token <string> DEVICE 266
%token <string> DISMESG 267
%token <string> ENUMERATES 268
%token <string> INTBOPS 269
%token <string> LOCALE 270
%token <string> MESGFILE 271
%token <string> MESSAGE 272
%token <string> META 273
%token <string> MODULE 274
%token <string> MULTIPARENT 275
%token <string> NAME 276
%token <string> NSI 277
%token <string> PARBOPS 278
%token <string> PIOSERLIMIT 279
%token <string> PROPVERS 280
%token <string> PROVIDES 281
%token <string> READFILE 282
%token <string> REGION 283
%token <string> RELEASE 284
%token <string> REQUIRES 285
%token <string> SHORTNAME 286
%token <string> SRCFILES 287
%token <string> SRCREQS 288
%token <string> SUPPLIER 289
%token <string> SYMBOLS 290
%token <string> MAPPER 291

%token <string> CR 500
%token <string> HEX 501
%token <string> INTEG 502
%token <string> STRING 503

%token METAIDX 700
%token MSGNUM 701
%token MSGFSTART 800
%token MSGFPROPV 801

%type <string> relnum
%type <string> text
%type <string> line
%type <string> keytext
%type <string> nohexkeytext
%type <string> nohextext

%%
statement: line {}
	| line statement {}
	;
line:	CR {
		/* Do nothing */
	}
	| CATEGORY INTEG CR {
			/* TODO: actions */
                        SAW_PROP(P_CATEGORY);
			entries[entindex-1].type = MSGNUM;
			saveinput();
		}
	| CFGCHOICE INTEG text CR {
			/* TODO: verify text and actions */
                        SAW_PROP(P_CFGCHOICE);
			verify_choices();
			saveinput();
		}
	| CHLDBOPS INTEG INTEG INTEG CR {
			/* TODO: verification */
                        SAW_PROP(P_CHILD_BOPS);
			entries[entindex-3].type = CHLDBOPS;
			saveinput();
		}
	| COMPOPTS CR {
			saveinput();
		}
	| COMPOPTS text CR {
			/* TODO: text verification and actions */
			verify_compopts();
			saveinput();
		}
	| CONTACT INTEG CR {
			/* TODO: actions */
                        SAW_PROP(P_CONTACT);
			contact_seen++;
			saveinput();
		}
	| CUSTOM STRING STRING INTEG INTEG INTEG text CR {
			/* TODO: actions */
                        SAW_PROP(P_CUSTOM);
			verify_custom();
			saveinput();
		}
	| DEVICE INTEG INTEG CR {
			/* TODO: verify text and actions */
                        SAW_PROP(P_DEVICE);
			verify_device_enum(1);
			saveinput();
		}
	| DEVICE INTEG INTEG text CR {
			/* TODO: verify text and actions */
                        SAW_PROP(P_DEVICE);
			verify_device_enum(1);
			saveinput();
		}
	| DISMESG INTEG text CR {
			/* TODO: verify text and actions */
                        SAW_PROP(P_DISMESG);
			verify_message_num(entries[1].ulong);
			saveinput();
		}
	| ENUMERATES INTEG INTEG INTEG INTEG CR {
                        SAW_PROP(P_ENUMERATES);
			verify_device_enum(2);
			saveinput();
		}
	| ENUMERATES INTEG INTEG INTEG INTEG text CR {
                        SAW_PROP(P_ENUMERATES);
			verify_device_enum(2);
			saveinput();
		}
	| INTBOPS INTEG INTEG INTEG INTEG INTEG CR {
			/* TODO: verify text and actions */
                        SAW_PROP(P_INT_BOPS);
			entries[entindex-5].type = INTBOPS;
			saveinput();
		}
	| LOCALE STRING CR {
			/* TODO: verify text and actions */
                        SAW_PROP(P_LOCALE);
			saveinput();
		}
	| MESGFILE STRING CR {
			/* TODO: verify text and actions */
                        SAW_PROP(P_MSGFSTART);
	                verify_filename();
			saveinput();
		}
	| MESSAGE INTEG text CR {
			/* TODO: actions */
                        SAW_PROP(P_MESSAGE);
			entries[0].type = MESSAGE;
			verify_message_num(entries[1].ulong);
			maketext(2);
			verify_attr_string(2);
			saveinput();
		}
	| META INTEG STRING CR {
			/* TODO: verify STRING and actions */
                        SAW_PROP(P_META);
			entries[entindex-2].type = META;
			saveinput();
		}
	| MODULE STRING CR {
			/* TODO: actions */
                        SAW_PROP(P_MODULE);
			module_seen++;
			verify_string(0);
			set_type_module();
			saveinput();
		}
	| MODULE STRING text CR {
			/* TODO: actions */
/*kwq*/			use_src = 1;
                        SAW_PROP(P_MODULE);
			module_seen++;
			verify_filename();
			set_type_module();
			saveinput();
		}
	| MULTIPARENT CR {
			/* TODO: actions */
                        SAW_PROP(P_MULTIPARENT);
			saveinput();
		}
	| NAME INTEG CR {
			/* TODO: actions */
                        SAW_PROP(P_NAME);
			name_seen++;
			saveinput();
		}
	| NSI INTEG CR {
			/* TODO: actions */
                        SAW_PROP(P_NSI);
			saveinput();
		}
	| NSI INTEG INTEG CR {
			/* TODO: actions */
                        SAW_PROP(P_NSI);
			saveinput();
		}
	| PARBOPS INTEG INTEG INTEG INTEG CR {
			/* TODO: verify STRING and actions */
                        SAW_PROP(P_PARENT_BOPS);
			entries[entindex-4].type = PARBOPS;
			saveinput();
		}
	| PIOSERLIMIT INTEG CR {
                        SAW_PROP(P_PIOSERLIMIT);
			verify_pioserlim();
			saveinput();
	}
	| PROPVERS HEX CR {
			/* TODO: actions */
                        if (!parsemfile) {
                                SAW_PROP(P_PROP_VER);
				prop_ver_seen++;
			} else {
                                SAW_PROP(P_PROP_VER);
				entries[0].type = MSGFPROPV;
                        }
			saveinput();
		}
	| PROVIDES STRING HEX CR {
                        SAW_PROP(P_PROVIDES);
			verify_provides();
			provides();
			saveinput();
		}
	| PROVIDES STRING HEX text CR {
                        SAW_PROP(P_PROVIDES);
			verify_provides();
			provides();
			saveinput();
		}
	| READFILE STRING CR {
			/* TODO: actions */
                        SAW_PROP(P_READABLE);
	                verify_filename();
			saveinput();
		}
	| REGION INTEG CR {
			/* TODO: verify text and actions */
                        SAW_PROP(P_REGION);
			saveinput();
		}
	| REGION INTEG text CR {
			/* TODO: verify text and actions */
                        SAW_PROP(P_REGION);
			verify_region();
			saveinput();
		}
	| RELEASE relnum text CR {
                        SAW_PROP(P_RELEASE);
			release_seen++;
			maketext(2);
			saveinput();
		}
	| REQUIRES STRING HEX CR { 
			/* TODO: verify STRING and actions */
                        SAW_PROP(P_REQUIRES);
			requires_seen++;
			requires();
			saveinput();
		}
	| REQUIRES STRING nohextext CR {
			/* This syntax is valid only for mapper. */
			/* TODO: verify STRING and actions */
			if (udi_global.make_mapper == 0 ) { YYERROR; }
                        SAW_PROP(P_REQUIRES);
			requires_seen++;
			saveinput();
		}
	| SHORTNAME STRING CR {
                        SAW_PROP(P_SHORTNAME);
			shortname_seen++;
			verify_string(8);
			set_type_shortname();
			saveinput();
		}
	| SRCFILES text CR {
			/* TODO: actions */
	                verify_filespec();
			saveinput();
		}
	| SRCREQS text CR {
			/* TODO: actions */
			saveinput();
		}
	| SUPPLIER INTEG CR {
			/* TODO: actions */
                        SAW_PROP(P_SUPPLIER);
			supplier_seen++;
			saveinput();
		}
	/*
	 * `mapper' is a meta token valid only for mappers.
	 * the syntax is completely OS dependent.
	 */
	| MAPPER text CR {
			if (udi_global.make_mapper == 0 ) { YYERROR; }
			saveinput();
		}
	| SYMBOLS text CR {
                        SAW_PROP(P_SYMBOLS);
			verify_symbols();
			saveinput();
		}
	| error {
			printloc(stderr, UDI_MSG_FATAL, 1200,
					"invalid declaration on line %d in "
					"file %s!\n",
					lineno-1, propfile);
		}
	;
relnum:	HEX
	| INTEG
	;

text:	text keytext
	| keytext
	;

keytext: nohexkeytext
	 | HEX
	 ;

nohextext: nohextext nohexkeytext
	   | nohexkeytext
	   ;

nohexkeytext: CATEGORY
	      | CFGCHOICE
	      | CHLDBOPS
              | COMPOPTS
	      | CONTACT
	      | CUSTOM
	      | DEVICE
	      | DISMESG
	      | ENUMERATES
	      | INTBOPS
	      | LOCALE
	      | MAPPER
	      | MESGFILE
	      | MESSAGE
	      | META
	      | MODULE
	      | MULTIPARENT
	      | NAME
	      | NSI
	      | PARBOPS
	      | PROPVERS
	      | PROVIDES
	      | READFILE
	      | REGION
	      | RELEASE
	      | REQUIRES
	      | SHORTNAME
	      | SRCFILES
	      | SRCREQS
	      | SUPPLIER
	      | SYMBOLS
	      | INTEG
	      | STRING
	      ;

%%

#include <ctype.h>
#include <errno.h>
#define UDI_VERSION 0x101
#include <udi.h>
#include "udi_sprops_ra.h"
 
#define ALLOCELS 256

int input_num;
int input_size;
int use_src;
int module_ent;
int release_ent;
int comp_ent;
int drv_xxx_idx, alt_yyy_idx, comp_zzz_idx;
char *propfile;


/* do_udiparse
 *
 * This is the main routine called to perform the read and parse of the
 * UDI properties file specified in the input argument (or 
 * udi_global.propfile if the input argument is NULL).
 *
 * Inputs:
 *    udi_global.propfile  || propfile arg
 *    udi_global.progname
 *
 * Outputs:
 *    udi_global.propdata
 *    udi_global.drv_xxx
 *    udi_global.alt_yyy
 *    udi_global.comp_zzz
 */

void
do_udiparse(char *propfile)
{
	int i;
	int pfalloc = 0;
	udi_boolean_t allow_silent_fail;

	if (propfile) {
		allow_silent_fail = FALSE;
	} else {
		allow_silent_fail = TRUE;
		propfile = optinit();
		optcpy(&propfile, udi_global.propfile);
		pfalloc = 1;
	}

	/*
	 * If this was used once already, we need to free the old stuff
	 *kwq: I don't think that's what this loop currently does... it
	 *     instead skips all current definitions, but will that cause
	 *     them to be scanned twice?
	if (input_data != NULL) { fprintf(stderr, "Warning: input_data exists\n"); }
	 */
	if (input_data != NULL) {
		i = 0;
		while (input_data[i] != NULL) {
			while (input_data[i] != NULL) {
				i++;
			}
			i++;
		}
	}
	prop_ver_seen = supplier_seen = contact_seen = name_seen =
			shortname_seen = release_seen = requires_seen =
			module_seen = region0_seen = provides_seen = 0;
        prop_seen  = 0;
	module_cnt = 0;
	module_ent = -1;
	input_num = 0;
	udi_global.rl_list = NULL;
	udi_global.rl_tail = NULL;
	udi_global.pl_list = NULL;
	udi_global.pl_tail = NULL;
	/* Allocate space for the parse of the static properties file */
	if ((input_data = malloc(sizeof(propline_t)*ALLOCELS)) == NULL) {
		printloc(stderr, UDI_MSG_FATAL, 1201, "Unable to allocate "
				"memory for the input table!\n");
	}
	input_size = ALLOCELS;

	/* Open up the file to parse, and parse it */
	yyin = fopen(propfile, "r");
	if (yyin == NULL) {
		if (!allow_silent_fail) {
			printloc(stderr, UDI_MSG_FATAL, 1202, 
				 "Cannot open %s!\n", propfile);
		}
		udi_global.propdata[0] = NULL;
		return;
	}
	parsemfile = 0;
	yyparse();

	entindex = 0;
	saveinput();

        END_CHK_PROP();
	if (prop_ver_seen != 1) {
		printloc(stderr, UDI_MSG_FATAL, 1203, "Must be exactly one "
				"'%s' declaration!\n", "properties_version");
	}
	if (supplier_seen != 1) {
		printloc(stderr, UDI_MSG_FATAL, 1203, "Must be exactly one "
				"'%s' declaration!\n", "supplier");
	}
	if (contact_seen != 1) {
		printloc(stderr, UDI_MSG_FATAL, 1203, "Must be exactly one "
				"'%s' declaration!\n", "contact");
	}
	if (name_seen != 1) {
		printloc(stderr, UDI_MSG_FATAL, 1203, "Must be exactly one "
				"'%s' declaration!\n", "name");
	}
	if (shortname_seen != 1) {
		printloc(stderr, UDI_MSG_FATAL, 1203, "Must be exactly one "
				"'%s' declaration!\n", "shortname");
	}
	if (release_seen != 1) {
		printloc(stderr, UDI_MSG_FATAL, 1203, "Must be exactly one "
				"'%s' declaration!\n", "release");
	}
	if (requires_seen < 1) {
		printloc(stderr, UDI_MSG_FATAL, 1204, "One or more '%s' "
				"declaration(s) required!\n", "requires");
	}
	if (!provides_seen && region0_seen != 1) {
		printloc(stderr, UDI_MSG_FATAL, 1205, "Missing 'region' "
				"declaration for region zero!\n");
	}

       /* Establish some defaults from the udiprops contents which may
        * be overridden by the user */

	optcpy(&udi_global.drv_xxx,  udi_global.propdata[drv_xxx_idx]->string);
	optcpy(&udi_global.alt_yyy,  udi_global.propdata[alt_yyy_idx]->string);
	optcpy(&udi_global.comp_zzz, udi_global.propdata[comp_zzz_idx]->string);

	if (pfalloc) {
		free(propfile);
		propfile = NULL;
	}
}

void
saveinput() {
	int i;

	startdec = 0;

	/*
	 * If we are just parsing message files, make sure we only have valid
	 * declarations for message files.
	 */
	if (parsemfile) {
		switch (entries[0].type) {
			case MSGFPROPV:
			case LOCALE:
			case MESSAGE:
			case DISMESG:
			case MSGFSTART:
				break;
			default:
				printloc(stderr, UDI_MSG_FATAL, 1200,
					"invalid declaration on line "
					"%d in file %s!\n",
					lineno-1, propfile);
		}
	}

	/* Make sure the first line is the static properties version */
	if (input_num == 0) {
		if (entries[0].type != PROPVERS) {
			printloc(stderr, UDI_MSG_FATAL, 1206, "the first "
					"non-comment line for the static "
					"properties was\n\tnot the "
					"properties_version line!\n");
		}
	        if (entries[1].ulong != 0x101) {
			printloc(stderr, UDI_MSG_ERROR, 1207, "invalid static "
					"properties version specified!\n");
			printloc(stderr, UDI_MSG_FATAL, 1208, "These tools "
					"support the following version(s): "
					"%s\n", VER_SUPPORT);
		}
	}

	/* For each element of a line, plus a null */
	for (i = 0; i <= entindex; i++) {
		if (input_size == input_num) {
			/* Allocate more space */
			input_size += ALLOCELS;
			if ((input_data = realloc(input_data, sizeof
					(propline_t)*input_size)) == NULL) {
				printloc(stderr, UDI_MSG_FATAL, 1209, "Unable "
						"to allocate more memory for "
						"the input table!\n");
			}
		}
		if (i != entindex) {
			if ((input_data[input_num] = malloc(sizeof(propline_t)))
					== NULL) {
				printloc(stderr, UDI_MSG_FATAL, 1210, "Unable "
						"to allocate memory for an "
						"entry!\n");
			}
			memcpy(input_data[input_num], &entries[i],
					sizeof(propline_t));
			/* Save pointers to certain strings */
			if (i == 0) {
				switch (input_data[input_num]->type) {
					case SHORTNAME:
					        drv_xxx_idx = input_num + 1;
						break;
					case RELEASE:
					        alt_yyy_idx = input_num + 1;
						break;
					case REGION:
						if (entries[1].ulong == 0) {
                                                        SAW_PROP(P_REGION);
							region0_seen++;
						}
						break;
					case MODULE:
						module[module_cnt] = input_num;
						module_ent = input_num;
						if (module_cnt == 0)
							comp_zzz_idx =
								input_num + 1;
						module_cnt++;
						break;
				        case PROVIDES:
                                                SAW_PROP(P_PROVIDES);
						provides_seen++;
						break;
					default:
						break;
				}
			}
		} else {
			input_data[input_num] = NULL;
		}
		input_num++;
	}
	entindex = 0;
}

void verify_message_num(unsigned long num)
{
	if (num == 0 || num > 65535) {
		printloc(stderr, UDI_MSG_FATAL, 1230, "invalid "
				"message number %d specified on line %d!\n",
				num, lineno-1);
	}
}

void verify_attr_string(int index)
{
	if (! verify_type(VSTRING, &entries[index])) {
		printloc(stderr, UDI_MSG_FATAL, 1213, "invalid "
				"attribute value (%s) for attribute\n"
				"type %s, specified on line %d!\n",
				entries[index+1].string,
				entries[index].string, lineno-1);
	}
}

void
maketext(int index)
{
	char *oldstr = entries[index].string;
	int totsize = 0;
	int i = index;

	/*
	 * Starting at the index indicated, concatenate the strings
	 * into a single, space separated string.
	 */

	/* First, get the total size for the concatenated string */
	while (i < entindex) {
		totsize += (int) strlen(entries[i].string) + 1;
		i++;
	}
	/* Allocate enough space for it */
	if ((entries[index].string = malloc(totsize))
			== NULL) {
		printloc(stderr, UDI_MSG_FATAL, 1211, "Unable to allocate "
				"memory for a string!\n");
	}
	/* Copy the pieces, freeing the old ones */
	strcpy(entries[index].string, oldstr);
	free(oldstr);
	i = index + 1;
	while (i < entindex) {
		strcat(entries[index].string, " ");
		strcat(entries[index].string, entries[i].string);
		free(entries[i].string);
		i++;
	}
	entindex = index + 1;
}

/*
 * Provided a type and a ptr to an entry, verifies the content of the entry for
 * that type.  Return 0 if invalid, otherwise 1.
 *   type = VSTRING:  string
 *   type = VUBIT32:  ubit32
 *   type = VBOOLEAN: boolean
 *   type = VARRAY:   array
 */
int
verify_type(int type, propline_t *ent)
{
	char *ptr, *ptr2;
	int valid;

	valid = 1;
	switch(type) {
	case VSTRING: /* string */
		/*
		 * Really, the only thing to check is to
		 * make sure any escape sequence is valid.
		 */
		ptr = ent->string;
		while (valid && *ptr) {
			if (valid && *ptr == '\\') {
				ptr2 = ptr+1;
				if (*ptr2 != '_' && *ptr2 != 'H' &&
						*ptr2 != '\\' && *ptr2 != 'p' &&
						*ptr2 != 'm')
					valid = 0;
				if (valid && *ptr2 == 'm' && (*(ptr2+1)<'1' ||
						*(ptr2+1)>'9'))
					valid = 0;
			}
			ptr++;
		}
		break;
	case VUBIT32: /* ubit32 */
		if (ent->type != INTEG && ent->type != HEX)
			valid = 0;
		break;
	case VBOOLEAN: /* boolean */
		if (strcmp(ent->string, "T") != 0 &&
				strcmp(ent->string, "F") != 0 &&
				strcmp(ent->string, "t") != 0 &&
				strcmp(ent->string, "f") != 0)
			valid = 0;
		break;
	case VARRAY: /* array */
		/* Must have pairs of hex digits */
		if (strlen(ent->string)%2)
			valid = 0;
		if (strlen(ent->string)/2 > UDI_MIN_INSTANCE_ATTR_LIMIT) {
			printloc(stderr, UDI_MSG_ERROR, 1232, "size of array "
					"exceeded %d characters!\n",
					UDI_MIN_INSTANCE_ATTR_LIMIT);
			valid = 0;
		}
		/* Verify we only have 0-9a-fA-F as characters */
		ptr = ent->string;
		while (valid && *ptr) {
			if ((*ptr >= '0' && *ptr <= '9') ||
					(*ptr >= 'a' && *ptr <= 'f') ||
					(*ptr >= 'A' && *ptr <= 'F')) {
				ptr++;
				continue;
			}
			valid = 0;
		}
		break;
	default: /* Unknown type passed in */
		valid = 0;
		break;
	}
	return (valid);
}

void
verify_string(size_t len) {	/* used to be called verify_shortname() */
	char *ptr = entries[1].string;
	int  p;

	if (len && (strlen(entries[1].string) > len)) {
		goto string_err;
	}
	/* All chars must be upper and lower case letters, digits, or _ */
	while (*ptr != '\0') {
		p = (int)(*ptr);
		if (isalpha(p) == 0 && isdigit(p) == 0 && *ptr != '_') {
			string_err:
			printloc(stderr, UDI_MSG_FATAL, 1212, "invalid "
					"string specified on line %d!\n",
					lineno-1);
		}
		ptr++;
	}
}


void
verify_filename()
{
	char *fname = entries[1].string;
	int  f;

	while (*fname != 0) {
		f = (int)(*fname);
		if (isalpha(f) == 0 &&
		    isdigit(f) == 0 &&
		    *fname != '_' &&
		    *fname != '.')  /* '/' excluded automatically */
			printloc(stderr, UDI_MSG_FATAL, 1228, "invalid "
				         "filename specified on line %d!\n",
				         lineno-1);
		fname++;
	}
}


void
verify_filespec()
{
	char *fname;
	int i;

	for (i = 1; i < entindex; i++) {
		fname = entries[i].string;
		while (*fname != 0) {
			int l = strlen(fname);
			if (strcmp(fname, " /") == 0 ||
			    l >= 64  ||
			    fname[l-2] != '.' ||
			    (fname[l-1] != 'c' && fname[l-1] != 'h' ) ||
			    strcmp(fname, "/../") == 0)
				printloc(stderr, UDI_MSG_FATAL, 1229, "invalid "
						 "filespec specified on "
						 "line %d!\n", lineno-1);
			fname++;
		}
	}
}


void set_type_shortname( void )
{
	entries[0].type = SHORTNAME;
}

void set_type_module( void )
{
	entries[0].type = MODULE;
}


/*
 * Verifies attributes of the device or enumerates declarations
 *	type = 1 for device declarations
 *	type = 2 for enumerates declarations
 */
void
verify_device_enum(int type)
{
	int index;

	if (type == 1) {
		if (entindex == 4) return;	/* No attributes case */
		index = 3;	/* Start at beginning of attributes */
	} else {
		if (entindex == 6) return;	/* No attributes case */
		index = 5;	/* Start at beginning of attributes */
	}
	while (index < entindex) {
		index++;
		if (strcmp(entries[index].string, "string") == 0) {
			entries[index].type = UDI_ATTR_STRING;
		} else if (strcmp(entries[index].string, "ubit32")
				== 0) {
			entries[index].type = UDI_ATTR_UBIT32;
			if (! verify_type(VUBIT32, &entries[index+1])) {
				printloc(stderr, UDI_MSG_FATAL, 1213, "invalid "
					"attribute value (%s) for attribute\n"
					"type %s, specified on line %d!\n",
					entries[index+1].string,
					entries[index].string, lineno-1);
			}
		} else if (strcmp(entries[index].string, "boolean")
				== 0) {
			entries[index].type = UDI_ATTR_BOOLEAN;
			if (! verify_type(VBOOLEAN, &entries[index+1])) {
				printloc(stderr, UDI_MSG_FATAL, 1213, "invalid "
					"attribute value (%s) for attribute\n"
					"type %s, specified on line %d!\n",
					entries[index+1].string,
					entries[index].string, lineno-1);
			}
		} else if (strcmp(entries[index].string, "array")
				== 0) {
			entries[index].type = UDI_ATTR_ARRAY8;
			if (! verify_type(VARRAY, &entries[index+1])) {
				printloc(stderr, UDI_MSG_FATAL, 1213, "invalid "
					"attribute value (%s) for attribute\n"
					"type %s, specified on line %d!\n",
					entries[index+1].string,
					entries[index].string, lineno-1);
			}
		} else {
			printloc(stderr, UDI_MSG_FATAL, 1214, "invalid "
					"attribute type (%s) specified on "
					"line %d!\n", entries[index].string,
					lineno-1);
		}
		index += 2;
	}
}
void
verify_region()
{
	int cnt;
	int fnd;
	int origtype;

	cnt = 2;
	while (cnt+1 < entindex) {
		origtype = entries[cnt+1].type;
		entries[cnt+1].type = 0;
		if (strcmp(entries[cnt].string, "type") == 0) {
			cnt++;
			fnd = 0;
			entries[cnt].type = 0;
			if (strcmp(entries[cnt].string, "normal") == 0) {
				entries[cnt].type = _UDI_RA_NORMAL;
				fnd++;
			}
			if (strcmp(entries[cnt].string, "fp") == 0) {
				entries[cnt].type = _UDI_RA_FP;
				fnd++;
			}
			if (strcmp(entries[cnt].string, "interrupt")
					== 0) {
				entries[cnt].type = _UDI_RA_INTERRUPT;
				fnd++;
			}
			if (!fnd) {
				printloc(stderr, UDI_MSG_FATAL, 1215, "invalid "
						"region attribute value (%s) "
						"near line %d!\n",
						entries[cnt].string, lineno-1);
			}
		} else if (strcmp(entries[cnt].string, "binding") == 0) {
			cnt++;
			fnd = 0;
			entries[cnt].type = 0;
			if (strcmp(entries[cnt].string, "static") == 0) {
				entries[cnt].type = _UDI_RA_STATIC;
				fnd++;
			}
			if (strcmp(entries[cnt].string, "dynamic") == 0) {
				entries[cnt].type = _UDI_RA_DYNAMIC;
				fnd++;
			}
			if (! fnd) {
				printloc(stderr, UDI_MSG_FATAL, 1215, "invalid "
						"region attribute value (%s) "
						"near line %d!\n",
						entries[cnt].string,
						lineno-1);
			}
		} else if (strcmp(entries[cnt].string, "priority") == 0) {
			cnt++;
			fnd = 0;
			entries[cnt].type = 0;
			if (strcmp(entries[cnt].string, "lo") == 0) {
				entries[cnt].type = _UDI_RA_LO;
				fnd++;
			}
			if (strcmp(entries[cnt].string, "med") == 0) {
				entries[cnt].type = _UDI_RA_MED;
				fnd++;
			}
			if (strcmp(entries[cnt].string, "hi") == 0) {
				entries[cnt].type = _UDI_RA_HI;
				fnd++;
			}
			if (! fnd) {
				printloc(stderr, UDI_MSG_FATAL, 1215, "invalid "
						"region attribute value (%s) "
						"near line %d!\n",
						entries[cnt].string,
						lineno-1);
			}
		} else if (strcmp(entries[cnt].string, "latency") == 0) {
			cnt++;
			fnd = 0;
			entries[cnt].type = 0;
			if (strcmp(entries[cnt].string, "powerfail_warning")
					== 0) {
				entries[cnt].type = _UDI_RA_POWER_WARN;
				fnd++;
			}
			if (strcmp(entries[cnt].string, "overrunable") == 0) {
				entries[cnt].type = _UDI_RA_OVERRUN;
				fnd++;
			}
			if (strcmp(entries[cnt].string, "retryable") == 0) {
				entries[cnt].type = _UDI_RA_RETRY;
				fnd++;
			}
			if (strcmp(entries[cnt].string, "non_overrunable")
					== 0) {
				entries[cnt].type = _UDI_RA_NON_OVERRUN;
				fnd++;
			}
			if (strcmp(entries[cnt].string, "non_critical") == 0) {
				entries[cnt].type = _UDI_RA_NON_CRITICAL;
				fnd++;
			}
			if (! fnd) {
				printloc(stderr, UDI_MSG_FATAL, 1215, "invalid "
						"region attribute value (%s) "
						"near line %d!\n",
						entries[cnt].string,
						lineno-1);
			}
		} else if (strcmp(entries[cnt].string, "overrun_time") == 0) {
			cnt++;
			entries[cnt].type = origtype;
			if (! verify_type(VUBIT32, &entries[cnt])) {
				printloc(stderr, UDI_MSG_FATAL, 1215, "invalid "
						"region attribute value (%s) "
						"near line %d!\n",
						entries[cnt].string,
						lineno-1);
			}
			if (entries[cnt].ulong == 0)
				printloc(stderr, UDI_MSG_FATAL, 1231, "invalid "
						"value for region attribute "
						"overrun_time (%d), near line "
						"%d!\n",
						entries[cnt].ulong,
						lineno-1);
		} else if (strcmp(entries[cnt].string, "pio_probe") == 0) {
			cnt++;
			fnd = 0;
			entries[cnt].type = 0;
			if (strcmp(entries[cnt].string, "no") == 0) {
				fnd++;
			}
			if (strcmp(entries[cnt].string, "yes") == 0) {
				entries[cnt].type = _UDI_RA_PIO_PROBE;
				fnd++;
			}
			if (! fnd) {
				printloc(stderr, UDI_MSG_FATAL, 1215, "invalid "
						"region attribute value (%s) "
						"near line %d!\n",
						entries[cnt].string,
						lineno-1);
			}
		} else {
			printloc(stderr, UDI_MSG_FATAL, 1216, "invalid region "
					"attribute (%s) near line %d!\n",
					entries[cnt].string, lineno-1);
		}
		cnt++;
	}
	if (cnt != entindex) {
		/* Uneven reg_attr/value pair */
		printloc(stderr, UDI_MSG_FATAL, 1217, "region attribute "
				"provided with no value (%s) near line %d!\n",
				entries[cnt].string, lineno-1);
	}
}

void
verify_symbols()
{
	int error;

	error = 0;
	if (entindex == 4) {
		if (strcmp(entries[2].string, "as") != 0)
			error++;
	} else if (entindex != 2)
		error++;

	if (error) {
		printloc(stderr, UDI_MSG_FATAL, 1218, "invalid symbols "
				"declaration near line %d!\n", lineno-1);
	}
}

void
verify_compopts()
{
	/* Verify that we only have -D<name>, -D<name>=<token>, -U<name> */
	char *opt;
	int i;
	udi_boolean_t error = FALSE;

	/* Mappers may (and probably do) need to specifiy "illegal" flags. */
	if (udi_global.make_mapper == 1 ) {
		return;
	}

	for (i = 1; i < entindex; i++) {
		opt = entries[i].string;

		error |= opt[0] != '-';
		error |= (opt[1] != 'D') && (opt[1] != 'U');

		if (error) {
			printloc(stderr, UDI_MSG_FATAL, 1244,
				 "invalid compile_option `%s' "
				 "at line %d)\n",
				 opt, lineno-1);
		}
	}
}

void
verify_choices()
{
	int idx, type = 0;

	if (entries[0].type == CUSTOM)
		idx = 6;
	else
		idx = 2; /* config_choices */

	/* You can not have none */
	if (idx == entindex) {
		printloc(stderr, UDI_MSG_FATAL, 1219, "no choices specified "
			"for a custom or config_choices declaration near "
			"line %d!\n", lineno-1);
	}

	/* Verify validity */
	while (idx < entindex) {
		/* Verify an attr_type */
		if (strcmp(entries[idx].string, "string") == 0)
			type = VUBIT32;
		else if (strcmp(entries[idx].string, "ubit32") == 0)
			type = VUBIT32;
		else if (strcmp(entries[idx].string, "boolean") == 0)
			type = VBOOLEAN;
		else if (strcmp(entries[idx].string, "array") == 0)
			type = VARRAY;
		else {
			printloc(stderr, UDI_MSG_FATAL, 1220, "invalid "
				"attr_type (%s) in the declaration near line "
				"%d!\n", entries[idx].string, lineno-1);
		}
		idx++;

		/* Verify the default_value */
		if (! verify_type(type, &entries[idx])) {
		    	printloc(stderr, UDI_MSG_FATAL, 1228,
				"mismatch of default value with "
				"attribute type near "
		    		"line %d!\n", lineno-1);
		}
		idx++;

		if (idx < entindex) {
		    /* Verify the choice type and choices within */
		    if (strcmp(entries[idx].string, "any") == 0) {
			idx++;
		    } else if (strcmp(entries[idx].string, "only") == 0) {
			idx++;
		    } else if (strcmp(entries[idx].string, "mutex") == 0) {
			idx++;
			if (strcmp(entries[idx].string, "end") == 0) {
			    printloc(stderr, UDI_MSG_FATAL, 1221, "no values "
				    "were used with range in mutex declaration "
				    "near line %d!\n", lineno-1);
			}
			while (strcmp(entries[idx].string, "end") != 0) {
				if (! verify_type(type, &entries[idx])) {
			    		printloc(stderr, UDI_MSG_FATAL, 1222,
						"mismatch of value type with "
						"mutex in declaration near "
				    		"line %d!\n", lineno-1);
				}
				idx++;
			}
			idx++;
		    } else if (strcmp(entries[idx].string, "range") == 0) {
			if (type != VUBIT32) {
			    printloc(stderr, UDI_MSG_FATAL, 1223, "invalid "
				    "attr_type was used with range in custom "
				    "declaration near line %d!\n", lineno-1);
			}
			idx++;
			/* Verify range */
			if (verify_type(VUBIT32, &entries[idx]) &&
					verify_type(VUBIT32, &entries[idx+1]) &&
					verify_type(VUBIT32, &entries[idx+2])) {
				if (entries[idx].ulong > entries[idx+1].ulong) {
					printloc(stderr, UDI_MSG_FATAL, 1224,
						"min_value was greater than "
						"max_value in custom "
						"declaration near line %d!\n",
						lineno-1);
				}
			} else {
			    printloc(stderr, UDI_MSG_FATAL, 1225, "invalid "
						"range was defined in custom "
						"declaration near line %d!\n",
						lineno-1);
			}
			idx += 3;
		    }
		}
		if (entries[0].type == CUSTOM) {
		    if (idx == entindex-1) {
			/* Last thing is a device <msgnum> */
			/* FIXME: allow <msgnum> to be non-zero */
			if ((!verify_type(VUBIT32, &entries[idx])) ||
				entries[idx].ulong != 0)
			    printloc(stderr, UDI_MSG_FATAL, 1226, "invalid "
						"<msgnum> declared in custom "
						"declaration near line %d!\n",
						lineno-1);
		    } else {
			/* Invalid number of parameters passed */
			printloc(stderr, UDI_MSG_FATAL, 1235, "invalid number "
					"of parameters declared in custom "
					"declaration near line %d!\n",
					lineno-1);
		    }
	    	    break;	/* Only 1 for custom */
		}
	}
}

void
verify_custom()
{
	/* TODO: Verify attr_name */
	/* Verify scope */
	if (strcmp(entries[2].string, "device") != 0 &&
			strcmp(entries[1].string, "device_optional") != 0 &&
			strcmp(entries[1].string, "driver") != 0 &&
			strcmp(entries[1].string, "driver_optional") != 0) {
		printloc(stderr, UDI_MSG_FATAL, 1227, "invalid scope (%s) for "
				"a custom declaration near line %d!\n",
				entries[2].string, lineno-1);
	}
	/* Verify choices */
	verify_choices();
}

void
verify_pioserlim()
{
	/* bounds check that 0<= SERLIMIT <= 255 */
	if (verify_type(VUBIT32, &entries[1])) {
		if (entries[1].ulong < 0 || entries[1].ulong > 255) {
			printloc(stderr, UDI_MSG_FATAL, 1233, "invalid "
				"value (%s) for declaration type %s, specified "
				"on line %d!\nMust be in the range of 0 - 255."
				"\n", entries[1].string, entries[0].string,
				lineno-1);
		}
	} else {
		printloc(stderr, UDI_MSG_FATAL, 1234, "invalid "
				"value (%s) for declaration\n"
				"type %s, specified on line %d!\n",
				entries[1].string,
				entries[0].string, lineno-1);
	}
}

void 
verify_provides()
{
	int i = 0;
	char *ptr = entries[1].string;

	/* Validate interface name */
	while (*(ptr+i) != 0) {
		if (! (isalnum(*(ptr+i)) || *(ptr+i) == '_' || i != 35)) {
			/* Invalid interface name */
			printloc(stderr, UDI_MSG_FATAL, 1236, "invalid "
				"interface name (%s) for provides declaration "
				"on line %d!\n", entries[1].string, lineno-1);
		}
		i++;
	}

	/* Validate version number */
	if (*(entries[2].string) != '0' || *(entries[2].string+1) != 'x' ||
			!verify_type(VUBIT32, &entries[2]) ||
			strlen(entries[2].string) > 6) {
		/* Invalid interface name */
		printloc(stderr, UDI_MSG_FATAL, 1237, "invalid version_number "
				"(%s) for provides declaration on line %d!\n",
				entries[1].string, lineno-1);
	}
}

static 
void
requires(void)
{
	requires_line_t *rl = calloc(1, sizeof *rl);

	if (rl == NULL) {
		/*
	 	 * Borrow error text from above to reduce 
		 * internationalization costs.
		 */
		printloc(stderr, UDI_MSG_FATAL, 1210, "Unable "
				"to allocate memory for an "
				"entry!\n");
	}

	/* 
	 * First on list?
	 */
	if (udi_global.rl_list == NULL)
		udi_global.rl_list = rl;
	/*
	 * rl_tail always points to the last valid member.
	 * Thus rl_tail->rl_next is always NULL once we
	 * stitch the new one in.
	 */
	if (udi_global.rl_tail) 
		udi_global.rl_tail->rl_next = rl;
	udi_global.rl_tail = rl;

	/*
	 * This could attempt to exclude things that are integrated
	 * into the core (in the implementation modularity sense, not 
	 * the spec sense) environment.    For now, we add them all just 
	 * for consistency.
 	 */
	rl->rl_meta = entries[1].string;
	rl->rl_version = strtoul(&entries[2].string[2], NULL, 16);
}

static 
void
provides(void)
{
	provides_line_t *pl;
	int nprovides = 0;
	int i;

	nprovides = entindex - 3;

	pl = calloc(1, sizeof *pl + sizeof (char *) * nprovides);

	if (pl == NULL) {
		/*
	 	 * Borrow error text from above to reduce 
		 * internationalization costs.
		 */
		printloc(stderr, UDI_MSG_FATAL, 1210, "Unable "
				"to allocate memory for an "
				"entry!\n");
	}

	/* 
	 * First on list?
	 */
	if (udi_global.pl_list == NULL)
		udi_global.pl_list = pl;
	/*
	 * pl_tail always points to the last valid member.
	 * Thus pl_tail->pl_next is always NULL once we
	 * stitch the new one in.
	 */
	if (udi_global.pl_tail) 
		udi_global.pl_tail->pl_next = pl;
	udi_global.pl_tail = pl;

	/*
	 * This could attempt to exclude things that are integrated
	 * into the core (in the implementation modularity sense, not 
	 * the spec sense) environment.    For now, we add them all just 
	 * for consistency.
 	 */
	pl->pl_interface_name = entries[1].string;
	pl->pl_version = strtoul(&entries[2].string[2], NULL, 16);
	pl->pl_num_includes = nprovides;
	for (i = 0; i < nprovides; i++) {
		pl->pl_include_file[i] = entries[3+i].string;
	}
	pl->pl_include_file[i] = NULL;
}



static char prop_p_name[][25] = {
        "properties_version", "supplier", "contact", "name",
        "shortname", "release", "requires", "module",
        "region", "provides", "symbols", "category",
        "config_choices", "child_bind_ops",
          "parent_bind_ops", "internal_bind_ops",
        "multi_parent", "locale", "message", "disaster_message",
        "message_file", "meta", "enumerates", "device",
        "custom", "pio_serialization_limit", "readable_file",
          "nonsharable_interrupt",
        "Message File", "Library", "Driver", "Oops!" 
};

        
static void
prop_p_names(char **optstr, unsigned long propmask)
{
        int jj, kk = 0;

        for (jj = 0; jj < 32; jj++)
                if (propmask & (1<<jj)) {
                        if (kk) {
                                optcat(optstr, ", ");
                                optcat(optstr, prop_p_name[jj]);
                        } else {
                                optcpy(optstr, prop_p_name[jj]);
                        }
                }
}


static void
check_prop(unsigned long new_prop)
{
        int ii;
        char *currp, *oldp;
        unsigned long old_prop;

        for (ii=0; prop_comb[ii].if_present; ii++) {
                old_prop = prop_comb[ii].if_present & prop_seen;
                if (old_prop && !(prop_comb[ii].allow & new_prop)) {
                        currp = optinit();
                        oldp = optinit();
                        prop_p_names(&currp, new_prop);
                        prop_p_names(&oldp,  old_prop);
                        if (old_prop & PTYPE_MASK) {
                                printloc(stderr, UDI_MSG_FATAL, 1242,
                                         "`%s' declaration not allowed "
                                         "in a %s (@ line %d)\n",
                                         currp, oldp, lineno-1);
                        } else if (new_prop & old_prop) {
                                printloc(stderr, UDI_MSG_FATAL, 1243,
                                         "multiple `%s' declarations not "
                                         "allowed (@ line %d)\n",
                                         currp, lineno-1);
                        } else {
                                printloc(stderr, UDI_MSG_FATAL, 1240,
                                         "`%s' declaration not allowed with "
                                         "`%s' @ line %d\n",
                                         currp, oldp, lineno-1);
                        }
                }
        }
        
        prop_seen |= new_prop;

        for (ii=0; ii < sizeof(ptype_set) / sizeof(ptype_set[0]); ii++) {
                if (new_prop & ptype_set[ii].p_mask) {
                        if (((prop_seen & PTYPE_MASK) &
                             ~ptype_set[ii].ptype) != 0) {
                                currp = optinit();
                                oldp = optinit();
                                prop_p_names(&currp, new_prop);
                                prop_p_names(&oldp, (prop_seen &
                                                     PTYPE_MASK));
                                printloc(stderr, UDI_MSG_FATAL, 1241,
                                         "`%s' declaration invalid "
                                         "for %s @ line %d\n",
                                         currp, oldp, lineno-1);
                        }
                        prop_seen |= ptype_set[ii].ptype;
                }
        }
}
