/*
 * File: src/tools/uw/osdep.h
 *
 * UDI UW specific definitions use in the udi tools.
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

#include <sys/fcntl.h>
#include <sys/cdefs.h>

#include <libelf.h>

#define OS_GETPAX_WITH_CPIO
#define OS_GLUE_PROPS_STRUCT
#define OS_LINK_OVERRIDE

#define UDI_CC "cc"
#define UDI_CCOPTS "-Xc -Kno_host -v"

extern char *svr5_mapper_opts;
#define UDI_CCOPTS_DEFAULT_MAPPER_OPTS "-Xa -Kno_host -v"
#define UDI_CCOPTS_MAPPER \
    getenv("ROOT") ? svr5_mapper_opts : UDI_CCOPTS_DEFAULT_MAPPER_OPTS
extern void uw_override_params(void);
#define OS_OVERRIDE_PARAMS uw_override_params

/*
 * Use 'idld' and not 'ld'.   idld is present in small footprint systems
 * and during ISL.  'ld' is neither.   Both are actually the same binary.
 */
#define UDI_LINK "idld"
#define UDI_LINKOPTS "-r"

#define UDI_DRVFILEDIR	"/etc/conf/pack.d"
#define UDI_NICFILEDIR	"/etc/inst/nd/mdi"

/*
 * We suppress the population of /etc/inst/UDI if we're in a kernel 
 * build env.
 */
#define OS_ALLOW_ARCHIVING (getenv("ROOT") == NULL)
#define OSDEP_PKG_ARCHIVE "/etc/inst/UDI"


/*
 * For UnixWare 7, these things do not exist.
 */
#define	elf64_getehdr(x)	NULL
#define	elf64_getshdr(x)	NULL
#define Elf64_Ehdr		Elf32_Ehdr
#define Elf64_Shdr		Elf32_Shdr

/*
 * Change this to 0 to enable new .bcfg format. If you do this, you will need
 * the updated netcfg + ndcfg changes. This supports more of the custom
 * parameter settings than the old style .bcfg format
 */
#define OLD_BCFG_FORMAT		1

#define OS_CPIO_OPTIONS			if ((int)getuid() == 0) { \
						argv[ii++] = "-Rroot"; \
					}
