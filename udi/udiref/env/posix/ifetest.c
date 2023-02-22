
/*
 * File: env/posix/ifetest.c
 *
 * Test Harness for Interphase Fast Ethernet network driver. Allows
 * driver to be bound and then have Meta-specific actions applied
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

extern udi_init_t ife_init_info;
extern udi_init_t netmap_posix_init_info;
extern udi_init_t bridgemap_init_info;

extern udi_mei_init_t udi_nic_meta_info;
extern udi_mei_init_t udi_bridge_meta_info;

struct posix_drv_info drivers[] = {
	{"udi_nic", NULL, &udi_nic_meta_info, NULL},
	{"udi_brdg", NULL, &udi_bridge_meta_info, NULL},
	{"brdg_map", &bridgemap_init_info, NULL, NULL},
	{"ife", &ife_init_info, NULL, NULL},
	{"netmap", &netmap_posix_init_info, NULL, NULL},
	{NULL}
};


int
main(int argc,
     char **argv)
{
	int rval;
	char c;

	posix_init(argc, argv, "", NULL);

	rval = posix_MA_bind(drivers);

	if (rval) {
		printf("posix_MA_bind FAILED\n");
		return rval;
	} else {
		printf("posix_MA_bind Succeeded\n");
	}

	/*
	 * Now try and unbind 
	 */

	posix_MA_unbind(drivers);

	posix_deinit();

	printf("Exiting\n");
	return 0;
}
