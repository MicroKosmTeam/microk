/*
 * File: linux/mkopaque-bin.c
 *
 * Template for generating the opaque data types header files.
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

/* This is meant to only be used with the opaquedefs */

/* This code assumes:
 *     KERNEL && __MODULE__ are defined.
 *     char is 8 bits.
 *     short is 16 bits.
 *     the compiler can handle 0 length arrays. (gcc)
 *     that it is easy to extract symbol sizes.
 *     sizeof() can be used as an array specifier. (gcc)
 *     an opaque type is represented as a struct.
 *     anytime a new opaque defn is added, a cooresponding
 *         entry is added to this file.
 */

#include "udi_osdep_opaquedefs.h"

#if 0
/*
 * If type sizes are used, then this is important.
 * If objdump is used to suck out symbol sizes, we dont need
 * to worry about endianness.
 * If we automate things by using libelf, then we might need this.
 */
extern short ENDIANNESS;
short ENDIANNESS = 0x0102;
#endif

/* Handy macros */
#define STRINGIFY(x) # x
#define DEREF(x) x

/*
 * An attempt to use linker scripts to validate the data types... 
 * In practice, this doesn't work correctly, so we use sizeof instead.
 */
#define SECTIONIZE(x) __attribute ((section (x)))
#undef SECTIONIZE
#define SECTIONIZE(x)


#define GEN_TYPE1(TYPE) \
extern struct TYPE TYPE; \
       struct TYPE TYPE SECTIONIZE(#TYPE"_SECT"); \

#if 0
extern short TYPE##__size; \
       short TYPE##__size = sizeof(TYPE);
#endif

#define GEN_TYPE2(TYPE) \
struct TYPE##__opaque \
{ \
	unsigned long TOP_PAD[0]; \
	char opaque[ sizeof(struct TYPE) ]; \
	unsigned long END_PAD[0]; \
};

#define GEN_TYPE3(TYPE) \
extern struct TYPE##__opaque TYPE##__opaque_var; \
       struct TYPE##__opaque TYPE##__opaque_var \
	SECTIONIZE(#TYPE"__opaque_var_SECT"); \

#if 0
extern short TYPE##__opaque_size; \
       short TYPE##__opaque_size = sizeof(struct TYPE##__opaque);
#endif

#define GEN_TYPE(x) \
	GEN_TYPE1(x) \
	GEN_TYPE2(x) \
	GEN_TYPE3(x)

GEN_TYPE(_udi_osdep_atomic_int_opaque)
GEN_TYPE(_udi_osdep_event_opaque)
GEN_TYPE(_udi_osdep_dev_node_opaque)
GEN_TYPE(_udi_osdep_semaphore_opaque)
GEN_TYPE(_udi_osdep_pio_opaque)
GEN_TYPE(_udi_osdep_spinlock_opaque)
GEN_TYPE(_udi_osdep_driver_opaque)

/*
 * Attempt to do compile-time resolution of sizes.
 * We want to avoid executing code, so we can't plop a main() in
 * here and execute it. (Since we want to validate these sizes on
 * cross-compilation environments.)
 */
#define SUBTRACT_ABS(x,y) ((x) > (y) ? ((x) - (y)) : ((y) - (x)))
#if 1
#define GENTEST_TYPE(TYPE) \
extern short TYPE##__is_invalid; \
       short TYPE##__is_invalid = sizeof(TYPE) - sizeof(TYPE##__opaque_var); \
extern char TYPE##__IS_INVALID[SUBTRACT_ABS(sizeof(TYPE), sizeof(TYPE##__opaque_var))]; \
       char TYPE##__IS_INVALID[SUBTRACT_ABS(sizeof(TYPE), sizeof(TYPE##__opaque_var))];

/* Generate test cases too. */
GENTEST_TYPE(_udi_osdep_atomic_int_opaque)
GENTEST_TYPE(_udi_osdep_event_opaque)
GENTEST_TYPE(_udi_osdep_dev_node_opaque)
GENTEST_TYPE(_udi_osdep_semaphore_opaque)
GENTEST_TYPE(_udi_osdep_pio_opaque)
GENTEST_TYPE(_udi_osdep_spinlock_opaque)
GENTEST_TYPE(_udi_osdep_driver_opaque)
#endif

