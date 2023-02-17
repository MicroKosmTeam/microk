/*
 * File: env/freebsd/udi_osdep.h
 *
 * General FreeBSD #includes and #defines.
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

/* Tell the environment we can do physio.   Must be done before any
 * includes of the env.
 */

#define _UDI_PHYSIO_SUPPORTED 1

#include <udi_env.h>
#include <udi_std_sprops.h>

#define _KERNEL 1

#include <sys/libkern.h>
#include <sys/queue.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/lock.h>
#include <machine/atomic.h>

#include <machine/bus.h>
#include <sys/rman.h>
#include <machine/resource.h>

#include <sys/systm.h>

/* 
 * ATOMICITY operators.
 */
#define _OSDEP_ATOMIC_INT_T	int
#define _OSDEP_ATOMIC_INT_INIT(atomic_intp, ival) \
	atomic_set_int(atomic_intp, ival)
#define _OSDEP_ATOMIC_INT_DEINIT(atomic_intp)

/* FIXME: Hey FreeBSD guys, where's the atomic read? */
#define _OSDEP_ATOMIC_INT_READ(atomic_intp)  (*atomic_intp)
#define _OSDEP_ATOMIC_INT_INCR(atomic_intp)  atomic_add_int(atomic_intp, 1)
#define _OSDEP_ATOMIC_INT_DECR(atomic_intp)  atomic_subtract_int(atomic_intp, 1)


/*
 * LOCKING primitives.
 */
typedef struct {
	struct simplelock;
	int ipl;
} _udi_simple_mutex_t;
#define _OSDEP_SIMPLE_MUTEX_T	_udi_simple_mutex_t
#define _OSDEP_SIMPLE_MUTEX_INIT(mutexp, lock_name) simple_lock_init(mutexp)
#define _OSDEP_SIMPLE_MUTEX_LOCK(mutexp) do { (mutexp)->ipl = splhigh(); simple_lock(mutexp.simplelock) } while (0);
#define _OSDEP_SIMPLE_MUTEX_UNLOCK(mutexp) do { simple_unlock(mutexp); splx((mutexp)->ipl); } while (0);
#define _OSDEP_SIMPLE_MUTEX_DEINIT(mutexp)


/*
 * EVENT definitions.
 */
typedef udi_ubit32_t event_t;
#define _OSDEP_EVENT_T udi_ubit32_t

#define _OSDEP_EVENT_INIT(event) *event = 0

#define _OSDEP_EVENT_WAIT(event) \
{ \
	int pl = splhigh(); \
	while (*(event) == 0) { \
		tsleep(event, PZERO|PCATCH, __FILE__, 0); \
	} \
	splx(pl); \
}

#define _OSDEP_EVENT_SIGNAL(event) \
	*(event) = 1; \
	wakeup_one(event) 

#define _OSDEP_EVENT_CLEAR(event) \
	*(event) = 0

/*
 * MEMORY allocator definitions.
 */
void * _udi_malloc(udi_size_t size, int flags, int sleep_ok);
#define _OSDEP_MEM_ALLOC _udi_malloc
#define _OSDEP_MEM_FREE(buf) free(buf, M_DEVBUF)
extern _OSDEP_EVENT_T _udi_alloc_event;
#define _OSDEP_NEW_ALLOC() _OSDEP_EVENT_SIGNAL(&_udi_alloc_event)

/*
 * TIME definitions.
 */
udi_ubit32_t _udi_curr_time_in_ticks(void);
#define _OSDEP_CURR_TIME_IN_TICKS	_udi_curr_time_in_ticks()
#define _OSDEP_GET_CURRENT_TIME(t)	t = _udi_curr_time_in_ticks()
#define _OSDEP_CLK_TCK	hz
#define _OSDEP_TIME_T_TO_TICKS(time_t) \
        (((time_t).nanoseconds / (1000000000/(hz))) + \
         ((time_t).seconds * (hz)))


#define _OSDEP_NANOSEC_TO_TICKS(t)      ((t) / 10000000)
#define _OSDEP_MICROSEC_TO_TICKS(t)     ((t) / 10000)
#define _OSDEP_SEC_TO_TICKS(t)          ((t) * 100)

#define _OSDEP_TIMER_T struct callout_handle
#define _OSDEP_TIMER_START(callback_fn, arg, interval, flags) \
	timeout((timeout_t*) callback_fn, arg, interval)
#define _OSDEP_TIMER_CANCEL(callback_fn, arg, timerID) \
	untimeout((timeout_t*) callback_fn, arg, timerID)


/*
 * DEBUGGING definitions.
 */

#define _OSDEP_ASSERT(expr) if (!(expr)) panic(#expr)
#define _OSDEP_PRINTF printf   /* uprintf is sometimes nice here... */
#define _OSDEP_WRITE_DEBUG(str, len) _OSDEP_PRINTF(str)
#define _OSDEP_DEBUG_BREAK Debugger("udi")

/*
 * TRACING and LOGGING.
 */

#define _OSDEP_TRACE_DATA(S, L) addlog("%s\n", (S))
#define _OSDEP_LOG_DATA(fmtbuf, len, cb) _udi_osdep_log_data(fmtbuf, len, cb)

/*
 * Driver management.
 */

typedef struct {
        struct _udi_std_sprops *sprops;
	/* FIXME: Actually a device_t, but that sucks in too many headers */
        void * device; 
} _osdep_driver_t;
typedef _udi_std_sprops_t _udi_sprops_t;

