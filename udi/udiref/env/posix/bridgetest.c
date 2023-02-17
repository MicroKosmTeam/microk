
/*
 * File: env/posix/bridgetest.c
 *
 * Test Harness for Bridge mapper. Allows driver to be bound and then have
 * Meta-specific actions applied.
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

#include <udi_env.h>
#include <posix.h>
#include "../common/udi_mgmt_MA.h"

#include <giomap_posix.h>

extern int posix_OS_interrupt(void);

void
do_binds(void)
{
	posix_module_init(udi_gio);
	posix_module_init(udi_brdg);
	posix_module_init(brdg_map);
	posix_module_init(brdg_drv);
}


void
do_unbinds(void)
{
	posix_module_deinit(brdg_map);
	posix_module_deinit(brdg_drv);
	posix_module_deinit(udi_gio);
	posix_module_deinit(udi_brdg);
}


int Debug = 0;
udi_ubit32_t bufsize = 32;

void
buftest_parseargs(int c,
		  void *optarg)
{
	switch (c) {
	case 'b':
		bufsize = udi_strtou32(optarg, NULL, 0);
		break;
	case 'D':
		Debug++;
		break;
	}
}

int
main(int ac,
     char **av)
{
	int rval;

	posix_init(ac, av, "b:D", &buftest_parseargs);

	do_binds();

	rval = posix_OS_interrupt();

	if (rval) {
		printf("posix_OS_interrupt FAILED\n");
		return rval;
	} else {
		printf("posix_OS_interrupt Succeeded\n");
	}
	/*
	 * Now try and unbind 
	 */

	do_unbinds();
	posix_deinit();

	printf("Exiting\n");
	return 0;
}
