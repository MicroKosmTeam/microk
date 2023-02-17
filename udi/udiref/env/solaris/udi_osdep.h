
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


#ifndef _UDI_OSDEP_H
#define _UDI_OSDEP_H

#ifndef _KERNEL
#define _KERNEL
#endif

#ifndef STATIC
#define STATIC static
#endif

/* Tell the environment we can do physio */
#define _UDI_PHYSIO_SUPPORTED 1
/*
 * NOTE: The following trickery is needed to avoid conflicting with the
 *	 declaration of _udi_get_pio_mapping present in common/udi_env.h
 *	 As this function is OS-specific, I could argue that there should be
 *	 no declaration in the common code.
 */
#define _OSDEP_GET_PIO_MAPPING(pio, dev)	\
	_udi_OS_get_pio_mapping(pio, dev, allocm->_u.pio_map_request.attributes)

/* Control whether we want to track memory leaks */
#define INSTRUMENT_MALLOC 0

extern int _udi_force_fail;

/* Control whether we want all (failable) memory requests to fail */

#define _OSDEP_INSTRUMENT_ALLOC_FAIL	(0x01 & _udi_force_fail)
#define _OSDEP_CALLBACK_LIMIT		(0x02 & _udi_force_fail ? 1 : 20)
#define _OSDEP_FORCE_CB_REALLOC		((0x04 & _udi_force_fail) && \
			((ifp->if_meta->meta_idx != 0) && (op_idx != 0)))

#define _OSDEP_DEFER_MEI_CALL(rgn, opflags)	\
		(/* !UDI_QUEUE_EMPTY(&region->reg_queue) || */ \
		 REGION_IS_ACTIVE(rgn) || (0x08 & _udi_force_fail))

#include <sys/proc.h>			/* For p0 reference	*/
#include <sys/disp.h>			/* For minclsyspri	*/
#include <sys/kmem.h>
#include <sys/ksynch.h>
#include <sys/debug.h>			/* For ASSERT() 	*/
#include <sys/param.h>			/* For HZ		*/
#include <sys/cmn_err.h>		/* For cmn_err arguments*/
#include <sys/pci.h>			/* For PCI_CONF_xxx	*/
#include <sys/ddi.h>
#include <sys/sunddi.h>

#define	_OSDEP_ASSERT(a)	ASSERT(a)
#define _OSDEP_DEBUG_BREAK	debug_enter("UDI ")
/*
 * _OSDEP_MUTEX_T -- Non-recursive mutex lock
 */
#define _OSDEP_MUTEX_T	kmutex_t

/*
 * Initially we simply map _OSDEP_MUTEX_* to the kernel mutex_* equivalents.
 * We may wish to revisit this for the _OSDEP_SIMPLE_MUTEX cases, as these may
 * need to become spin locks instead of MUTEX_ADAPTIVE. For the initial case
 * we'll simply keep SIMPLE_MUTEX the same as MUTEX.
 */
#define	_OSDEP_MUTEX_INIT(mutexp, lock_name) \
		mutex_init(mutexp, NULL, MUTEX_ADAPTIVE, NULL)

#define _OSDEP_MUTEX_LOCK(mutexp)	\
		mutex_enter(mutexp)

#define _OSDEP_MUTEX_UNLOCK(mutexp)	\
		mutex_exit(mutexp)

#define _OSDEP_MUTEX_DEINIT(mutexp)	\
		mutex_destroy(mutexp)

/*
 * Atomic integer manipulation - we need this because the common udi_buf and
 * udi_dma code make extensive use of these artefacts. There really should be
 * a better way of doing this ...
 */
typedef struct __udi_atomic_int {
	krwlock_t	amutex;
	int		aval;
} _osdep_atomic_int_t;

extern void
_osdep_atomic_int_init( _osdep_atomic_int_t *atomic_intp, int ival );

extern void
_osdep_atomic_int_incr( _osdep_atomic_int_t *atomic_intp );

extern void
_osdep_atomic_int_decr( _osdep_atomic_int_t *atomic_intp );

extern int
_osdep_atomic_int_read( _osdep_atomic_int_t *atomic_intp );

extern void
_osdep_atomic_int_deinit( _osdep_atomic_int_t *atomic_intp );

#define	_OSDEP_ATOMIC_INT_T	_osdep_atomic_int_t
#define	_OSDEP_ATOMIC_INT_INIT	_osdep_atomic_int_init
#define	_OSDEP_ATOMIC_INT_INCR	_osdep_atomic_int_incr
#define	_OSDEP_ATOMIC_INT_DECR	_osdep_atomic_int_decr
#define	_OSDEP_ATOMIC_INT_READ	_osdep_atomic_int_read
#define	_OSDEP_ATOMIC_INT_DEINIT _osdep_atomic_int_deinit

