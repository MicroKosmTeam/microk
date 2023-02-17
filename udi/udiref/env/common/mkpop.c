
/* 
 * File: env/common/mkpop.c
 *
 * Create a PIO ops header.
 *
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
 * This file reads a system-specific pio_pops.h to create an osdep_pops.h.
 * It reads a simplistic template language to mechanically generate an 
 * array of backend functions (mem vs. io, 8-bit vs. 32-bit, paced vs. 
 * not, etc.) that the PIO engine can then use to effeciently access a given
 * PIO mapping.
 *
 * The syntax of this file can, unfortunately, not express every concievable
 * combination of PIO operations handler.   It does, however, allow the 
 * common cases and a moderately simple way to override the defaults for 
 * each case.
 *
 * The code tends to be a little mind-numbing to read but it's still more
 * flexible than a pure solution with the C preprocessor.
 *
 * The templates tend to look funny to humans but once optimized, they
 * generate very good code.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/*
 * Size of the _udi_pop_t read and write arrays.
 */
#define NTABLE_ENTRIES 6

typedef struct pop_op_template {
	char *template;
	char *template_name;
} pop_op_template_t;

typedef struct pop_action_template {
	pop_op_template_t *pop_proto;
	pop_op_template_t *pop_op;
} pop_action_template_t;

/* 
 * If this flag is set, individual POPS are not generated for each 
 * of the attributes.  So you will not get _swapped_, _paced_, 
 * _swapped_paced_ versions of each.   So your template must manually
 * look into the pio_handle for pacing, swapping, etc. and handle them.
 * This is a trade-off of code size & efficiency.   
 */
#define AT_FLAG_SIMPLE_ONLY 		0x0001

/* 
 * If this flag is set, individual POPS are generated for the various
 * word sizes (think atomicity) but the "many" cases must still manually
 * be as above and look into the PIO handle and handle attributes.
 */
#define AT_FLAG_SIMPLE_MANY_ONLY 	0x0002

typedef struct all_action_template {
	pop_action_template_t *pop_action;
	char *all_name;
	int at_flags;
} all_action_template_t;

/*
 *   Pick up the OS-dependent tables that tell us how to generate
 *   the C code to represent the various ops. 
 */

#include "pio_pops.h"


char *sizes[] = {
	"udi_ubit8_t", "",
	"udi_ubit16_t", "_OSDEP_ENDIAN_SWAP_16",
	"udi_ubit32_t", "_OSDEP_ENDIAN_SWAP_32",
#if defined (_OSDEP_UDI_UBIT64_T)
	"udi_ubit64_t", "_OSDEP_ENDIAN_SWAP",
#endif
	NULL, NULL
};

char *pauses[] = {
	"",
	pause_proto,
	NULL
};

/*
 * Legend of substituted variables. 
 * %a - address (offset into this  mapping) to write into . 
 * %d - data to read or write. 
 * %p - function name prefix. 
 * %t - udi_ubit_XX_t associated with this size. 
 * %o - operator (endian swapper). 
 */

void
substitute(char *proto,
	   char *prefix,
	   char *type,
	   char *swap_op,
	   char *operator,
	   char *pause_op,
	   char *host_mode)
{
	char c;

	while ((c = *proto++)) {
		switch (c) {
		case '%':
			switch (c = *proto++) {
			case '%':
				putchar(c);
				continue;
			case 'p':
				printf("%s", prefix);
				if (pause_op && pause_op[0])
					printf("_paced");
				if (operator)
					printf("_swapped");
				if (host_mode)
					printf("%s", host_mode);
				continue;
			case 't':
				printf("%s", type);
				continue;
			case 'a':
				printf(" offset ");
				continue;
			case 'd':
				if (*host_mode) {
					printf("*(%s *) data", type);
					continue;
				}
				printf("*(_OSDEP_PIO_LONGEST_STYPE *) data");
				continue;
			case 'o':
				if (operator)
					printf("%s", operator);
				continue;
			case 's':
				substitute(swap_op, NULL, type,
					   NULL, operator, pause_op,
					   host_mode);
				continue;
			case 'P':	/*
					 * Pause 
					 */
				substitute(pause_op, NULL, type, NULL,
					   operator, NULL, host_mode);
				continue;
			default:
				putchar(toupper(c));
			}
		default:
			printf("%c", c);
		}
	}
}

