#undef offsetof

#include "posix_env.h"

   /* These will be in <pthread.h> on the release that follows 7.1.0 */
   int pthread_mutexattr_gettype(pthread_mutexattr_t *attr, int *type);
   int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);


/*
 *	Copyright 1996 The Santa Cruz Operation, Inc.  All Rights Reserved.
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF SCO, Inc.
 *	The copyright notice above does not evidence any
 *	actual or intended publication of such source code.
 */

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
/*
 	rm_key_t key;
 */
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
                        udi_ubit8_t     *mapped_addr;
                        int             fd;
                } mem;
        } _u ;
} __osdep_pio_t;
#define _OSDEP_PIO_T __osdep_pio_t

#define _OSDEP_GET_PIO_MAPPING  _udi_get_pio_mapping

/*
#define _OSDEP_RELEASE_PIO_MAPPING _udi_release_pio_mapping
*/

/*
#define _OSDEP_PIO_MAP _udi_osdep_pio_map
*/

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

/*
 * Unfortunately, we can't use the library usleep() becuase it interferes
 * with our synthetic timers.   The nap library prototype is also missing
 * from the system headers.
 */
/*
long nap(long period);
#define usleep(x) nap(x/1000)
*/

/************************************************************************
 * End of portability macros.
 ************************************************************************/


#define _OSDEP_BRIDGE_RDATA_T	void *

/*
 * Define host endianness.
 */
#define _OSDEP_HOST_IS_BIG_ENDIAN	FALSE

#endif /* _UDI_OSDEP_H */
