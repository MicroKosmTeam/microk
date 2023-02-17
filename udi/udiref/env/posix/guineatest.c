
/*
 * File: env/posix/guineatest.c
 *
 * Test Harness for GIO mapper and guinea driver
 *
 * Usage:
 *	guineatest [-b <buffer size>] [-A] [-B] [-C] [-D] [-n <itter>]
 *			-A	use async ioctl
 *			-B	do biostart test
 *			-C	do ioctl test
 *			-D	increment Debug flag
 *
 * Provides a read/write test for buffered i/o to the guinea driver using
 * GIO primitives.
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

#include <unistd.h>
#include <udi_env.h>
#include <posix.h>

#include <giomap_posix.h>


#define	DPRINT(lev, x)	if ( Debug >= lev ) { \
				printf x; \
			}

typedef enum { Biostart, Ioctl, Any } Test_t;
char *testname[] = { "Biostart",
	"Ioctl",
	"Any"
};

/*
 * GIO mapper interfaces
 */

int giomap_test(udi_ubit32_t,
		Test_t);
void show_time(char *,
	       clock_t,
	       clock_t);

int Debug = 0;
int Async = 0;				/* Synchronous ioctl */

void
do_binds(void)
{
	posix_module_init(udi_gio);
	posix_module_init(udi_brdg);
	posix_module_init(brdg_map);
	posix_module_init(udi_gpig);
	posix_module_init(giomap);
}

void
do_unbinds(void)
{
	posix_module_deinit(brdg_map);
	posix_module_deinit(udi_gpig);
	posix_module_deinit(giomap);
	posix_module_deinit(udi_gio);
	posix_module_deinit(udi_brdg);
}



udi_ubit32_t bufsize = 20;		/* Default buffer size */
udi_ubit32_t niter = 1;			/* # of iterations */
Test_t test = Any;			/* Run all tests */

void
giotest_parseargs(int c,
		  void *optarg)
{
	switch (c) {
	case 'b':			/* Buffer size */
		bufsize = udi_strtou32(optarg, NULL, 0);
		break;
	case 'D':
		Debug++;
		break;
	case 'n':
		niter = udi_strtou32(optarg, NULL, 0);
		break;
	case 'A':			/* Use asynchronous ioctl */
		Async = 1;
		break;
	case 'B':
		test = Biostart;
		break;
	case 'C':
		test = Ioctl;
		break;
	}
}

int
main(int ac,
     char **av)
{
	int rval;
	udi_ubit32_t n;

	posix_init(ac, av, "b:Dn:ABC", &giotest_parseargs);

	do_binds();

	/*
	 * We've now got the GIO mapper and pseudo driver 
	 */

	for (n = 0; n < niter; n++) {
		/*
		 * Test the biostart() interface 
		 */
		rval = giomap_test(bufsize, test == Any ? Biostart : test);
		if (rval != 0) {
			printf("\n*** %s test FAILED ***\n",
			       test == Any ? "Biostart" : testname[(int)test]);
			return -1;
		}

		/*
		 * Test the ioctl() interface 
		 */
		rval = giomap_test(bufsize, test == Any ? Ioctl : test);
		if (rval != 0) {
			printf("\n*** %s test FAILED ***\n",
			       test == Any ? "Ioctl" : testname[(int)test]);
			return -1;
		}
		printf("Completed Iteration %d of %d\n", n + 1, niter);
	}

	/*
	 * Now try and unbind 
	 */

	do_unbinds();

	posix_deinit();

	printf("Exiting\n");
	return 0;
}

/*
 * Simple Read/Write test
 */
