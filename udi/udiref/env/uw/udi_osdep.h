
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

/* Control whether we want to track memory leaks */
#define INSTRUMENT_MALLOC 0

/* 
 * Define this if you suspect drivers are writing past the end of 
 * allocated blocks.  If set, OSDEP_MEM_ALLOC will add this number of 
 * "guard" bytes after the allocation set to a fixed pattern and will
 * verify that those bytes are intact when freeing.
 * This is quite expensive at runtime.
 */
#define MEM_ALLOC_PAD_SIZE 0

extern int _udi_force_fail;

/* Control whether we want all (failable) memory requests to fail */

#define _OSDEP_INSTRUMENT_ALLOC_FAIL 	(0x01 & _udi_force_fail)
#define _OSDEP_CALLBACK_LIMIT		((0x02 & _udi_force_fail) ? 1U : 20U)
#define _OSDEP_FORCE_CB_REALLOC		((0x04 & _udi_force_fail) && \
			((ifp->if_meta->meta_idx != 0) && (op_idx != 0)))
#define _OSDEP_DEFER_MEI_CALL(rgn, op_flag)	\
		(/* !UDI_QUEUE_EMPTY(&region->reg_queue) || */ \
		 REGION_IS_ACTIVE(rgn) || (0x08 & _udi_force_fail) || \
			op_flag & UDI_OP_LONG_EXEC)
#if 0 /* Turned off while under development */
#define _OSDEP_FASTQ_MEI_CALL(rgn, op_flag)	\
		((servicing_interrupt() || (0x10 & _udi_force_fail)) && \
		 dtimeout(_udi_run_inactive_region, rgn, 0, pltimeout, \
			  (myengnum | TO_IMMEDIATE)) != 0)
#define _OSDEP_FASTQ_SCHED(rgn)
#endif

/* USE WITH INSTRUMENTED KERNEL ONLY */
#if 0
#define DEBUG
#define _KMEM_STATS
#define _KMEM_HIST
#define _LOCKTEST
#define SPINDEBUG
#endif

#ifdef _KERNEL_HEADERS
#include <mem/kmem.h>
#include <util/ksynch.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <util/debug.h>			/* For ASSERT() */
#include <svc/clock.h>			/* For timer defs. */
#include <util/param_p.h>		/* For HZ */
#include <util/cmn_err.h>		/* For cmn_err arguments */
#include <util/engine.h>		/* For myengnum */
#else
#include <sys/kmem.h>
#include <sys/ksynch.h>
#include <sys/resmgr.h>
#include <sys/systm.h>
#include <sys/debug.h>			/* For ASSERT() */
#include <sys/clock.h>			/* For timer defs. */
#include <sys/param_p.h>		/* For HZ */
#include <sys/cmn_err.h>		/* For cmn_err arguments */
#include <sys/engine.h>			/* For myengnum */
#endif

/*
 * These may be disabled to allow an instrumented UDI environment to be 
 * built at a cost of runtime size and speed.   These are independent of
 * the kernel instrumentation selected above.
 * 
 */

#if 1
/*
 * Maximum performance, minimum size, minimum safety net.
 */
#define NO_UDI_DEBUG
#define _UDI_SUPPRESS_PIO_DEBUGGING 1
#define _OSDEP_ASSERT(a)
#else
/* 
 * For debugging UDI.
 */
#define _OSDEP_ASSERT(a) ASSERT(a)
#endif



/*
 * Normally, we'd like to get these from <sys/xdebug.h>.   The underlying
 * kernel support is present whether KDB is installed or not.   Unfortunately,
 * if the user doesn't have KDB installed, we don't have that header.   So
 * we "borrow" the important pieces from that file
 * "5" is DR_OTHER.   ((int *) 0) is NO_FRAME.
 */

extern void (*volatile cdebugger) ();

#define _OSDEP_DEBUG_BREAK (*cdebugger)(5, ((int *) 0))

