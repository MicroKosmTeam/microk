/*
 * File: tools/solaris/osdep.h
 *
 * UDI Solaris specific definitions use in the udi tools.
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



#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "architecture.h"

/* Allow use of both gcc and native cc for comparison */
#define	NATIVE_CC	1    	/* Set 0 for gcc, 1 for native cc */

#define OS_GETPAX_WITH_TAR  	/* Solaris pax read seems to have problems... */
#define	OS_GLUE_PROPS_STRUCT	/* Use sprops struct in each driver */
#define	OS_LINK_OVERRIDE	/* Use solaris/link.c os_link() */
#define	OS_COMPILE_OVERRIDE	/* Use solaris/link.c os_compile() */

#if NATIVE_CC
  #define UDI_CC "cc"
  #define UDI_LINK "ld"
  #define UDI_CCOPTS_DEBUG_FLAGS	"-DDEBUG=1 -g "

  /* Architecture independent compilation flags */
  #define UDI_CCOPTS_ARCHINDEP		"-Xc -mt -v "
  #define UDI_LINKOPTS_ARCHINDEP	"-r "
  #define UDI_CCOPTS_MAPPER_FLAGS	"-Xa -mt -v "

#else	/* !NATIVE_CC */
  #define UDI_CC	"gcc"
  #define UDI_LINK	"ld"
  #define UDI_CCOPTS_DEBUG_FLAGS	"-DDEBUG -Wall -fno-omit-frame-pointer "

  /* Architecture independent compilation flags */
  #define UDI_CCOPTS_ARCHINDEP	"-ffreestanding -fno-builtin -fstrict-prototypes -pedantic -ansi "
  #define UDI_CCOPTS_MAPPER_FLAGS	"-ffreestanding -fno-builtin -fstrict-prototypes"
  #define UDI_LINKOPTS_ARCHINDEP	"-r "

#endif	/* NATIVE_CC */

/* Architecture dependent compilation flags */
#if defined(sparc) || defined(__sparc)
/* SPARC dependent compilation */
  #if !NATIVE_CC
    #define UDI_CCOPTS_ARCHDEP	""
    #define UDI_LINKOPTS_ARCHDEP	""
    #define OS_COMPILE_OPTIMIZE_FLAG	"-O"
    #warning "Hello Sparc Solaris"
  #else	/* NATIVE_CC */
    #define UDI_CCOPTS_ARCHDEP	"-xarch="ARCH " -dalign -Wc,-Qiselect-regsym=0 "
    #define OS_COMPILE_OPTIMIZE_FLAG	"-fast -xO3 -xarch="ARCH ""
    #define UDI_LINKOPTS_ARCHDEP	""
  #endif	/* !NATIVE_CC */
#elif defined(__ia64)
  /* Itanium dependent compilation */
  #define UDI_CCOPTS_ARCHDEP	""
  #define UDI_LINKOPTS_ARCHDEP	""
  #if !NATIVE_CC
    #warning "Hello IA64 solaris"
  #endif	/* !NATIVE_CC */
  #define OS_COMPILE_OPTIMIZE_FLAG	"-O"
#else
  /* Catchall */

  #if defined(i386)
    #if !NATIVE_CC
      #warning "Hello Intel Solaris"
    #endif	/* !NATIVE_CC */
  #endif	/* defined(sparc) || defined(__sparc) */

  #define UDI_CCOPTS_ARCHDEP	""
  #define UDI_LINKOPTS_ARCHDEP	""
  #define OS_COMPILE_OPTIMIZE_FLAG	"-O"
#endif	/* Architecture dependent compilation definitions */

#define UDI_CCOPTS UDI_CCOPTS_ARCHINDEP UDI_CCOPTS_ARCHDEP
#define UDI_CCOPTS_MAPPER UDI_CCOPTS_MAPPER_FLAGS UDI_CCOPTS_ARCHDEP
#define UDI_LINKOPTS UDI_LINKOPTS_ARCHINDEP UDI_LINKOPTS_ARCHDEP
#define	OS_COMPILE_DEBUG_FLAG UDI_CCOPTS_DEBUG_FLAGS

#define OSDEP_PKG_ARCHIVE "/opt/UDI"

#if !defined(NATIVE_CC)
  /* Solaris man page says these come from sys/types.h and sys/stat.h,
     but for GCC compiles this doesn't seem true. */

  extern int stat(const char *path, struct stat *buf);
  extern int lstat(const char *path, struct stat *buf);
  extern int mkdir(const char *path, mode_t mode);

  /* And this should come from sys/wait.h... */
  extern pid_t wait(int *stat_loc);
#endif	/* !defined(NATIVE_CC) */

#include <stdlib.h>
#include <libelf.h>

#if !defined(sparc) && !defined(ia64)
  /*
   * For Solaris, these things do not exist. (??)
   */
  #define	elf64_getehdr(x)	NULL
  #define	elf64_getshdr(x)	NULL
  #define Elf64_Ehdr		Elf32_Ehdr
  #define Elf64_Shdr		Elf32_Shdr
#endif	/* !defined(sparc) && !defined(ia64) */
