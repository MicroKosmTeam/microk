/*
 * File: env/linux-user/udi_osdep.h
 *
 * Include file for configuring the posix-Linux environment.
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

/* These are required to get UNIX98 POSIX semaphore support w/GNU extensions. */ 
#define _GNU_SOURCE   1
#define _REENTRANT    1
#define _THREAD_SAFE  1

#undef offsetof

/*
 * On 64-bit architectures, (Alpha, Sparc), this should be "%016llx".
 */
#if defined(__alpha__) || defined(__sparc__)
#define _OSDEP_PRINTF_LONG_LONG_SPEC "%016llx"
#else
#define _OSDEP_PRINTF_LONG_LONG_SPEC "%08lx"
#endif

#include <pthread.h>
#  define PTHREAD_MUTEX_ERRORCHECK PTHREAD_MUTEX_ERRORCHECK_NP
#  define PTHREAD_MUTEX_NORMAL  PTHREAD_MUTEX_ERRORCHECK_NP
#if 0
#  define pthread_mutexattr_settype(M,T) pthread_mutexattr_setkind_np(M,T)/* Linux uses setkind; not needed*/
#endif

/* glibc features file. */
#include <features.h>

#if ( (__GLIBC__ << 16) | __GLIBC_MINOR__ ) > 0x020000
/*
 * Linux uses GNU libc which provides a convenient debugging extension
 * for providing stack traces on our allocated resources.   These must
 * be defined before the include of posix_env.h.
 */
#include <execinfo.h>

#define STACK_TRACE_DEPTH 5

#define _POSIX_OSDEP_MBLK_DATA \
	void *backtrace_symbols[STACK_TRACE_DEPTH]; \
	int num_symbols;

#define _POSIX_OSDEP_INIT_MBLK(mblk) \
	mblk->num_symbols = \
		backtrace(mblk->backtrace_symbols, STACK_TRACE_DEPTH);

#define _POSIX_OSDEP_DEINIT_MBLK(mblk) \

#if 1
#define _POSIX_OSDEP_DUMP_MBLK(mblk) \
	{ \
		int i; \
		FILE *p; \
		char cmd[100]; \
		snprintf(cmd, sizeof(cmd), \
			"addr2line --basenames -e %s", posix_me); \
		p = popen(cmd, "w"); \
		for (i=1;i<mblk->num_symbols-1;i++) { \
			fprintf(p,"%p\n", mblk->backtrace_symbols[i]); \
		} \
		pclose(p); \
	}
#else
#define _POSIX_OSDEP_DUMP_MBLK(mblk) \
	{ \
		int i; \
		char **sym = backtrace_symbols(mblk->backtrace_symbols, \
						 STACK_TRACE_DEPTH); \
		for (i=0;i<mblk->num_symbols;i++) { \
			fprintf(stdout, "  %s\n", sym[i]); \
		} \
		free (sym);\
	}
#endif
	
#endif /* glibc > 2.0 */

/*
 * Override things that the POSIX environment would otherwise
 * provide for free.
 */
/* Enable userland PCI device matching... */
#define POSIXUSER_ENUMERATE_PCI_DEVICES posixuser_enumerate_pci_devices
#define POSIXUSER_NUM_PCI_DEVICES posixuser_num_pci_devices

#include "posix_env.h"

#ifndef _UDI_OSDEP_H
#define _UDI_OSDEP_H

/* #define _OSDEP_CALLBACK_LIMIT 3 */


/*
 * ------------------------------------------------------------------
 * Buffer Management Definitions.
 */
#define _OSDEP_INIT_BUF_INFO(osinfo)
/* Is this a kernel-space buffer? */
#define _OSDEP_BUF_IS_KERNELSPACE(buf)  1


/*
 * Device Node Macros
 */
#define _OSDEP_DEV_NODE_INIT    linux_posix_dev_node_init
#define _OSDEP_DEV_NODE_DEINIT  linux_posix_dev_node_deinit
#define _OSDEP_DEV_NODE_T       struct __osdep_dev_node

#define _OSDEP_MATCH_DRIVER	_udi_MA_match_children
struct _udi_dev_node;
struct _udi_driver *_udi_MA_match_children(struct _udi_dev_node *child_node);
int linux_posix_dev_node_init(struct _udi_driver *driver,
			struct _udi_dev_node *udi_node);
int linux_posix_dev_node_deinit(struct _udi_dev_node *udi_node);

struct __osdep_dev_node {
        int key;
        POSIX_DEV_NODE_T
};

#define _OSDEP_GET_PIO_MAPPING  _udi_get_pio_mapping
#define _OSDEP_RELEASE_PIO_MAPPING _udi_release_pio_mapping

#define _OSDEP_PIO_MAP _udi_osdep_pio_map


/*
 * Driver Management
 */

#define _OSDEP_DRIVER_T _osdep_driver_t
typedef struct {
    _udi_sprops_t *sprops;
} _osdep_driver_t;


/*
 * Debugging macros.
 */
#define _OSDEP_ASSERT assert
 
#define _ASSERT    _OSDEP_ASSERT
#define ASSERT     _OSDEP_ASSERT


/*
 * PIO configuration
 */
typedef long long _OSDEP_LONGEST_PIO_TYPE;
typedef unsigned long long _OSDEP_LONGEST_PIO_UTYPE;
#define _OSDEP_UDI_UBIT64_T unsigned long long
#define _OSDEP_UDI_SBIT64_T signed long long

#define _UDI_PHYSIO_SUPPORTED 1

typedef struct {
	unsigned char pci_busnum, pci_devnum, pci_fnnum;
        union {
	    struct {
                int addr;
	    } io;
	    struct {
		udi_ubit8_t *mapped_addr;
		int fd;
	    } mem;
	    struct {
		int fd;
	    } cfg;
        } _u ;
} __osdep_pio_t;
#define _OSDEP_PIO_T __osdep_pio_t


/*
 * Define host endianness.
 */
#include <endian.h>
#define _OSDEP_HOST_IS_BIG_ENDIAN (__BIG_ENDIAN == __BYTE_ORDER)

/************************************************************************
 * End of portability macros.
 ************************************************************************/
 
#endif /* _UDI_OSDEP_H */