/*
 * _OSDEP_MUTEX_T -- Non-recursive mutex lock
 */
#define _OSDEP_MUTEX_T	struct _mutex

struct _mutex {
	lock_t *lock;
	pl_t saved_pl;
};

/*
 * Since we may call OS services while holding a lock, we have to 
 * use the kernel range of lock heirarchy.   For example, it's possible
 * to hold an OSDEP_MUTEX on a PIO regset lock while we call cm_read_pci_word
 * which will itself grab a lock in kernel domain.   To insure that we
 * don't "lower" the lock heirarchy we use "1".
 */
#define _OSDEP_MUTEX_INIT(mutexp, lock_name) \
		{ static LKINFO_DECL(_lkinfo, "IO:UDI:" lock_name, LK_BASIC); \
		(mutexp)->lock = LOCK_ALLOC(1, plhi, &_lkinfo, KM_SLEEP); \
		}

#define _OSDEP_MUTEX_LOCK(mutexp)	\
		((mutexp)->saved_pl = LOCK((mutexp)->lock, plhi))
#define _OSDEP_MUTEX_UNLOCK(mutexp) \
		UNLOCK((mutexp)->lock, (mutexp)->saved_pl)
#define _OSDEP_MUTEX_DEINIT(mutexp) \
		LOCK_DEALLOC((mutexp)->lock)

/*
 * _OSDEP_SIMPLE_MUTEX_T -- Simple non-nesting, non-recursive mutex lock
 */
#define _OSDEP_SIMPLE_MUTEX_T	fspin_t
#define _OSDEP_SIMPLE_MUTEX_INIT(mutexp, lock_name)	FSPIN_INIT(mutexp)
#define _OSDEP_SIMPLE_MUTEX_LOCK(mutexp)		FSPIN_LOCK(mutexp)
#define _OSDEP_SIMPLE_MUTEX_UNLOCK(mutexp)		FSPIN_UNLOCK(mutexp)
#define _OSDEP_SIMPLE_MUTEX_DEINIT(mutexp)

/* 
 * Event handling
 */
#define _OSDEP_EVENT_T		event_t
#define _OSDEP_EVENT_INIT(ev) 	EVENT_INIT(ev)
#define _OSDEP_EVENT_SIGNAL(ev)	EVENT_SIGNAL(ev, 0)
#define _OSDEP_EVENT_WAIT(ev)	EVENT_WAIT(ev, primed)
#define _OSDEP_EVENT_CLEAR(ev) /**/

/*
 * The FSPIN and EVENT family aren't in DDI8.   We like to pretend that
 * we're DDI8 even in spite of this.   So we explictly add protos for these
 * that are copies of the 7.1.1 entries from ksynch.h.
 */
extern void FSPIN_LOCK(fspin_t *);
extern boolean_t FSPIN_TRYLOCK(fspin_t *);
extern void FSPIN_UNLOCK(fspin_t *);

extern void EVENT_WAIT(event_t *,
		       int);
extern void EVENT_INIT(event_t *);
extern void EVENT_SIGNAL(event_t *,
			 int);

extern clock_t drv_usectohz(clock_t);

/*
 * Atomic Integer Macros
 */
#define _OSDEP_ATOMIC_INT_T	atomic_int_t
#define _OSDEP_ATOMIC_INT_INIT(atomic_intp, ival) \
		ATOMIC_INT_INIT(atomic_intp, ival)
#define _OSDEP_ATOMIC_INT_DEINIT(atomic_intp) /**/
#define _OSDEP_ATOMIC_INT_READ(atomic_intp)	ATOMIC_INT_READ(atomic_intp)
#define _OSDEP_ATOMIC_INT_INCR(atomic_intp)	ATOMIC_INT_INCR(atomic_intp)
#define _OSDEP_ATOMIC_INT_DECR(atomic_intp)	ATOMIC_INT_DECR(atomic_intp)