int
giomap_test(udi_ubit32_t Bufsize,
	    Test_t test)
{
	int rval;
	giomap_buf_t mybuf;		/* for Biostart test */
	giomap_uio_t myuio;		/* for Ioctl test */
	udi_gio_rw_params_t myparams;	/* for Ioctl test only */
	void *databuf;
	udi_ubit32_t total = 0;
	struct tms start, end;
	clock_t s_clk, e_clk;
	int retval = 0;

	printf("Opening GIO device 0\n");
	rval = giomap_open(0);		/* Should exist */
	assert(rval == 0);

	printf("Opening GIO device 17 (non-existent)\n");
	rval = giomap_open(17);		/* Shouldn't exist */
	assert(rval != 0);

	udi_memset(&mybuf, 0, sizeof (mybuf));

	udi_memset(&myuio, 0, sizeof (myuio));

	udi_memset(&myparams, 0, sizeof (myparams));

	databuf = _OSDEP_MEM_ALLOC(Bufsize + 1, 0, UDI_WAITOK);
	udi_memset(databuf, 0, Bufsize + 1);
	mybuf.b_addr = databuf;
	mybuf.b_flags = GIOMAP_B_READ;
	mybuf.b_bcount = Bufsize;

	myuio.u_addr = databuf;
	myuio.u_op = UDI_GIO_OP_READ;
	myuio.u_count = Bufsize;
	myuio.tr_param_len = sizeof (myparams);
	myuio.tr_params = &myparams;

	printf("Starting test sequences...\n");

	s_clk = times(&start);
	while (total < Bufsize) {
		if (test == Biostart) {
			DPRINT(2,
			       ("giomap_test: Calling giomap_biostart {READ}\r"));
			DPRINT(3,
			       ("\n\t... b_bcount = %d, b_flags = %d\n",
				mybuf.b_bcount, mybuf.b_flags));
			mybuf.b_addr = databuf;
			rval = giomap_biostart(0, &mybuf);
			DPRINT(3,
			       ("Return value = %d, b_error = %d b_resid = %d\n",
				rval, mybuf.b_error, mybuf.b_resid));

			if (mybuf.b_error == 0) {
				if (Debug >= 4) {
					printf("New buffer contents = [");
					write(1, databuf, rval);
					printf("]\n");
				}
				total += (udi_ubit32_t)rval;
				mybuf.b_bcount = Bufsize - total;
			} else {
				retval = -1;
				break;
			}
		} else {
			/*
			 * Ioctl test  - Cannot perform asynchronous READ 
			 */
			DPRINT(2,
			       ("giomap_test: Calling giomap_ioctl {READ}\r"));
			DPRINT(3, ("\n\t... u_count = %d\n", myuio.u_count));

			myuio.u_addr = databuf;
			myuio.u_async = 0;

			rval = giomap_ioctl(0, UDI_GIO_DATA_XFER, &myuio);

			DPRINT(3, ("Return value = %d, u_error = %d\n", rval,
				   myuio.u_error));

			if (myuio.u_error == 0) {
				if (Debug >= 4) {
					printf("New buffer contents = [");
					write(1, databuf, myuio.u_count);
					printf("]\n");
				}
				total += myuio.u_count;
				myuio.u_count = Bufsize - total;
			} else {
				retval = -1;
				break;
			}
		}

	}
	e_clk = times(&end);
	printf("\ngiomap_test: Read %d bytes using %s\n", total,
	       test == Biostart ? "giomap_biostart" : "giomap_ioctl");
	show_time("Time to Read", s_clk, e_clk);

	if (retval != 0)
		return retval;		/* Bail out now */

	/*
	 * Now write the data back out -- tests the alternate case 
	 */
	udi_memset(databuf, 'E', Bufsize + 1);
	mybuf.b_bcount = Bufsize;
	mybuf.b_flags = GIOMAP_B_WRITE;
	mybuf.b_blkno = 0;
	mybuf.b_blkoff = 0;
	mybuf.b_resid = 0;
	mybuf.b_addr = databuf;
	myuio.u_count = Bufsize;
	myuio.u_op = UDI_GIO_OP_WRITE;
	myuio.u_resid = 0;
	myuio.u_addr = databuf;

	udi_memset(&myparams, 0, sizeof (myparams));

	if (test == Biostart) {
		DPRINT(2, ("giomap_test: Calling giomap_biostart {WRITE}\n"));
		s_clk = times(&start);
		rval = giomap_biostart(0, &mybuf);
		e_clk = times(&end);
		if (mybuf.b_error == 0) {
			printf("giomap_test: Wrote %d bytes using giomap_biostart\n", rval);
		} else {
			retval = -1;
		}
	} else {
		/*
		 * Ioctl test 
		 */
		/*
		 * If we're performing asynchronous WRITEs, we'll attempt to
		 * abort the request as soon as we get the handle.
		 * There is no completion mechanism for the aborted request
		 * as the calling application _must_ have used an asynchronous
		 * ioctl() to initiate the process
		 */
		myuio.u_async = Async;
		DPRINT(2, ("giomap_test: Calling giomap_ioctl {WRITE} %s\n",
			   Async ? "asynchronously" : "synchronously"));
		s_clk = times(&start);
		rval = giomap_ioctl(0, UDI_GIO_DATA_XFER, &myuio);
		e_clk = times(&end);
		if (Async) {
			printf("giomap_test: Got handle [%p] for request\n",
			       myuio.u_handle);

#if 0
			/*
			 * Now attempt to abort the request
			 */
			rval = giomap_ioctl(0, UDI_GIO_ABORT, myuio.u_handle);

			printf("giomap_test; ABORT returned %d\n", rval);
#endif

			retval = rval;
		} else {
			if (myuio.u_error == 0) {
				printf("giomap_test: Wrote %d bytes using giomap_ioctl\n", myuio.u_count - myuio.u_resid);
			} else {
				retval = -1;
			}
		}
	}
	if ((test == Biostart) || ((test == Ioctl) && !Async)) {
		show_time("Time to Write", s_clk, e_clk);
	}

	rval = giomap_close(17);	/* Should fail */
	assert(rval != 0);

	rval = giomap_close(0);
	assert(rval == 0);

	_OSDEP_MEM_FREE(databuf);

	return retval;
}

void
show_time(char *s,
	  clock_t start,
	  clock_t end)
{
	printf("%s: Elapsed Time = %ld ticks\n", s, end - start);
}
