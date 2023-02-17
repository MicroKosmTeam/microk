
/*
 * File: env/posix/shrktest.c
 *
 * Test Harness for shark network driver. Allows driver to be bound and then have
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

#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>

#define UDI_NIC_VERSION 0x101
#include <udi_nic.h>
#include <netmap_posix.h>
#include <netmap.h>

#if 0
#include "shrk.h"			/* For shrk network driver */
#else
#include "../../driver/shrkudi/shrk.h"	/* For shrk network driver */
#endif

#include <giomap_posix.h>

_OSDEP_EVENT_T shrktest_ev;

extern void *_udi_MA_local_data(const char *,
				udi_ubit32_t);
extern int nsrmap_open(udi_channel_t);
extern int nsrmap_ioctl(udi_channel_t,
			int,
			void *);
extern int nsrmap_close(udi_channel_t);

#define	ND_DRIVER_NAME		"shrkudi"


#define	SHRK_MAC_IOCTL(cmd, data, datalen, macp, retzero ) \
	{ \
		int retval; \
		(macp)->ioc_cmd = cmd; \
		(macp)->ioc_len = datalen; \
		(macp)->ioc_data = data; \
		printf("Trying %s...\n", #cmd ); \
		retval = nsrmap_ioctl( 0, cmd, macp ); \
		if ( retzero ) { \
			assert( retval == 0 ); \
		} else { \
			assert( retval != 0 ); \
		} \
	}
int Debug = 0;
volatile shrktx_rdata_t *dv;

void
shrktest_parseargs(int c,
		   void *optarg)
{
	switch (c) {
	case 'D':
		Debug++;
		printf("shrktest: Debug %d\n", Debug);
		break;
	}
}


void
shrk_print_MAC(nsr_macaddr_t * macp)
{
	udi_ubit8_t *p = (udi_ubit8_t *)macp->ioc_data;

	printf("Address = %02x.%02x.%02x.%02x.%02x.%02x\n",
	       p[0], p[1], p[2], p[3], p[4], p[5]);
}

void
shrk_test_ioctls(void)
{
	nsr_macaddr_t MAC_req;		/* MAC address manipulation */
	void *loc_buf;			/* Buffer contents  */
	udi_size_t buf_size;

	buf_size = 16 * 6;		/* Size for 16 Multicast Addresses assuming
					 * 6-byte MAC address format
					 */

	loc_buf = _OSDEP_MEM_ALLOC(buf_size, 0, UDI_WAITOK);

	/*
	 * Get the currently active MAC address
	 */
	SHRK_MAC_IOCTL(POSIX_ND_GETADDR, loc_buf, buf_size, &MAC_req, TRUE);
	shrk_print_MAC(&MAC_req);
	SHRK_MAC_IOCTL(POSIX_ND_GETRADDR, loc_buf, buf_size, &MAC_req, TRUE);
	shrk_print_MAC(&MAC_req);

	_OSDEP_MEM_FREE(loc_buf);
}


void
do_binds(void)
{
	posix_module_init(udi_brdg);
	posix_module_init(brdg_map);
	posix_module_init(udi_gio);
	posix_module_init(udi_nic);
	posix_module_init(netmap);
	posix_module_init(shrkudi);
}

void
do_unbinds(void)
{
	posix_module_init(shrkudi);
	posix_module_init(netmap);
	posix_module_init(brdg_map);
	posix_module_init(udi_brdg);
	posix_module_init(udi_nic);
	posix_module_init(udi_gio);
}

int
main(int argc,
     char **argv)
{
	int rval;

	srand(getpid());

	posix_init(argc, argv, "b:Dn:m:M:o:", &shrktest_parseargs);

	_OSDEP_EVENT_INIT(&shrktest_ev);

	do_binds();

	/*
	 * open the card 
	 */
	printf("NIC interface open...\n");
	rval = nsrmap_open(0);
	_OSDEP_ASSERT(rval == 0);
	printf("...NIC interface open succeeded\n");

	dv = (shrktx_rdata_t *) _udi_MA_local_data(ND_DRIVER_NAME, 0);
	assert(dv != NULL);

	if (Debug) {
		printf("percard info dv %p\n", dv);
	}

	shrk_test_ioctls();

	/*
	 * Close the card down 
	 */
	rval = nsrmap_close(0);
	_OSDEP_ASSERT(rval == 0);
	printf("...NIC interface close succeeded\n");

	do_unbinds();

	posix_deinit();

	printf("Exiting\n");
	return 0;
}