/*
 * Bridge mapper macros, definitions.
 */
#define _OSDEP_INTR_T		_udi_intr_info_t
#define _OSDEP_BRIDGE_RDATA_T	_OSDEP_DEV_NODE_T
#define _OSDEP_BRIDGE_CHILD_DATA_T      rm_key_t
#define _OSDEP_ENUMERATE_PCI_DEVICE	bridgemap_enumerate_pci_device
	typedef struct _udi_intr_info {
	int irq;
	void *cookie;
} _udi_intr_info_t;

#define _OSDEP_REGISTER_ISR(isr, gcb, context, enum_osinfo, attach_osinfo) \
		_udi_register_isr(isr, gcb, context)
#define _OSDEP_UNREGISTER_ISR(isr, gcb, context, enum_osinfo, attach_osinfo) \
		_udi_unregister_isr(isr, gcb, context)

udi_status_t _udi_register_isr(int (*isr) (),
			       udi_cb_t *gcb,
			       void *context);
udi_status_t _udi_unregister_isr(int (*isr) (),
				 udi_cb_t *gcb,
				 void *context);

/*
 * Interrupt routine return value wrapper
 * For UDI_INTR_UNCLAIMED we need to return ISTAT_NONE, 
 * for all CLAIMED interrupts we need to return ISTAT_ASSERTED
 */
#define	_OSDEP_ISR_RETURN_VAL(intr_result)	\
	((intr_result & UDI_INTR_UNCLAIMED) ? ISTAT_NONE : ISTAT_ASSERTED)

/*
 * Device Node Macros
 */
#define _OSDEP_DEV_NODE_T	struct __osdep_dev_node
#define _OSDEP_DEV_NODE_INIT	_udi_dev_node_init
#define _OSDEP_DEV_NODE_DEINIT	_udi_dev_node_deinit
#define _OSDEP_ENUMERATE_DONE	_osdep_udi_enumerate_done

struct __osdep_dev_node {
	rm_key_t rm_key;		/* autoconfig resource manager key */
	int dmachan1;
	int irq;
	ulong_t memaddr;		/* ROM BIOS start address       */
	void *cookie;			/* interrupt cookie             */
	_OSDEP_EVENT_T enum_done_ev;	/* event to signal that enumeration 
					 * for this node is done */
};

#define _OSDEP_MATCH_DRIVER	_udi_MA_match_children
struct _udi_dev_node;
struct _udi_driver *_udi_MA_match_children(struct _udi_dev_node *child_node);
void _osdep_udi_enumerate_done(_udi_dev_node_t *udi_node);

/*
 * Memory allocation definitions.
 */
void _udi_wrapped_free(void *);

#if INSTRUMENT_MALLOC
void *_udi_wrapped_alloc(size_t size,
			 int flags,
			 int sleep,
			 int line,
			 char *f);

#define _OSDEP_MEM_ALLOC(size, flags, sleep_ok) \
		_udi_wrapped_alloc(size, flags, sleep_ok, __LINE__, __FILE__)
#else
void *_udi_wrapped_alloc(size_t size,
			 int flags,
			 int sleep);

#define _OSDEP_MEM_ALLOC(size, flags, sleep_ok) \
		_udi_wrapped_alloc(size, flags, sleep_ok)
#endif
#define _OSDEP_MEM_FREE(mem) \
		_udi_wrapped_free(mem)

/* Allocation daemon related */
extern event_t _udi_alloc_event;

#define _OSDEP_NEW_ALLOC()	EVENT_SIGNAL(&_udi_alloc_event, 0);

/*
 * Timer services from the host OS.
 */
#define _OSDEP_TIMER_T toid_t

#define _OSDEP_TIME_T_TO_TICKS(interval) \
		(drv_usectohz(((interval).nanoseconds + 999)/1000) + \
			(interval).seconds * drv_usectohz(1000000))

