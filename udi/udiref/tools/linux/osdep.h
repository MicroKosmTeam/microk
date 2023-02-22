/*
 * File: tools/linux/osdep.h
 *
 * Configures tool options, such as architecture build flags
 * and os-dependent directories.
 */

/*
 * UDI Linux specific definitions use in the udi tools.
 *
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

#ifndef __TOOLS_LINUX_OSDEP_H__
#define __TOOLS_LINUX_OSDEP_H__

#if defined(__sparc__) /*&& defined(__arch64__)*/
/* The tools cant tell what they're targeting yet! Doh! */
/* So if we're on a Sparc, assume the kernel is 64 bit. */
#define is_sparc64
#warning "__sparc__ is defined."
#endif

/* Tool configuration for Linux */
#define UDI_DRVFILEDIR  "/lib/modules/misc"
#define UDI_NICFILEDIR  "/lib/modules/net"

#define OSDEP_PKG_ARCHIVE "/lib/UDI"

#define OS_MKPAX_WITH_TAR    /* Linux has tar but not pax */
#define OS_GETPAX_WITH_TAR
#define OS_GLUE_PROPS_STRUCT


/* Define the functions we want to override */
#define OS_COMPILE_OVERRIDE
#define OS_LINK_OVERRIDE

/* If we use a CODEFILE here, make wont understand the dependency. */
/*#define OS_CODEFILE "linux_override.c"*/

/* Unfortunately, only ONE flag can be specified here. */
#define OS_COMPILE_DEBUG_FLAG "-g3"

/*
 * "Using any optimization more than -O2 is risky, as the compiler
 * might inline functions that are not declared as inline in the
 * source. This may be a problem with kernel code, as some functions
 * expect to find a standard stack layout when they are called."
 *     Quoted from "Linux Device Drivers", O'Reilly & Assoc.
 */
#define OS_COMPILE_OPTIMIZE_FLAG "-O2"


#if 1
/* Make the environment less verbose. */
#define MAPPER_DEBUG_INFO ""
#else
/* Make the environment more verbose and debuggable. */
#define MAPPER_DEBUG_INFO "-DDEBUG -DSTATIC= "
#endif

/* Architecture independent flags */
/* Switch between three modes of malloc debugging... most ... least. */
#if 0
#define UDI_CCOPTS_MAPPER_DEBUG_FLAGS "-DINSTRUMENT_MALLOC " MAPPER_DEBUG_INFO
#elif 1
#define UDI_CCOPTS_MAPPER_DEBUG_FLAGS MAPPER_DEBUG_INFO  
#else
#define UDI_CCOPTS_MAPPER_DEBUG_FLAGS "-DLOW_OVERHEAD_MALLOC " MAPPER_DEBUG_INFO
#endif
#define UDI_CCOPTS_DEBUG_FLAGS "-Wall "

/*
 * Annoying verbose flags:
 * -Wstrict-prototypes -Wmissing-prototypes
 * -Wmissing-declarations
 */

#define UDI_CCOPTS_ARCHINDEP \
	"-ffreestanding -fno-builtin " \
	"-fno-omit-frame-pointer -fsigned-char "
/*" -pipe -fno-strength-reduce -fno-strict-aliasing " */
/*
 * -fsigned-char is needed for anything that talks to the kernel.
 * -funsigned-char is default on PPC.
 * -fsigned-char is default on x86.
 */

#define UDI_LINKOPTS_ARCHINDEP "-r "

/* -ffree-standing: includes -fno-builtin? and also causes the
 *     compiler to not generate calls to any assumed built-in
 *     functions. This removes any problems by gcc calling memset,
 *     as well as gcc creating prototypes for memcpy, memset and
 *     memcmp... which conflict with /usr/include/linux/strings.h.
 */

#undef CROSS_COMPILE

/* This is for generating 64-bit capable binaries. */
#if defined(is_sparc64)
#define UDI_CC "sparc64-linux-gcc"
#define UDI_CCOPTS_ARCHDEP "-m64"
#ifdef CROSS_COMPILE
#warning "Hello Sparc64 Linux"
#endif

#elif defined(__alpha__)
/* Alpha needs some special flags when using linux/sched.h.
 * More flags than are really needed might be present.
 */
#define UDI_CCOPTS_ARCHDEP \
	"-pipe -mno-fp-regs -ffixed-8 -mcpu=ev4 "
#define UDI_LINKOPTS_ARCHDEP \
	"-b elf64-alpha "
#ifdef CROSS_COMPILE
#warning "Hello Alpha Linux"
#endif

#elif defined(__powerpc__)

/* These more closely match the kernel build arguments. */
#define UDI_CCOPTS_ARCHDEP \
	"-mmultiple -mstring -ffixed-r2 -msoft-float "
#define UDI_LINKOPTS_ARCHDEP \
	""
#ifdef CROSS_COMPILE
#warning "Hello Linux PPC"
#endif

#else

#if defined(__i386__)
#ifdef CROSS_COMPILE
#warning "Hello Linux ia32"
#endif
#define UDI_LINKOPTS_ARCHDEP "-b elf32-i386 "
#else
#warning "Unknown architecture. Use cc -dM -E - </dev/null to find your arch"
#define UDI_LINKOPTS_ARCHDEP ""
#endif
#endif

/* Default values when an arch didn't specify one */
#ifndef UDI_CC
#define UDI_CC "cc"
#endif
#ifndef UDI_CCOPTS_ARCHDEP
#define UDI_CCOPTS_ARCHDEP ""
#endif
#ifndef UDI_LINK
#define UDI_LINK "ld"
#endif
#ifndef UDI_LINKOPTS_ARCHDEP
#define UDI_LINKOPTS_ARCHDEP ""
#endif

/*
 * Specifying -b fails pretty miserably in non-pure cross compile
 * environments.
 */
/* #undef UDI_LINKOPTS_ARCHDEP */
/* #define UDI_LINKOPTS_ARCHDEF "" */

/* Flags suitable for glue code, mapper, & environment generation. */
#define UDI_CCOPTS_MAPPER UDI_CCOPTS_ARCHDEP UDI_CCOPTS_ARCHINDEP \
			 UDI_CCOPTS_MAPPER_DEBUG_FLAGS UDI_CCOPTS_DEBUG_FLAGS

/*
 * The cleanest ANSI C compile environment we can get...
 * Drivers fall in this category.
 * Yep, it is very verbose if you're not following strict
 * function prototyping rules, or if you violate ANSI-ness.
 */
#define UDI_CCOPTS "-ansi -pedantic " \
	" -Wstrict-prototypes -Wmissing-prototypes " \
	" -Wmissing-declarations " \
	UDI_CCOPTS_ARCHINDEP UDI_CCOPTS_DEBUG_FLAGS

#define UDI_LINKOPTS UDI_LINKOPTS_ARCHINDEP UDI_LINKOPTS_ARCHDEP


#include <string.h>
#include <sys/fcntl.h>

#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <elf.h>
#include <libelf/libelf.h>

#if !defined(__ia64__) && !defined(__alpha__) && !defined(is_sparc64)
/*
 * For Linux on IA32 there is only partial ELF64 support.
 * Right now, on any non-64 bit ABI we just turn it off completely.
 * For Alpha Linux and Sparc 64, don't disable elf64!
 */

#define	elf64_getehdr(x)	NULL
#define	elf64_getshdr(x)	NULL
#define Elf64_Ehdr		Elf32_Ehdr
#define Elf64_Shdr		Elf32_Shdr
#endif

#endif /* multi-include wrapper */
