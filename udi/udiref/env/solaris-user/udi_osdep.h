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
#undef offsetof
#include <time.h>		/* For CLK_TCK */

/* Use PTHREAD_MUTEX_ERRORCHECK for mutex type */
#define	_POSIX_MUTEX_TYPE	PTHREAD_MUTEX_ERRORCHECK

#include "posix_env.h"

#ifndef _UDI_OSDEP_H
#define _UDI_OSDEP_H

/*
 * ------------------------------------------------------------------
 * Buffer Management Definitions.
 */
#define _OSDEP_INIT_BUF_INFO(osinfo)	
/* Is this a kernel-space buffer? */
#define _OSDEP_BUF_IS_KERNELSPACE(buf)	1


/*
 * Device Node Macros
 */
#define _OSDEP_DEV_NODE_T	struct __osdep_dev_node
#define _OSDEP_DEV_NODE_INIT	_udi_dev_node_init

struct __osdep_dev_node {
	udi_ubit32_t key;
        POSIX_DEV_NODE_T
};

#define _OSDEP_MATCH_DRIVER	_udi_MA_match_children
struct _udi_dev_node;
struct _udi_driver *_udi_MA_match_children(struct _udi_dev_node *child_node);


#define _UDI_PHYSIO_SUPPORTED 1

typedef struct {
        union {
		struct {
                	int addr;
		} io;
                struct {
                        char     	*mapped_addr;
                        int             fd;
                } mem;
        } _u ;
} __osdep_pio_t;
#define _OSDEP_PIO_T __osdep_pio_t

#define _OSDEP_GET_PIO_MAPPING  _udi_get_pio_mapping

#define _OSDEP_RELEASE_PIO_MAPPING _udi_release_pio_mapping

#define _OSDEP_PIO_MAP _udi_osdep_pio_map

typedef long long _OSDEP_PIO_LONGEST_TYPE;
typedef unsigned long long _OSDEP_PIO_LONGEST_UTYPE;
/* 
 * Optional types indicating that we have these.   Beneficial to PIO.
 */
#define _OSDEP_UDI_UBIT64_T unsigned long long
#define _OSDEP_UDI_SBIT64_T signed long long

/*
 * Driver and Static Properties Management
 */

#define _OSDEP_DRIVER_T		_osdep_driver_t

#ifdef KWQ
typedef struct {
	int	num;
	void	*off;
} _udi_sprops_idx_t;

typedef struct {
	_udi_sprops_idx_t *sprops;
} _osdep_driver_t;
#else
typedef struct {
	_udi_sprops_t *sprops;
} _osdep_driver_t;
#define _udi_sprops_idx_t _udi_sprops_t
#endif

/*
 * Debugging macros.
 */
#ifdef lint
#define	_OSDEP_ASSERT(x)
#else
#define _OSDEP_ASSERT(x) assert(x)
#endif	/* lint */

/************************************************************************
 * End of portability macros.
 ************************************************************************/

#define _OSDEP_BRIDGE_RDATA_T	void *

/*
 * Define host endianness.
 */
#if defined(sparc)
#define	_OSDEP_HOST_IS_BIG_ENDIAN	TRUE
#else	/* !defined(sparc) */
#define _OSDEP_HOST_IS_BIG_ENDIAN	FALSE
#endif	/* defined(sparc) */

#endif /* _UDI_OSDEP_H */