#define _OSDEP_CURR_TIME_IN_TICKS TICKS()
#define _OSDEP_TIMER_START(callback_fn, arg, interval, flags) \
	itimeout((void (*)())callback_fn, arg, \
		 interval, PLTIMEOUT)
#define _OSDEP_TIMER_CANCEL(callback_fn, arg, timerID) untimeout(timerID)

#define _OSDEP_GET_CURRENT_TIME(t) t = TICKS()

/* 
 * Use the default _OSDEP_SUBTRACT_TIME.
 * Shipping a new UDI supplement (only an issues for environments that
 * aren't integrated into the UW kernel source tree) if HZ changes is 
 * deemed to be an acceptable, yet unlikely, risk.
 */
#define _OSDEP_CLK_TCK HZ

extern udi_queue_t _udi_region_q_head;
extern _OSDEP_MUTEX_T _udi_region_q_lock;
extern _OSDEP_EVENT_T _udi_region_q_event;

/*
 * NOTE: the REGION_LOCK is an _OSDEP_SIMPLE_MUTEX_LOCK so we cannot hold it
 *	 while taking an _OSDEP_MUTEX_LOCK. This means we have to drop, and
 *	 reacquire the lock to maintain lock hierarchy rules
 */
#define _OSDEP_REGION_SCHEDULE(region) \
	region->in_sched_q = TRUE; \
	REGION_UNLOCK(region); \
	_OSDEP_MUTEX_LOCK(&_udi_region_q_lock); \
	UDI_ENQUEUE_TAIL(&_udi_region_q_head, &region->reg_sched_link);\
	_OSDEP_MUTEX_UNLOCK(&_udi_region_q_lock); \
	_OSDEP_EVENT_SIGNAL(&_udi_region_q_event); \


/* 
 * PIO definitions.
 */
typedef struct {
	rm_key_t cfg_key;
	union {
		struct {
			int addr;
		} io;
		struct {
			caddr_t mapped_addr;
		} mem;
		struct {
			size_t offset;
		} cfg;
	} _u;
} __osdep_pio_t;

#define _OSDEP_PIO_T __osdep_pio_t

#define _OSDEP_GET_PIO_MAPPING  _udi_get_pio_mapping
#define _OSDEP_RELEASE_PIO_MAPPING _udi_release_pio_mapping
#define _OSDEP_PIO_DELAY(usecs) drv_usecwait(usecs)

/* 
 * Let PIO trans handle up to 64 bit types by itself.
 */
#define _OSDEP_PIO_LONGEST_STYPE        signed long long
#define _OSDEP_PIO_LONGEST_UTYPE        unsigned long long

/*
 * DMA definitions.
 */
typedef struct {
	physreq_t *physreq;
	physreq_t *scgth_physreq;
	udi_ubit32_t scgth_prefix_bytes;
	void *copy_mem;
} __osdep_dma_t;

#define _OSDEP_DMA_T		__osdep_dma_t

struct _udi_dma_handle;
struct _udi_alloc_marshal;

#define _OSDEP_DMA_PREPARE(dmah,gcb)	\
		_udi_dma_prepare(dmah)
udi_status_t _udi_dma_prepare(struct _udi_dma_handle *dmah);

#define _OSDEP_DMA_PREPARE_MIGHT_BLOCK	1

#define _OSDEP_DMA_RELEASE(dmah) 	\
		_udi_dma_release(dmah)
void _udi_dma_release(struct _udi_dma_handle *dmah);

#define _OSDEP_DMA_BUF_MAP(dmah, allocm, waitflag)	\
		_udi_dma_buf_map(dmah, allocm, waitflag)
udi_boolean_t _udi_dma_buf_map(struct _udi_dma_handle *dmah,
			       struct _udi_alloc_marshal *allocm,
			       int waitflag);

#define _OSDEP_DMA_BUF_MAP_DISCARD(allocm)	\
		_udi_dma_buf_map_discard(allocm)
