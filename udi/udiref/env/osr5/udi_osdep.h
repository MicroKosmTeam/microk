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
/*
 * Written for SCO by: Rob Tarte
 * At: Pacific CodeWorks, Inc.
 * Contact: http://www.pacificcodeworks.com
 */

#ifndef _UDI_OSDEP_H
#define _UDI_OSDEP_H

#include <sys/types.h>
#include <sys/param.h>
#include <sys/byteorder.h>
#include <sys/clock.h>
#include <sys/debug.h>
#include <sys/ipl.h>
#include <sys/kmem.h>
#include <sys/pci.h>
#include <sys/systm.h>
#include <sys/sysmacros.h>
#include <sys/scsi.h>
#include <sys/devreg.h>
#include <sys/ci/cilock.h>
#include <sys/immu.h>
#include <sys/cmn_err.h>

/* #include <stddef.h>	/* for offsetof() */

int		sleep(caddr_t, int); 
void		splx(int);
int 		splhi();
int 		spl0();
int		timeout(int (*)(), caddr_t, int);
int		untimeout(int);
int		bcopy(caddr_t, caddr_t, int);
int		cmn_err(int, char *, ...);
int		printcfg(char *, unsigned, unsigned, int, int, char *, ...);
void		sptfree(caddr_t, pgcnt_t, int);
int		qincl(caddr_t);
int		wakeup(caddr_t);
void *		udi_memchr(const void *, udi_ubit8_t, udi_size_t);
void		seterror(int);
int		getchar();
void		bzero(caddr_t, int);
int		copyin(caddr_t, caddr_t, int);
int		copyout(caddr_t, caddr_t, int);
uchar_t		inb(udi_ubit32_t);
ushort_t	inw(udi_ubit32_t);
ulong_t		inl(udi_ubit32_t);
void		outb(udi_ubit32_t, uchar_t);
void		outw(udi_ubit32_t, ushort_t);
void		outl(udi_ubit32_t, ulong_t);
void *		sptalloc(pgcnt_t, int, pfn_t, mem_flags_t);
void *		udi_memchr(const void *, udi_ubit8_t, udi_size_t);

void
_udi_driver_init(
);


#define STATIC	static

/* Tell the environment we can do physio */
#define _UDI_PHYSIO_SUPPORTED 1

/************************************************************************
 * Portability macros.
 *
 * Change these definitions to suit your system.
 ************************************************************************/

/*
 * _OSDEP_TIMER_T -- Timer ID type
 *
 * This type is used to hold a timed callback request ID, which can be
 * used to cancel the request.
 */
#define _OSDEP_TIMER_T	int

_OSDEP_TIMER_T
udi_osdep_timer_start(
	udi_timer_expired_call_t *,
	udi_cb_t *,
	uint_t,
	udi_boolean_t
);

int
udi_osdep_timer_cancel(
udi_timer_expired_call_t *,
udi_cb_t *,
_OSDEP_TIMER_T
);

/*
 * ------------------------------------------------------------------
 * Timer definitions.
 *
 */
int	suspend(unsigned int);

#define _OSDEP_TIME_T_TO_TICKS(interval) \
	(((uint_t)Hz * (interval).seconds) + \
	(((uint_t)Hz * ((interval).nanoseconds / 1000)) / (1000 * 1000)))

#define _OSDEP_CURR_TIME_IN_TICKS lbolt

#define _OSDEP_TIMER_START(func, arg, interval, flags) \
		udi_osdep_timer_start((udi_timer_expired_call_t *)func, \
			(udi_cb_t *)arg, interval, 0)

#define _OSDEP_TIMER_CANCEL(func, arg, timer_ID) \
		udi_osdep_timer_cancel((udi_timer_expired_call_t *)func, \
			arg, timer_ID)


/*
 * Timer and clock intervals.
 */
#define _OSDEP_GET_CURRENT_TIME(interval) \
		interval = lbolt

#define _OSDEP_CLK_TCK		   Hz

/*
 * ------------------------------------------------------------------
 * Lock definitions.
 *
 * _OSDEP_MUTEX_T -- Non-recursive mutex lock
 *
 * No calls to potentially blocking functions will be made while holding
 * a lock of this type.  Locks of this type will only be acquired at base
 * level, not at interrupt level.
 */
#define _OSDEP_MUTEX_T	struct _mutex
#define _OSDEP_MUTEX_ALIGN	

/* #define _OSDEP_LOCK_DEBUG */

struct _mutex {
	struct lockb	lock;
	int		saved_pl;

	/*
	 * This is all debug, but we are leaving it in here so that locks
	 * can be instrumented.
	 */
	struct _mutex	*next;
	unsigned	lock_count;
	unsigned	max_lock_time;
	unsigned	min_lock_time;

