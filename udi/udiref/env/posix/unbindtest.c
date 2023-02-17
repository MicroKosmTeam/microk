
/*
 * File: env/posix/unbindtest.c
 *
 * Verify that we can unbind the pseudo driver and the mapper and de-init
 * the environment.
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
#include <stdlib.h>

void
do_binds(void)
{
	posix_module_init(udi_gio);
	posix_module_init(udi_brdg);
	posix_module_init(brdg_map);
	posix_module_init(pseudod);
	posix_module_init(giomap);
}

/*
 * Sequence of unbinds which approximates a real OS unload order
 */

void
do_unbinds(void)
{
	posix_module_deinit(pseudod);
	posix_module_deinit(giomap);
	posix_module_deinit(brdg_map);
	posix_module_deinit(udi_brdg);
	posix_module_deinit(udi_gio);
}


udi_ubit32_t loop = 2;			/* # of times to run whole test */


/* The unbindtest will run through several iterations of
   init-bind-unbind-uninit to validate the hygene of those operations.
   Unfortunately, init processes the command line arguments using
   getopt and getopt consumes those arguments with the side-effect of
   nullifying them.  Therefore if we want to iterate multiple times we
   need multiple copies of the argument list.  Sigh.

   Use posix malloc/free here rather than _OSDEP_MEM_ALLOC stuff
   because the UDI environment isn't initialized yet (i.e. no alloc
   daemon is running).  At least this gives us the advantage of not
   needing to deallocate this stuff when we're done.  */

char **
dup_args2(int argc,
	  char **argv)
{
	char **new_argv;
	int ii;

	new_argv = malloc(argc * sizeof (char *));
	if (new_argv == NULL) {
		printf("ARGV duplication failed.\n");
		exit(2);
	}

	for (ii = 0; ii < argc; ii++) {
		new_argv[ii] = malloc(strlen(argv[ii]));
		if (new_argv[ii] == NULL) {
			printf("Argument duplication failed.\n");
			exit(3);
		}
		strcpy(new_argv[ii], argv[ii]);
	}

	return new_argv;
}


static char argcopy[1024];
static char *dup_argv[100];

char **
dup_args(int argc,
	 char **argv)
{
	char *cp;
	int ii;

	cp = argcopy;

	for (ii = 0; ii < argc; ii++) {
		dup_argv[ii] = cp;
		strcpy(cp, argv[ii]);
		cp += strlen(cp) + 1;
	}

	return dup_argv;
}


void
unbindtest_parseargs(int c,
		     void *optarg)
{
	switch (c) {
	case 'L':
		loop = udi_strtou32(optarg, NULL, 0);
		break;
	}
}


int
main(int argc,
     char **argv)
{
	udi_ubit32_t iter;

/* 	posix_init(argc, dup_args(argc, argv), "l:", &unbindtest_parseargs); */

#if 0
	posix_init(argc, argv, "L:", &unbindtest_parseargs);
	posix_deinit();
#endif

	printf("Initial binding exercised\n");

	for (iter = 0; iter < loop; iter++) {
		/*
		 * As you can see, the "hard" part is done elsewhere. 
		 */

/* 		posix_init(argc, dup_args(argc, argv), */

/* 			   "L:", &unbindtest_parseargs); */
		posix_init(argc, argv, "L:", &unbindtest_parseargs);

		do_binds();

		printf("Binding %d complete; attempting unbind...\n", iter);

		do_unbinds();

		printf("Deinitializing %d...\n", iter);

		posix_deinit();

		printf("\n\n*** Iteration %d complete\n\n", iter);
	}

	/*
	 * If we didn't abort, we made it! 
	 */
	printf("Exiting\n");
	assert(Allocd_mem == 0);
	return (0);
}
