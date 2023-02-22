/*
 * tools/darwin/osdep.h
 *
 * UDI Darwin specific definitions use in the udi tools.
 */

/*
 * $Copyright udi_reference:
 * 
 * 
 *    Copyright (c) 1995-2000; Compaq Computer Corporation; Hewlett-Packard
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


#define UDI_DRVFILEDIR  "/System/Library/Extensions"
#define UDI_NICFILEDIR  "/System/Library/Extensions"

#define OSDEP_PKG_ARCHIVE "/Library/UDI"

/* -ffree-standing: includes -fno-builtin and also causes the
 *     compiler to not generate calls to any assumed built-in
 *     functions. This removes any problems by gcc calling memset,
 *     as well as gcc creating prototypes for memcpy, memset and
 *     memcmp... which conflict with /usr/include/linux/strings.h.
 */
#define UDI_CC "cc"
#define UDI_LINK "ld"

#if 0
#define OS_COMPILE_OPTIMIZE_FLAG "-O2"
#else
#define OS_COMPILE_OPTIMIZE_FLAG ""
#endif

#define OS_COMPILE_DEBUG_FLAG "-gstabs+"
  

/* Architecture independent flags */
#define UDI_CCOPTS_DEBUG_FLAGS "-DDEBUG -DSTATIC= " \
        "-W -Wall " \
        "-fno-omit-frame-pointer "
/* 
 * Annoying verbose flags: 
 * -Wstrict-prototypes -Wmissing-prototypes
 * -Wmissing-declarations
 */

/* #if __powerpc as opposed to x86 */
#define UDI_CCOPTS_ARCHDEP ""
#define UDI_LINKOPTS_ARCHDEP ""

#define UDI_CCOPTS_ARCHINDEP \
        "-traditional-cpp -ffreestanding "
#define UDI_LINKOPTS_ARCHINDEP "-r " 
 
/* Flags suitable for glue code, mapper, & environment generation. */
#define UDI_CCOPTS_MAPPER UDI_CCOPTS_ARCHDEP UDI_CCOPTS_ARCHINDEP UDI_CCOPTS_DEBUG_FLAGS
 
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



#define OS_MKPAX_WITH_TAR    /*Darwin has pax & tar */
#define OS_GETPAX_WITH_TAR  /*Darwin has pax & tar */

#define OS_CODEFILE "darwin_osexec.c"
#define OS_DELETE_OVERRIDE
#define OS_LINK_OVERRIDE

#include <string.h>
#include <sys/fcntl.h>
#include <ctype.h> /* for tolower */
#include <unistd.h>

#include <stdio.h>

/* Include object module headers */
#include <libelf/libelf.h>
#include </usr/include/mach-o/getsect.h>
#include </usr/include/mach-o/loader.h>

#include <nl_types.h>