	unsigned	total_lock_time;
	unsigned	total_lock_time_ns;

	unsigned	lock_start;
	unsigned	lock_start_ns;
	char		*lock_name;
};

/*
 * _OSDEP_MUTEX_INIT() -- Initialize an _OSDEP_MUTEX_T
 *
 * Will not be called at interrupt level.  Will not be called while
 * holding any locks.
 */
#ifdef _OSDEP_LOCK_DEBUG
#define _OSDEP_MUTEX_INIT(mutexp, lock_name) \
	_udi_mutex_init(mutexp, lock_name)
#else
#define _OSDEP_MUTEX_INIT(mutexp, lock_name) \
	{ \
	(mutexp)->lock.ciflag = 0; \
	(mutexp)->lock.ciowner = 0; \
	}
#endif

/*
 * _OSDEP_MUTEX_LOCK() -- Lock an _OSDEP_MUTEX_T (from base level)
 */
#ifdef _OSDEP_LOCK_DEBUG
#define _OSDEP_MUTEX_LOCK(mutexp)	\
	_udi_mutex_lock(mutexp)
#else
#define _OSDEP_MUTEX_LOCK(mutexp)	\
		((mutexp)->saved_pl = ilockb(&((mutexp)->lock)));
#endif

/*
 * _OSDEP_MUTEX_UNLOCK() -- Unlock an _OSDEP_MUTEX_T (from base level)
 *
 * The lock will have been previously locked by _OSDEP_MUTEX_LOCK().
 */
#ifdef _OSDEP_LOCK_DEBUG
#define _OSDEP_MUTEX_UNLOCK(mutexp) \
	_udi_mutex_unlock(mutexp)
#else
#define _OSDEP_MUTEX_UNLOCK(mutexp) \
		(iunlockb(&((mutexp)->lock), (mutexp)->saved_pl));
#endif

#ifdef _OSDEP_LOCK_DEBUG
#define _OSDEP_MUTEX_DEINIT(mutexp) \
	_udi_mutex_deinit(mutexp)
#endif

#ifdef _OSDEP_LOCK_DEBUG
void
_udi_mutex_init(
struct _mutex *,
char *
);

void
_udi_mutex_deinit(
struct _mutex *
);

void
_udi_mutex_lock(
struct _mutex *
);

void
_udi_mutex_unlock(
struct _mutex *
);

void
_udi_dump_locks(
);
#endif

/*
 * ------------------------------------------------------------------
 * Atomic operations
 */
#define _OSDEP_ATOMIC_INT_T	unsigned

#define _OSDEP_ATOMIC_INT_INIT(atomic_int, val)	{ *atomic_int = val; }
#define _OSDEP_ATOMIC_INT_DEINIT(atomic_int)
#define _OSDEP_ATOMIC_INT_READ(atomic_int)	*atomic_int
#define _OSDEP_ATOMIC_INT_INCR(atomic_int)	qincl(atomic_int)
#define _OSDEP_ATOMIC_INT_DECR(atomic_int)	qdecl(atomic_int)

/*
 * ------------------------------------------------------------------
 * Event definitions.
 */
typedef struct _udi_osdep_event {
	udi_ubit32_t	event_state;
	_OSDEP_MUTEX_T	event_lock;
} _udi_osdep_event_t;

void _udi_osdep_event_init(_udi_osdep_event_t *);
void _udi_osdep_event_deinit(_udi_osdep_event_t	*);
void _udi_osdep_event_signal(_udi_osdep_event_t *);
void _udi_osdep_event_wait(_udi_osdep_event_t *);

#define _OSDEP_EVENT_T		_udi_osdep_event_t
#define _OSDEP_EVENT_INIT(ev)	_udi_osdep_event_init(ev)
#define _OSDEP_EVENT_DEINIT(ev)	_udi_osdep_event_deinit(ev)
#define _OSDEP_EVENT_SIGNAL(ev)	_udi_osdep_event_signal(ev)
#define _OSDEP_EVENT_WAIT(ev)	_udi_osdep_event_wait(ev)

/*
 * ------------------------------------------------------------------
 * Region scheduling definitions.
 *
 */

extern udi_queue_t _udi_region_q_head;
extern _OSDEP_MUTEX_T _udi_region_q_lock;
extern _OSDEP_EVENT_T _udi_region_q_event;

#define _OSDEP_REGION_SCHEDULE(region) \
	_udi_osdep_region_schedule(region)

/*
 * ------------------------------------------------------------------
 * PIO definitions.
 */
/* 
 * OS-Dependent portion of the PIO handle.
 */