/*
 * Event handling
 */

#define	_OSDEP_EVENT_T	struct _osdep_event_t

struct _osdep_event_t {
	kcondvar_t	cv;		/* Condition variable	*/
	kmutex_t	mutex;		/* Access mutex		*/
	unsigned int	flag;		/* Flag value		*/

};

#define _OSDEP_EVENT_INIT(ev)					\
	{							\
	  mutex_init(ev.mutex, NULL, MUTEX_DRIVER, NULL);	\
	  cv_init(ev.cv, NULL, CV_DRIVER, NULL);		\
	  (*ev).flag = 0;					\
	}
#define _OSDEP_EVENT_SIGNAL(ev)					\
	{							\
	  mutex_enter(ev.mutex);				\
	  (*ev).flag = 1;					\
	  cv_signal(ev.cv);					\
	  mutex_exit(ev.mutex);					\
	}
#define	_OSDEP_EVENT_WAIT(ev)					\
	{							\
	  mutex_enter(ev.mutex);				\
	  while ( ! (*ev).flag ) {				\
	    cv_wait(ev.cv, ev.mutex);				\
	  }							\
	  (*ev).flag = 0;					\
	  mutex_exit(ev.mutex);					\
	}
#define	_OSDEP_EVENT_CLEAR(ev)	/**/
#define	_OSDEP_EVENT_DEINIT(ev)					\
	{							\
	  cv_destroy(ev.cv);					\
	  mutex_destroy(ev.mutex);				\
	}

/*
 * Bridge mapper macros, definitions
 */
#define	_OSDEP_INTR_T			_udi_intr_info_t

typedef struct _udi_intr_info {
	int irq;
	void *cookie;
} _udi_intr_info_t;

#define	_OSDEP_ENUMERATE_PCI_DEVICE	bridgemap_enumerate_pci_device
#define	_OSDEP_BRIDGE_CHILD_DATA_T	void *

/*
 * Define host endianness.
 */
#if defined(sparc)
#define _OSDEP_HOST_IS_BIG_ENDIAN	TRUE
#else	/* !defined(sparc) */
#define _OSDEP_HOST_IS_BIG_ENDIAN	FALSE
#endif	/* defined(sparc) */

/*
 * Driver Management
 */
typedef struct _udi_sprops_idx {
	int num;
	void *off;
} _udi_sprops_idx_t;

typedef struct _osdep_driver {
	_udi_sprops_idx_t	*sprops;
} _osdep_driver_t;

#define	_OSDEP_DRIVER_T	_osdep_driver_t

/*
 * Clock definitions and Timer services from the host OS
 */
/*
 * Timer services from the host OS.
 */
#define _OSDEP_TIMER_T timeout_id_t

#define _OSDEP_TIME_T_TO_TICKS(interval) \
	   drv_usectohz((interval).nanoseconds/1000 + \
			(interval).seconds * 1000 * 1000)
#define _OSDEP_CURR_TIME_IN_TICKS ddi_get_lbolt()
#define _OSDEP_TIMER_START(callback_fn, arg, interval, flags) \
	timeout(callback_fn, arg, interval)
#define _OSDEP_TIMER_CANCEL(callback_fn, arg, timerID) untimeout(timerID)

#define _OSDEP_GET_CURRENT_TIME(t) t = ddi_get_lbolt()

/* 
 * Obtain the HZ value by dividing the number of usecs in one Hz into the
 * number of usecs in one second
 */
#define _OSDEP_CLK_TCK		(1000000 / drv_hztousec(1))

/*
 * Memory allocation definitions.
 */
void _udi_wrapped_free(void *);

#if INSTRUMENT_MALLOC
void *_udi_wrapped_alloc(size_t size,
			 int	flags,
			 int	sleep,
			 int	line,
			 char	*f);

#define	_OSDEP_MEM_ALLOC(size, flags, sleep_ok) \
		_udi_wrapped_alloc(size, flags, sleep_ok, __LINE__, __FILE__)
#else
void *_udi_wrapped_alloc(size_t size,
			 int	flags,
			 int	sleep);

#define	_OSDEP_MEM_ALLOC(size, flags, sleep_ok) \
		_udi_wrapped_alloc(size, flags, sleep_ok)
#endif	/* INSTRUMENT_MALLOC */

#define	_OSDEP_MEM_FREE(mem) \
		_udi_wrapped_free(mem)

/* Allocation daemon related */
extern _OSDEP_EVENT_T	_udi_alloc_event;

#define	_OSDEP_NEW_ALLOC()	_OSDEP_EVENT_SIGNAL(&_udi_alloc_event)

#endif  /* _UDI_OSDEP_H */
