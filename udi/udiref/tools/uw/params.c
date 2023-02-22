/*
 * File: tools/uw/params.c
 *
 * SVR5-specific overrides to the default options of udibuild.
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
#include "osdep.h"
#include "global.h"

char *svr5_mapper_opts;

/*
 * Optionally override our default choices of tools.
 */
void
uw_override_params(void)
{
	/* 
	 * See if we're in an cross environment.   If so, we smuggle
	 * the data into the main compiler by setting the env vars
	 * that it knows to look at.    As a practical matter, none
	 * of this will ever get used by anyone building the tree
	 * outside of the Caldera-internal build processes.
	 */
	char *cpu;
	char *new_cc = optinit();
	char *new_link = optinit();

	if (getenv("ROOT") == NULL)
		return;

	optcpy(&new_cc, "UDI_CC=");
	optcat(&new_cc, getenv("CPU"));
	optcat(&new_cc, UDI_CC);
	putenv(new_cc);

	optcpy(&new_link, "UDI_LINK=");
	optcat(&new_link, getenv("CPU"));
	/*
	 * We don't use UDI_LINK below becuase while we're in a cross, 
	 * we don't use "i386idld", we need "i386ld".
	 */
	optcat(&new_link, "ld");
	putenv(new_link);

	if (udi_global.make_mapper) {
		optcpy(&svr5_mapper_opts, UDI_CCOPTS_DEFAULT_MAPPER_OPTS);
		optcat(&svr5_mapper_opts, " ");
		optcat(&svr5_mapper_opts, getenv("SVR5_UDI_MAPPER_OPTS"));
	}
}