#define _OSDEP_DRIVER_T _osdep_driver_t

struct __osdep_dev_node {
	struct device * device;
	/* 
	 * We "know" for PCI this is adequate, but how bus_alloc_resource
	 * treats these on non-PCI busses isn't at all clear to me.   The
	 * most we would ever need is 256 of them anyway...
	 *
	 */
	struct resource *bus_resource[6];
	struct resource *irq; /* IRQ resource handle */
	void *intrhand;  /* IRQ handler handle (what does that MEAN?) */
};
#define _OSDEP_DEV_NODE_T struct __osdep_dev_node
#define _OSDEP_DEV_NODE_INIT	_udi_dev_node_init
#define _OSDEP_DEV_NODE_DEINIT	_udi_dev_node_deinit

/*
 * For instance attributes.
 */

udi_status_t _osdep_instance_attr_set(struct _udi_dev_node *node,
                                       const char *name,
                                       const void *value,
                                       udi_size_t length,
                                       udi_ubit8_t type);

udi_size_t
  _osdep_instance_attr_get(struct _udi_dev_node *node,
                            const char *name,
                            void *value,
                            udi_size_t length,
                            udi_ubit8_t *type);

#define _OSDEP_INSTANCE_ATTR_SET _osdep_instance_attr_set
#define _OSDEP_INSTANCE_ATTR_GET _osdep_instance_attr_get


/*
 * For physical I/O.
 */

/*
 * Define host endianness.
 */
#define _OSDEP_HOST_IS_BIG_ENDIAN       FALSE

typedef struct {

	struct device * device;
	struct {
		struct {
			int rid;
			struct resource *res;
			bus_space_tag_t tag;
			bus_space_handle_t handle;
			udi_ubit8_t *mapped_addr;
		} mem;
		struct {
			int rid;
                        int addr;
			struct resource *res;
		} io;
		struct {
                        int offset;
		} cfg;
	} _u;
} __osdep_pio_t;

#define _OSDEP_PIO_T __osdep_pio_t


#define _OSDEP_GET_PIO_MAPPING  _udi_get_pio_mapping
#define _OSDEP_RELEASE_PIO_MAPPING _udi_release_pio_mapping
#define _OSDEP_PIO_DELAY(usecs) DELAY(usecs)

#define _OSDEP_BRIDGE_CHILD_DATA_T struct __osdep_dev_node
#define _udi_intr_info_t void

#define _OSDEP_REGISTER_ISR(isr, gcb, context, enum_osinfo, attach_osinfo) \
                _udi_register_isr(enum_osinfo, isr, context, attach_osinfo)
#define _OSDEP_UNREGISTER_ISR(isr, gcb, context, enum_osinfo, attach_osinfo) \
                _udi_unregister_isr(enum_osinfo, isr, context, attach_osinfo)

#define _OSDEP_ENUMERATE_PCI_DEVICE bridgemap_enumerate_pci_device

/*
 * DMA definitions.
 */
typedef struct {
	void *copy_mem;			/* Our bounce buffer */
	unsigned copy_mem_size;		/* size of our bounce buffer */
	unsigned long low;		/* lowest phys addr */
	unsigned long high;		/* highest phys addr */
	unsigned long alignment;	/* starting phy addr, ^2 */
	unsigned long boundary;		/* don't cross this addr; ^2 */
} _osdep_dma_t;

struct _udi_dma_handle;
struct _udi_alloc_marshal;

#define _OSDEP_DMA_T _osdep_dma_t
#define _OSDEP_DMA_PREPARE(dmah, gcb) 	\
	_udi_dma_prepare(dmah)

udi_status_t _udi_dma_prepare(struct _udi_dma_handle *dmah);

#define _OSDEP_DMA_PREPARE_MIGHT_BLOCK  1

#define _OSDEP_DMA_RELEASE(dmah)        \
                _udi_dma_release(dmah)
void _udi_dma_release(struct _udi_dma_handle *dmah);

#define _OSDEP_DMA_BUF_MAP(dmah, allocm, waitflag)      \
                _udi_dma_buf_map(dmah, allocm, waitflag)
udi_boolean_t _udi_dma_buf_map(struct _udi_dma_handle *dmah,
                               struct _udi_alloc_marshal *allocm,
                               int waitflag);

#define _OSDEP_DMA_BUF_MAP_DISCARD(allocm)      \
                _udi_dma_buf_map_discard(allocm)

#if 0
STATIC void _udi_dma_buf_map_discard(struct _udi_alloc_marshal *allocm);
#endif

#define _OSDEP_DMA_BUF_UNMAP(dmah)      \
                _udi_dma_buf_unmap(dmah)
void _udi_dma_buf_unmap(struct _udi_dma_handle *dmah);

#define _OSDEP_DMA_SCGTH_FREE(dmah)     \
                _udi_dma_scgth_free(dmah)
void _udi_dma_scgth_free(struct _udi_dma_handle *dmah);

#define _OSDEP_DMA_MEM_ALLOC(dmah, size, flags) \
                _udi_dma_mem_alloc(dmah, size, flags)
void *_udi_dma_mem_alloc(struct _udi_dma_handle *dmah,
                         udi_size_t size,
                         int flags);

#define _OSDEP_DMA_MEM_FREE(dmah) \
                /* free(dmah->dma_mem) */