void
loop_substitute(int at_flags,
		char *proto,
		char *prefix,
		char *type,
		pop_op_template_t * op,
		char *pause_op)
{
	char **s;
	pop_op_template_t *orig_op = op;

	char *table[] = { "", "_dir", NULL };
	char **tp = table;

	for (tp = table; *tp; tp++) {
		op = orig_op;
		for (s = &sizes[0]; *s; s += 2) {
			substitute(proto, prefix, s[0], op->template, NULL,
				   pause_op, *tp);
			op++;
		}
	}

	/*
	 * Now generate a byte swapped version of all the sizes if requested. 
	 */

	if (at_flags & AT_FLAG_SIMPLE_ONLY)
		return;

	for (tp = table; *tp; tp++) {
		op = orig_op;
		for (s = &sizes[0]; *s; s += 2) {
			substitute(proto, prefix, s[0], op->template, s[1],
				   pause_op, *tp);
			op++;
		}
	}
}

void
build_table(all_action_template_t * allops,
	    char *extra)
{
	pop_action_template_t *pop_actions;
	char *table[] = { "", "dir_", NULL };
	char **tp = table;

	printf("\nstatic _udi_pops_t _udi_pio_%s_%saccess = {\n",
	       allops->all_name, extra);

	for (tp = table; *tp; tp++) {
		printf("\t{\n");
		pop_actions = allops->pop_action;
		while (pop_actions->pop_proto) {
			int x;
			char *myextra = extra;
			char *mytp = *tp;

			/*
			 * This looks silly, but it's to allow the concept of
			 * "default" ops tables.  If a size isn't specified, it
			 * becomes a "many" entry.
			 */
			printf("\t\t{\n");
			for (x = 0; x < NTABLE_ENTRIES; x++) {
				char *s = sizes[x * 2];
				if ((s == 0) ||
				    (x >=
				     (sizeof (sizes) / sizeof (char *) / 2))) {
					s = "many";
					mytp = "";
					if (allops->at_flags &
					    AT_FLAG_SIMPLE_MANY_ONLY) {
						myextra = "";
					}
				}
				printf("\t\t%s_%s%s%s,\n",
				       pop_actions->pop_op->template_name,
				       myextra, mytp, s);
			}
			printf("\t\t},\n");
			pop_actions++;
		}
		printf("\t},\n");
	}
	printf("};\n");
}

int
main(void)
{
	all_action_template_t *allops = &all_ops[0];

	printf("/* This is a machine generated file.   "
	       " It was created by mkpop.\n"
	       " * Editing it by hand is almost certainly a mistake.\n"
	       " */\n");

	for (allops = &all_ops[0]; allops->pop_action; allops++) {
		pop_action_template_t *pop_actions;


		/*
		 * Loop over the actions (i.e. read vs. write) 
		 */
		for (pop_actions = allops->pop_action; pop_actions->pop_proto;
		     pop_actions++) {

			char **p;

			/*
			 * Loop over all the pacing.  (i.e. pause vs. not) 
			 */
			for (p = &pauses[0]; *p; p++) {

				loop_substitute(allops->at_flags, pop_actions->pop_proto->template, pop_actions->pop_op->template_name, sizes[0], pop_actions->pop_op, *p	/* Pacing op */
					);

				if (allops->at_flags & AT_FLAG_SIMPLE_ONLY)
					break;
			}
		}

		build_table(allops, "");

		if (!(allops->at_flags & AT_FLAG_SIMPLE_ONLY)) {
			build_table(allops, "swapped_");
			build_table(allops, "paced_");
			build_table(allops, "paced_swapped_");
		}
	}
	exit(EXIT_SUCCESS);
}