typedef struct {
	struct pci_devinfo	devinfo;

	union {
		udi_ubit32_t		io_addr;
		udi_ubit8_t		*mapped_addr;
		udi_ubit32_t		offset;
	} _u;

	udi_ubit32_t		npages;
} __osdep_pio_t;

#define _OSDEP_PIO_T __osdep_pio_t

#define _OSDEP_GET_PIO_MAPPING  _udi_get_pio_mapping
#define _OSDEP_PIO_DELAY(usecs) suspend(usecs)

/*
 * ------------------------------------------------------------------
 * DMA definitions.
 */
/* 
 * OS-Dependent portion of the DMA handle.
 */
#define OSDEP_MEM_TYPE_SPTALLOC		1
#define OSDEP_MEM_TYPE_GETCPAGES	2
#define OSDEP_MEM_TYPE_KMEM		3
#define OSDEP_MEM_TYPE_LOCAL		4
#define OSDEP_NUM_MEM_TYPE		4

typedef struct {
	udi_size_t		total_segment_size;
	udi_dma_constraints_attr_t	scgth_vis;
	udi_size_t		phys_dmasize;
	udi_size_t		phys_max_scgth;
	udi_size_t		phys_align;
	udi_size_t		phys_boundary;
	udi_ubit32_t		vaddr;
	udi_ubit32_t		mem_type;
	void			*copy_mem;
} __osdep_dma_t;
#define _OSDEP_DMA_T		__osdep_dma_t *

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
			       struct _udi_alloc_marshal *allocm, int waitflag);

#define _OSDEP_DMA_BUF_MAP_DISCARD(allocm)	\
		_udi_dma_buf_map_discard(allocm)
void _udi_dma_buf_map_discard(struct _udi_alloc_marshal *allocm);

#define _OSDEP_DMA_BUF_UNMAP(dmah)	\
		_udi_dma_buf_unmap(dmah)
void _udi_dma_buf_unmap(struct _udi_dma_handle *dmah);

#define _OSDEP_DMA_SCGTH_FREE(dmah)	\
		_udi_dma_scgth_free(dmah)
void _udi_dma_scgth_free(struct _udi_dma_handle *dmah);

#define _OSDEP_DMA_MEM_ALLOC(dmah, size, flags) \
		_udi_dma_mem_alloc(dmah, size, flags)
void *_udi_dma_mem_alloc(struct _udi_dma_handle *dmah, udi_size_t size,
			 int flags);

#define _UDI_DMA_CANT_WAIT 	  1
#define _UDI_DMA_CANT_USE_INPLACE 2

#if HAVE_USE_INPLACE
int _udi_dma_use_inplace(struct _udi_dma_handle *, int);
#else
#define _udi_dma_use_inplace(dp, wf) _UDI_DMA_CANT_USE_INPLACE
#endif

void _udi_dma_scgth_really_free(struct _udi_dma_handle *);

struct _osdep_udi_freemem {
	struct _osdep_udi_freemem	*next;
	udi_ubit32_t			mem_type;
	udi_ubit32_t			length;
};

#define KMEM_SIZE(size) \
	((size < sizeof(struct _osdep_udi_freemem)) ? \
		sizeof(struct _osdep_udi_freemem) : size)

extern unsigned char	activeintr;
extern int		_udi_init_time;

/*
 * ------------------------------------------------------------------
 * Memory allocation functions.
 *
 * For prototype, zero all allocated memory.
 */
void	*_udi_kmem_alloc(size_t, int, int);
void	_udi_kmem_free(void *);
void	osdep_register_memory(void *);
void	osdep_unregister_memory(void *);
void	_udi_osdep_new_alloc(void);
void	_udi_get_limits_t(udi_limits_t *);

struct udi_mem_stats {
	int		num_alloc;
	int		num_defer_free;
	int		num_free;
	int		pad;
};

extern struct udi_mem_stats	_udi_mem_stats[];

#define _OSDEP_MEM_ALLOC(size, flags, sleep_ok) \
		_udi_kmem_alloc(size, flags, sleep_ok)

#define _OSDEP_MEM_FREE(mem) \
		_udi_kmem_free(mem)

#define _OSDEP_DMA_MEM_ALLOC(dmah, size, flags) \
		_udi_dma_mem_alloc(dmah, size, flags)

#define _OSDEP_NEW_ALLOC() \
		_udi_osdep_new_alloc()

/*
 * ------------------------------------------------------------------
 * SCSI
 */
struct scsi_info {
	struct scsi_info	*next;
	struct Shareg_ex	regp;
	HAINFO			infop;
	_udi_driver_t		*drvp;
};

/*
 * ------------------------------------------------------------------
 * Buffer Management Definitions.
 */
