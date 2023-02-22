/*
 * File: linux/opaquetest.c
 *
 * This code can be used with and without opaquedefs.h.
 * Shared code between opaquedefs.c and opaquetypes.c that
 * implements the desired functionality needed to generate
 * the opaque data type definitions.
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

#undef printf
extern int printf(const char *fmt, ...);

void dotest(void)
{
#define STRINGIFY(x) # x
#define MYTEST_TYPE(TYPE) \
	{ \
		int structSize, wrappedSize; \
		typedef struct { \
			char pad1; \
			TYPE t; \
			char pad2;\
		} wrapper_t; \
		structSize = sizeof(TYPE); \
		wrappedSize = sizeof(wrapper_t); \
		printf("%20s: actual:%2d wrapped:%2d\n", STRINGIFY(TYPE), structSize, wrappedSize); \
	}

MYTEST_TYPE(_OSDEP_ATOMIC_INT_T);
MYTEST_TYPE(_OSDEP_EVENT_T);
MYTEST_TYPE(_OSDEP_SEM_T);
MYTEST_TYPE(_OSDEP_DEV_NODE_T);
MYTEST_TYPE(_OSDEP_PIO_T);
MYTEST_TYPE(_OSDEP_MUTEX_T);
MYTEST_TYPE(_OSDEP_SIMPLE_MUTEX_T);
MYTEST_TYPE(_OSDEP_TIMER_T);
MYTEST_TYPE(_OSDEP_TICKS_T);

/* These don't use kernel headers */
/*
_OSDEP_DRIVER_T
_OSDEP_DMA_T
_OSDEP_ISR_RETURN_T
*/
	
}
