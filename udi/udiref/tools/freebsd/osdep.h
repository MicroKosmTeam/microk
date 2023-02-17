/*
 * src/tools/linux/osdep.h
 *
 * UDI FreeBSD specific definitions use in the udi tools.
 *
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
 *      
 */

#ifndef __TOOLS_OSDEP_H__
#define __TOOLS_OSDEP_H__

#define OS_GLUE_PROPS_STRUCT

/* FIXME: probably should go somewhere else */
#define OSDEP_PKG_ARCHIVE "/usr/udi/UDI"

/* Unfortunately, only ONE flag can be specified here. */
#define OS_COMPILE_DEBUG_FLAG "-g3"

#define OS_COMPILE_OPTIMIZE_FLAG "-O2"


#define UDI_CCOPTS_DEBUG_FLAGS "-Wall " \
	"-fno-omit-frame-pointer -fno-strict-aliasing "

/*
 * Annoying verbose flags:
 * -Wstrict-prototypes -Wmissing-prototypes
 * -Wmissing-declarations
 */

#define UDI_CCOPTS_ARCHINDEP \
	"-g -ffreestanding -fno-builtin -fno-strict-aliasing "
#define UDI_LINKOPTS_ARCHINDEP "-r "
#define OS_LINK_OVERRIDE

/* -ffree-standing: includes -fno-builtin? and also causes the
 *     compiler to not generate calls to any assumed built-in
 *     functions. This removes any problems by gcc calling memset,
 *     as well as gcc creating prototypes for memcpy, memset and
 *     memcmp... which conflict with /usr/include/linux/strings.h.
 */

/* Flags suitable for glue code, mapper, & environment generation. */
#define UDI_CCOPTS_MAPPER UDI_CCOPTS_ARCHINDEP UDI_CCOPTS_DEBUG_FLAGS

#define UDI_CC "cc"
#define UDI_LINK "ld"

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

#define UDI_LINKOPTS UDI_LINKOPTS_ARCHINDEP 


#include <string.h>
#include <sys/fcntl.h>

#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <libelf/libelf.h>

/*
 * Do these really exist on FreeBSD?   They must...
 */
#define elf64_getehdr(x)        NULL
#define elf64_getshdr(x)        NULL
#define Elf64_Ehdr              Elf32_Ehdr
#define Elf64_Shdr              Elf32_Shdr


#endif /* multi-include wrapper */