typedef udi_ubit32_t _OSDEP_BUF_T;
#define _OSDEP_INIT_BUF_INFO(osinfo)	/**/
/* Is this a kernel-space buffer? */
#define _OSDEP_BUF_IS_KERNELSPACE(buf)	1


/*
 * _OSDEP_HOST_IS_BIG_ENDIAN -- Is CPU Big-Endian or Little-Endian?
 */
#define _OSDEP_HOST_IS_BIG_ENDIAN	FALSE

extern u_long htonl(u_long hostlong);

/*
 * RSTFIXME - this is a hack
 */
#define strlen	udi_strlen

/*
 * Bridge mapper macros, definitions.
 */
typedef void *	udi_dev_node_t;
#define _OSDEP_INTR_T		_udi_intr_info_t
#define _OSDEP_ISR_RETURN_T	void
#define _OSDEP_ISR_RETURN_VAL(x)
#define _OSDEP_BRIDGE_RDATA_T	_OSDEP_DEV_NODE_T
#define _OSDEP_BRIDGE_CHILD_DATA_T      struct __osdep_dev_node
#define _OSDEP_ENUMERATE_PCI_DEVICE	bridgemap_enumerate_pci_device
#define _OSDEP_REGISTER_ISR(isr, gcb, context, enum_osinfo, attach_osinfo) \
		_udi_register_isr(isr, context, enum_osinfo, attach_osinfo)
#define _OSDEP_UNREGISTER_ISR(isr, gcb, context, enum_osinfo, attach_osinfo) \
		_udi_unregister_isr(enum_osinfo, isr, context, attach_osinfo)

typedef struct _udi_intr_info {
	void			*context;
	int			irq;
	void			(*isr_func)();
	int			pending;
} _udi_intr_info_t;

udi_status_t
_udi_register_isr(
void (*)(),
void *,
void *,
_udi_intr_info_t *
);

udi_status_t _udi_unregister_isr(udi_dev_node_t, void (*)(), void *,
				 _udi_intr_info_t *);

/*
 * Device Node Macros
 */
#define _OSDEP_DEV_NODE_T	struct __osdep_dev_node
#define _OSDEP_DEV_NODE_INIT    _udi_dev_node_init

struct __osdep_dev_node {
	struct pci_devinfo	devinfo;
	int			irq;
};

#define _OSDEP_MATCH_DRIVER	_udi_MA_match_children
struct _udi_dev_node;
struct _udi_driver *_udi_MA_match_children(struct _udi_dev_node *child_node);

/*
 * Instance Attribute Definitions
 */

udi_status_t
_udi_osdep_instance_attr_set(struct _udi_dev_node *node,
					const char *name,
					const void *value,
					udi_size_t length,
					udi_ubit8_t type);

udi_size_t
_udi_osdep_instance_attr_get(struct _udi_dev_node *node,
				const char *name,
				void *value,
				udi_size_t length,
				udi_ubit8_t *type);

#define _OSDEP_INSTANCE_ATTR_SET _udi_osdep_instance_attr_set
#define _OSDEP_INSTANCE_ATTR_GET _udi_osdep_instance_attr_get

udi_sbit32_t _udi_get_instance_index(struct pci_devinfo *);


/*
 * Definitions required for static properties
 */
typedef struct {
	int	num;
	void	*off;
} _udi_sprops_idx_t;

udi_ubit32_t	_udi_osdep_sprops_get_meta_idx (_udi_driver_t *, udi_ubit32_t);

#define _OSDEP_DRIVER_T		_osdep_driver_t

typedef struct _osdep_driver {
	_udi_sprops_idx_t *sprops;
} _osdep_driver_t;

/*
 * Mapper Initialization
 */
#define _OSDEP_MAPPER_INIT(name) {	\
	  /* Do OS-dependent mapper initialization (eg. attaching \
	     an OS-dependent ops vector). */  \
	}


#define _OSDEP_ASSERT(a)	ASSERT(a)

#define _OSDEP_PRINTF		_udi_osdep_printf
#define _OSDEP_TRACE_DATA(S,L)	_udi_osdep_printf("%s\n", (S))
#if 0
#define _OSDEP_LOG_DATA(S,L,CB) (_udi_osdep_printf("%s\n",(S)), 0)
#else
#define _OSDEP_LOG_DATA(S,L,CB) (cmn_err(CE_NOTE, "!%s\n",(S)), 0)
#endif
#define _OSDEP_WRITE_DEBUG(S, L) _udi_osdep_printf("%s", (S))


int	_udi_osdep_printf(const char *, ...);

/************************************************************************
 * End of portability macros.
 ************************************************************************/

#endif /* _UDI_OSDEP_H */
