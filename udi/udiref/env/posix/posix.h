
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

/*
 * Provide definitions for the POSIX 1003 and POSIX thread layer.
 * There should be no OS-specific code in here.
 *
 * This file should provide all the information needed for
 * specifically building the POSIX test fixtures.  The information
 * needed to build a user-space environment via POSIX definitions is
 * found in env/common/posix_env.h instead.
 *
 */

#ifndef POSIX_H
#define POSIX_H

#ifndef STATIC
#  define STATIC static
#endif

#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <stdio.h>
#undef offsetof				/* Defined in udi_types.h for drivers */
#include <stddef.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/times.h>
#include <string.h>
#include <unistd.h>

/* 
 * Common POSIX definitions.
 */

extern char *posix_me;
extern sem_t posix_loader_busy;

/*
 * Startup/shutdown support
 */

#define posix_module_init(ModuleName) {					\
		void *ModuleName##_POSIX_get_init_info(void);		\
		_OSDEP_DRIVER_T driver_info = {NULL}; 			\
		sem_wait(&posix_loader_busy);				\
		_udi_module_load(#ModuleName,				\
				  ModuleName##_POSIX_get_init_info(),	\
				  driver_info);				\
		sem_post(&posix_loader_busy);				\
	}

#define posix_module_deinit(ModuleName) {				\
		void *ModuleName##_POSIX_get_init_info(void);		\
		sem_wait(&posix_loader_busy);				\
		_udi_module_unload_by_name(#ModuleName, ModuleName##_POSIX_get_init_info());		\
		sem_post(&posix_loader_busy);				\
	}

void posix_deinit(void);

void posix_init(int argc,
		char **argv,
		char *testargs,
		void (*test_parse_func) (int c,
					 void *optarg));

void *_udi_module_load(char *name, void *init_info, 
		       _OSDEP_DRIVER_T driver_info);
void _udi_module_unload_by_name(char *name, void *init_info);


#endif /* POSIX_H */