STATIC void _udi_dma_buf_map_discard(struct _udi_alloc_marshal *allocm);

#define _OSDEP_DMA_BUF_UNMAP(dmah)	\
		_udi_dma_buf_unmap(dmah)
void _udi_dma_buf_unmap(struct _udi_dma_handle *dmah);

#define _OSDEP_DMA_SCGTH_FREE(dmah)	\
		_udi_dma_scgth_free(dmah)
void _udi_dma_scgth_free(struct _udi_dma_handle *dmah);

#define _OSDEP_DMA_MEM_ALLOC(dmah, size, flags) \
		_udi_dma_mem_alloc(dmah, size, flags)
void *_udi_dma_mem_alloc(struct _udi_dma_handle *dmah,
			 udi_size_t size,
			 int flags);

#define _OSDEP_DMA_MEM_FREE(dmah) \
		kmem_free(dmah->dma_mem, dmah->dma_maplen)

#define _UDI_DMA_CANT_WAIT 	  1
#define _UDI_DMA_CANT_USE_INPLACE 2
#if HAVE_USE_INPLACE
int _udi_dma_use_inplace(struct _udi_dma_handle *,
			 int);
#else
#define _udi_dma_use_inplace(dp, wf) _UDI_DMA_CANT_USE_INPLACE
#endif

void _udi_dma_scgth_really_free(struct _udi_dma_handle *);

#define _OSDEP_DRIVER_T		_osdep_driver_t
typedef struct {
	struct _udi_std_sprops *sprops;
	void *drvinfo; 			/* Actually a drvinfo_t */
} _osdep_driver_t;

/*
 * sysdump handling routines
 */

int _udi_sysdump_alloc(_udi_dev_node_t *parent_node,
		       void **intr_cookie);
void _udi_sysdump_free(void **intr_cookie);

/*
 * Internal debugging (only) output machinery.
 * Kernel 'printf' is not an exposed interface (it's base) but it's
 * believed to be stable and avoids a slew of calling convention issues
 * if we try to use cmn_err.
 */
#define _OSDEP_PRINTF_LONG_LONG_SPEC	"%08Lx"
#define _OSDEP_PRINTF			printf

/* 
 * Trace data is handed a preformatted string.  
 */
#define _OSDEP_TRACE_DATA(S, L) cmn_err(CE_CONT, "%s\n", (S))
#define _OSDEP_WRITE_DEBUG(str, len) cmn_err(CE_CONT, "%s", (str))
#define _OSDEP_LOG_DATA(fmtbuf, len, cb) _udi_osdep_log_data(fmtbuf, len, cb)

/*
 * Instance Attribute Definitions
 */
udi_status_t __osdep_instance_attr_set(struct _udi_dev_node *node,
				       const char *name,
				       const void *value,
				       udi_size_t length,
				       udi_ubit8_t type);

udi_size_t
  __osdep_instance_attr_get(struct _udi_dev_node *node,
			    const char *name,
			    void *value,
			    udi_size_t length,
			    udi_ubit8_t *type);

_udi_dev_node_t *_udi_find_node_by_rmkey(rm_key_t);

#define _OSDEP_INSTANCE_ATTR_SET __osdep_instance_attr_set
#define _OSDEP_INSTANCE_ATTR_GET __osdep_instance_attr_get

/*
 * These definitions are here because environment uses these udi interfaces.
 * environment probably should have used _OSDEP_* of these but...
 */

#define udi_strcat	strcat
#define udi_strcmp	strcmp
#define udi_strncmp	strncmp
#define udi_memcmp	bcmp
#define udi_strcpy	strcpy
#define udi_strncpy	strncpy
#define udi_strchr	strchr
#define udi_strtou32	strtoul
#define udi_strlen	strlen

/*
 * Define host endianness.
 */
#define _OSDEP_HOST_IS_BIG_ENDIAN	FALSE

#endif /* _UDI_OSDEP_H */
