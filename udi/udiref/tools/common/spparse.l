%{
/*
 * File: tools/common/spparse.l
 *
 * UDI Static Properties Parser Lex File
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
#include <string.h>
#include <nl_types.h>
#include "common.h"
#include "y.tab.h"
/* y.tab.h doesn't protect itself against multiple inclusion so we have to */
#define _y_tab_h
#include "global.h"
#include "spparse.h"

extern nl_catd nlcat;

/* Used for typecasting to satisfy C lib calls */
#define _CCP  (const char *)
%}

 /*
  * Definitions required for some lex implementations. Allow for at least
  * 4000 positions (%p) and 3000 transitions (%a)
  */
%p4000
%a 3000

legalchar	[\!\"\#\$\%\&\'\(\)\*\+\,\-\.\/0-9\:\;\<\=\>\?\@A-Z\[\\\]\^\_\`a-z\{\|\}\~]
utf8char	[\x80\xe0\xf0\x00\x3f]

%%
[^ \t\n]*\\[^_H\\pm\n]{1}[^\n]*\n	{endofline(1);}/* line continuation */
[^ \t\n]*\\\n		{endofline(1);}/* line continuation */
#.*			;		/* ignore comments */
[ \t]+$			;		/* ignore whitespace at end of line */
^[ \t]+			;		/* ignore beg. of line whitespace */
^\n			{endofline(0); return CR;}	/* ignore empty lines */
[ \t]+			linesize++;	/* ignore white space */
\n			{endofline(0); return CR;}
"category"		{
				return(rettoken (CATEGORY));
			}
"config_choices"	{
				return(rettoken (CFGCHOICE));
			}
"child_bind_ops"	{
				return(rettoken (CHLDBOPS));
			}
"compile_options"	{
				return(rettoken (COMPOPTS));
			}
"contact"		{
				return(rettoken (CONTACT));
			}
"custom"		{
				return(rettoken (CUSTOM));
			}
"device"		{
				return(rettoken (DEVICE));
			}
"disaster_message"	{
				return(rettoken (DISMESG));
			}
"enumerates"		{
				return(rettoken (ENUMERATES));
			}
"internal_bind_ops"	{
				return(rettoken (INTBOPS));
			}
"locale"		{
				return(rettoken (LOCALE));
			}
"mapper"		{
				return(rettoken (MAPPER));
			}
"message_file"		{
				return(rettoken (MESGFILE));
			}
"message"		{
				return(rettoken (MESSAGE));
			}
"meta"			{
				return(rettoken (META));
			}
"module"		{
				return(rettoken (MODULE));
			}
"multi_parent"		{
				return(rettoken (MULTIPARENT));
			}
"name"			{
				return(rettoken (NAME));
			}
"nonsharable_interrupt"	{
				return(rettoken (NSI));
			}
"parent_bind_ops"	{
				return(rettoken (PARBOPS));
			}
"pio_serialization_limit" {
				return(rettoken (PIOSERLIMIT));
			}
"properties_version"	{
				return(rettoken (PROPVERS));
			}
"provides"		{
				return(rettoken (PROVIDES));
			}
"readable_file"		{
				return(rettoken (READFILE));
			}
"region"		{
				return(rettoken (REGION));
			}
"release"		{
				return(rettoken (RELEASE));
			}
"requires"		{
				return(rettoken (REQUIRES));
			}
"shortname"		{
				return(rettoken (SHORTNAME));
			}
"source_files"		{
				return(rettoken (SRCFILES));
			}
"source_requires"	{
				return(rettoken (SRCREQS));
			}
"supplier"		{
				return(rettoken (SUPPLIER));
			}
"symbols"		{
				return(rettoken (SYMBOLS));
			}
[0-9]+			{
				entries[entindex].type = INTEG;
				entries[entindex].string = strdup(_CCP yytext);
				entries[entindex].ulong = strtoul(_CCP yytext, NULL, 10);
				entindex++;
                                linesize += strlen(_CCP yytext);
				return INTEG;
			}
0x[0-9A-Fa-f]+		{
				entries[entindex].type = HEX;
				entries[entindex].string = strdup(_CCP yytext);
				entries[entindex].ulong = strtoul(_CCP yytext, NULL, 16);
				entindex++;
                                linesize += strlen(_CCP yytext);
				return HEX;
			}
{legalchar}+		{
				entries[entindex].type = STRING;
				entries[entindex].string = strdup(_CCP yytext);
				entindex++;
                                linesize += strlen(_CCP yytext);
				return STRING;
			}
.			yyerror(catgets(nlcat, 1, 1100, "invalid character"));
%%

#include "parser.c"
