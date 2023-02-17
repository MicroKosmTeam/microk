/*
 * Files: tools/common/parser.c
 * 
 * UDI Static Properties Parser Support
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

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#ifndef _y_tab_h
#include "y.tab.h"
#define _y_tab_h
#endif
#include "global.h"
#include "osdep.h"
#include "spparse.h"


/*
 * Global Variables
 */
 
propline_t entries[512];

int lineno = 1;
int linesize = 0;
int entindex = 0;
int startdec = 0;


/*
 * General support for Yacc/Lex/Def Parser
 */

void
yyerror(char *s)
{
	printloc(stderr, UDI_MSG_ERROR, 1101, "line %d: %s at '%s'\n",
		         lineno, s, yytext);
}

int
rettoken(int token)
{
	entries[entindex].string = strdup((const char *)yytext);
	linesize += strlen((const char*)yytext);
	entindex++;
	if (! startdec) {
		entries[entindex-1].type = token;
		startdec++;
		return (token);
	}
	entries[entindex-1].type = STRING;
	return(STRING);
}


void
endofline(int is_continued)
{
    char *last, *bptr, *cptr;

    if (linesize > 512) 
	    printloc(stderr, UDI_MSG_FATAL, 1102, "line %d: too long "
		             "(%d > 512 chars)\n",
		             lineno, linesize);
    if (is_continued) {
	/* Remove the backslash and whatever follows, and unget to reparse */
	last = NULL;
	bptr = yytext;
	cptr = bptr + strlen(bptr) - 1;
	while (cptr >= bptr) {
		if (*cptr == '\\' && (*(cptr+1) == ' ' || *(cptr+1) == '\n'))
			last = cptr;
		cptr--;
	}
	if (last) {
		cptr = last - 1;
		while (cptr >= bptr) {
			unput(*cptr);
			cptr--;
		}
	}
    } else {
	linesize = 0;
    }
    lineno++;
}


/*
 * `Def'ault Parser.  Use this if you don't have Lex or it's brethren
 * available (e.g. MS Windows)
 */

#ifdef OS_HAS_NO_LEX

#include <stdlib.h>

FILE *yyin;                  /* Input file */
int   yylvalue;              /* Current token's semantic value */
int   yychar = 0;            /* Current character value? */
unsigned char yytext[512];   /* Current token's input value */
int   saw_nl = 0;            /* Memory for Newline: whitespace and token */


/* Very basic input tokenizer; compresses whitespace, suppresses comments,
   and returns tokens for the parser. */

int yylex(void) 
{
    int c, ii;

    /* Newline may have terminated the previous line, but newline is a
       token itself, so we remembered it.  Here we check our memory
       and return the newline token if needed. */

    if (saw_nl == '\n') {
	saw_nl = 0;
	endofline(0);
	return CR;
    }

    /* Skip whitespace */

 ws_skip:
    c = ' ';
    ii = 0;
    while (c == ' ' || c == '\t') { c = getc(yyin); linesize++; }
    if (c == EOF) return 0;

    /* Read a word, terminating with whitespace, comment, or newline */

    while (c != ' ' && c != '\t' && c != '#' && c != EOF) {
	yytext[ii] = c;
	if (c == '\n') {
	    if (ii > 0 && yytext[ii-1] == '\\') {
		endofline(1);
		if (ii == 1) {
		    goto ws_skip;
		}
		ii -= 3;
	    } else {
		saw_nl = '\n';
		break;
	    }
	}
	ii++;
	c = getc(yyin);
    }
    yytext[ii] = 0;

    if (c == '#')    /* strip comment to EOL */
	while (c != '\n') c = getc(yyin);

    if (ii == 0) {  /* blank line, just return newline token */
	saw_nl = 0;
	endofline(0);
	return CR;
    }

    /* Figure out a token value for the input read */


#define Token(String, Tokenval) \
              if (strcmp(yytext, String)==0) return(rettoken(Tokenval));

    Token("category", CATEGORY);
    Token("config_choices", CFGCHOICE);
    Token("child_bind_ops", CHLDBOPS);
    Token("compile_options", COMPOPTS);
    Token("contact", CONTACT);
    Token("custom", CUSTOM);
    Token("device", DEVICE);
    Token("disaster_message", DISMESG);
    Token("enumerates", ENUMERATES);
    Token("internal_bind_ops", INTBOPS);
    Token("locale", LOCALE);
    Token("message_file", MESGFILE);
    Token("message", MESSAGE);
    Token("meta", META);
    Token("module", MODULE);
    Token("multi_parent", MULTIPARENT);
    Token("name", NAME);
    Token("nonsharable_interrupt", NSI);
    Token("parent_bind_ops", PARBOPS);
    Token("properties_version", PROPVERS);
    Token("provides", PROVIDES);
    Token("readable_file", READFILE);
    Token("region", REGION);
    Token("release", RELEASE);
    Token("requires", REQUIRES);
    Token("shortname", SHORTNAME);
    Token("source_files", SRCFILES);
    Token("source_requires", SRCREQS);
    Token("supplier",  SUPPLIER);
    Token("symbols", SYMBOLS);

    if (yytext[0] == '0' && yytext[1] == 'x' &&
	((yytext[2] >= '0' && yytext[2] <= '9') ||
	 (yytext[2] >= 'a' && yytext[2] <= 'f') ||
	 (yytext[2] >= 'A' && yytext[2] <= 'F'))) {
	entries[entindex].type = HEX;
	linesize += strlen((const char*)yytext);
	entries[entindex].string = strdup((const char *)yytext);
	entries[entindex].ulong = strtoul(yytext, NULL, 16);
	entindex++;
	return HEX;
    }

    if (yytext[0] >= '0' && yytext[0] <= '9') {
	entries[entindex].type = INTEG;
	linesize += strlen((const char*)yytext);
	entries[entindex].string = strdup((const char *)yytext);
	entries[entindex].ulong = strtoul(yytext, NULL, 10);
	entindex++;
	return INTEG;
    }

    entries[entindex].type = STRING;
    linesize += strlen((const char*)yytext);
    entries[entindex].string = strdup((const char *)yytext);
    entindex++;
    return STRING;
}
#endif
