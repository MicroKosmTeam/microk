
/*
 * File: env/posix/cmostest.c
 *
 * Test Harness for CMOS driver. Allows driver to be bound and then have
 * Meta-specific actions applied
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

#include <giomap_posix.h>

int cmos_test(void *,
	      void *,
	      udi_ubit32_t,
	      char);

void
do_binds(void)
{
	posix_module_init(udi_gio);
	posix_module_init(udi_brdg);
	posix_module_init(brdg_map);
	posix_module_init(udi_cmos);
	posix_module_init(giomap);
}

void
do_unbinds(void)
{
	posix_module_deinit(udi_cmos);
	posix_module_deinit(giomap);
	posix_module_deinit(brdg_map);
	posix_module_deinit(udi_gio);
	posix_module_deinit(udi_brdg);
}

int Debug = 0;
udi_ubit32_t bufsize = 32;
int niter = 1;

void
cmostest_parseargs(int c,
		   void *optarg)
{
	switch (c) {
	case 'b':
		bufsize = udi_strtou32(optarg, NULL, 0);
		break;
	case 'D':
		Debug++;
		break;
	case 'n':
		niter = udi_strtou32(optarg, NULL, 0);
		break;
	}
}

int
main(int ac,
     char **av)
{
	int rval;
	int i;
	struct tms start, end;
	clock_t s_clk, e_clk;
	void *databuf, *databuf2;

	posix_init(ac, av, "b:Dn:", &cmostest_parseargs);

	/*
	 * Bind the driver and mappers 
	 */
	do_binds();

	/*
	 * Open the GIO interface to the driver 
	 */
	rval = giomap_open(0);
	_OSDEP_ASSERT(rval == 0);

	databuf = _OSDEP_MEM_ALLOC(bufsize, 0, UDI_WAITOK);
	databuf2 = _OSDEP_MEM_ALLOC(bufsize, 0, UDI_WAITOK);

	s_clk = times(&start);
	for (i = 0; i < niter; i++) {
		printf("Iteration %d\n", i);
		rval = cmos_test(databuf, databuf2, bufsize,
				 (char)((i % 10) + '0'));
		if (rval != 0) {
			printf("\n*** Test Iteration %d FAILED ***\n", i);
			return -1;
		}
	}
	e_clk = times(&end);

	_OSDEP_MEM_FREE(databuf);
	_OSDEP_MEM_FREE(databuf2);

	/*
	 * Close the card down 
	 */
	rval = giomap_close(0);
	_OSDEP_ASSERT(rval == 0);

	printf("\nElapsed Time = %d ticks\n", (int)(e_clk - s_clk));

	/*
	 * Now try and unbind 
	 */
	do_unbinds();

	/*
	 * Deinitialize 
	 */
	posix_deinit();

	printf("Exiting\n");
	return 0;
}

int
cmos_test(void *databuf,
	  void *databuf2,
	  udi_ubit32_t bufsize,
	  char c)
{
	giomap_buf_t mybuf;		/* for Biostart test */
	int rval;

	udi_memset(databuf, c, bufsize);
	udi_memset(&mybuf, 0, sizeof (mybuf));

	mybuf.b_addr = databuf;
	mybuf.b_flags = GIOMAP_B_READ;
	mybuf.b_bcount = bufsize;

	rval = giomap_biostart(0, &mybuf);
	rval = 0;
	if (Debug)
		printf("rval = %d\n", rval);
	if (rval == 0) {
		int i;

		for (i = 0; i < bufsize; i++)
			printf("%02x ", ((udi_ubit8_t *)databuf)[i]);
		printf("\n");
		fflush(stdout);
	}
	return rval;
#if 0
	if (Debug) {
		printf("databuf2 = <%s>\n", databuf2);
	}
	return (udi_memcmp(databuf, databuf2, bufsize));
#endif
}
