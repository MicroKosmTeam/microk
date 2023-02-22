
/*
 * File: env/common/udi_osdep_defaults.h
 *
 * Defaults for OS-dependent hooks in UDI Environment implementations.
 * Each OS provides its own udi_osdep.h, which must be included before
 * this file, so it can override any of the default definitions here.
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

#ifndef _UDI_OSDEP_DEFAULTS_H
#define _UDI_OSDEP_DEFAULTS_H

/*
 * _OSDEP_PRINTF -- Debugging printf
 */
#ifndef _OSDEP_PRINTF
#ifdef DEBUG
#define _OSDEP_PRINTF	printf
#else
#define	_OSDEP_PRINTF	(void)
#endif
#endif

/*
 * _OSDEP_ASSERT -- Assertion check
 */
#ifndef _OSDEP_ASSERT
#define _OSDEP_ASSERT(expr) \
		((void)((expr) || _OSDEP_PRINTF("ASSERT: %s\n" #expr)))
#endif

/*
 * _OSDEP_DEBUG_BREAK -- Debugging breakpoint
 */
#ifndef _OSDEP_DEBUG_BREAK
# define _OSDEP_DEBUG_BREAK		/* */
#endif

/*
 * _OSDEP_MUTEX_T -- Non-recursive mutex lock
 *
 * The synchronization abstraction supports two different types of mutex
 * locks, _OSDEP_MUTEX_T and _OSDEP_SIMPLE_MUTEX_T. Many OSes define both
 * of these to be the same, but some can use a lighter-weight mutex under
 * certain conditions.
 *
 * Usage of _OSDEP_MUTEX_T locks must conform to the following usage rules:
 * _OSDEP_MUTEX_T locks do not nest; they may not be acquired while holding
 * another _OSDEP_MUTEX_T lock. However, an _OSDEP_SIMPLE_MUTEX_T may be
 * acquired while holding an _OSDEP_MUTEX_T. OS functions that might block
 * must not be called while holding an _OSDEP_MUTEX_T. _OSDEP_MUTEX_T locks
 * may be acquired from interrupt and base levels.
 */
#ifndef _OSDEP_MUTEX_T
#define _OSDEP_MUTEX_T	char
#endif

/*
 * _OSDEP_MUTEX_INIT() -- Initialize an _OSDEP_MUTEX_T
 *
 * This must be called after (statically or dynamically) allocating an
 * _OSDEP_MUTEX_T before using it. Must not be called from interrupt level or
 * while holding any locks.
 */
#ifndef _OSDEP_MUTEX_INIT
#define _OSDEP_MUTEX_INIT(mutexp, lock_name)
#endif

/*
 * _OSDEP_MUTEX_DEINIT() -- De-initialize an _OSDEP_MUTEX_T
 *
 * This must be called before freeing an _OSDEP_MUTEX_T. Must not be
 * called from interrupt level or while holding any locks.
 */
#ifndef _OSDEP_MUTEX_DEINIT
#define _OSDEP_MUTEX_DEINIT(mutexp)
#endif

/*
 * _OSDEP_MUTEX_LOCK() -- Lock an _OSDEP_MUTEX_T
 *
 * If necessary to prevent a nested call to _OSDEP_MUTEX_LOCK()
 * from being called at interrupt level, must disable interrupts.
 */
#ifndef _OSDEP_MUTEX_LOCK
#define _OSDEP_MUTEX_LOCK(mutexp)
#endif

/*
 * _OSDEP_MUTEX_UNLOCK() -- Unlock an _OSDEP_MUTEX_T
 *
 * The lock must have been previously locked by _OSDEP_MUTEX_LOCK().
 * If _OSDEP_MUTEX_LOCK() disabled interrupts, this must re-enable them.
 */
#ifndef _OSDEP_MUTEX_UNLOCK
#define _OSDEP_MUTEX_UNLOCK(mutexp)
#endif

/*
 * _OSDEP_SIMPLE_MUTEX_T -- Simple non-nesting, non-recursive mutex lock
 *
 * This is an optimized version of an _OSDEP_MUTEX_T.  It may be implemented
 * identically to _OSDEP_MUTEX_T, or the implementation may take advantage of
 * the additional restrictions on its use.
 *
 * No locks of any type may be acquired while holding an _OSDEP_SIMPLE_MUTEX_T.
 * OS functions that might block must not be called while holding an
 * _OSDEP_SIMPLE_MUTEX_T. _OSDEP_SIMPLE_MUTEX_T locks may be acquired from
 * interrupt and base levels.
 */

#ifndef _OSDEP_SIMPLE_MUTEX_T
#define _OSDEP_SIMPLE_MUTEX_T	_OSDEP_MUTEX_T
#endif

/*
 * _OSDEP_SIMPLE_MUTEX_INIT() -- Initialize an _OSDEP_SIMPLE_MUTEX_T
 *
 * This must be called after (statically or dynamically) allocating an
 * _OSDEP_SIMPLE_MUTEX_T before using it. Must not be called from interrupt
 * level or while holding any locks.
 */
#ifndef _OSDEP_SIMPLE_MUTEX_INIT
#define _OSDEP_SIMPLE_MUTEX_INIT	_OSDEP_MUTEX_INIT
#endif

/*
 * _OSDEP_SIMPLE_MUTEX_DEINIT() -- De-initialize an _OSDEP_SIMPLE_MUTEX_T
 *
 * This must be called before freeing an _OSDEP_SIMPLE_MUTEX_T. Must not be
 * called from interrupt level or while holding any locks.
 */
#ifndef _OSDEP_SIMPLE_MUTEX_DEINIT
#define _OSDEP_SIMPLE_MUTEX_DEINIT	_OSDEP_MUTEX_DEINIT
#endif

/*
 * _OSDEP_SIMPLE_MUTEX_LOCK() -- Lock a simple mutex
 *
 * If necessary to prevent a nested call to _OSDEP_SIMPLE_MUTEX_LOCK()
 * from being called at interrupt level, must disable interrupts.
 */
#ifndef _OSDEP_SIMPLE_MUTEX_LOCK
#define _OSDEP_SIMPLE_MUTEX_LOCK	_OSDEP_MUTEX_LOCK
#endif

/*
 * _OSDEP_SIMPLE_MUTEX_UNLOCK() -- Unlock a simple mutex
 *
 * The lock must have been previously locked by _OSDEP_SIMPLE_MUTEX_LOCK().
 * If _OSDEP_SIMPLE_MUTEX_LOCK() disabled interrupts, this must re-enable them.
 */
#ifndef _OSDEP_SIMPLE_MUTEX_UNLOCK
#define _OSDEP_SIMPLE_MUTEX_UNLOCK	_OSDEP_MUTEX_UNLOCK
#endif

/* 
 * _OSDEP_EVENT_T -- Event synchronization variable
 *
 * Event variables are used to block a thread waiting for a specific event
 * to occur.
 */
#ifndef _OSDEP_EVENT_T
#define _OSDEP_EVENT_T		volatile int
#endif

/*
 * _OSDEP_EVENT_INIT -- Initialize an event variable
 */
#ifndef _OSDEP_EVENT_INIT
#define _OSDEP_EVENT_INIT(ev)	((ev) = 0)
#endif

/*
 * _OSDEP_EVENT_DEINIT -- De-initialize an event variable
 */
#ifndef _OSDEP_EVENT_DEINIT
#define _OSDEP_EVENT_DEINIT(ev)
#endif

/*
 * _OSDEP_EVENT_WAIT -- Wait for an event to be signalled
 *
 * This call blocks the current thread until _OSDEP_EVENT_SIGNAL is called
 * for the same event variable, unless _OSDEP_EVENT_SIGNAL has already been
 * called with no other pending or subsequent _OSDEP_EVENT_WAIT calls.
 */
#ifndef _OSDEP_EVENT_WAIT
#define _OSDEP_EVENT_WAIT(ev)	{ while ((ev) == 0) ; }
#endif

/*
 * _OSDEP_EVENT_SIGNAL -- Signal an event
 *
 * This call causes any thread blocked in _OSDEP_EVENT_WAIT to be resumed.
 * If no threads are currently blocked when _OSDEP_EVENT_SIGNAL is called,
 * the next call to _OSDEP_EVENT_WAIT will not block.
 */
#ifndef _OSDEP_EVENT_SIGNAL
#define _OSDEP_EVENT_SIGNAL(ev)	((ev) = 1)
#endif

/*
 * Atomic Integer Macros
 */
#ifndef _OSDEP_ATOMIC_INT_T
#define _OSDEP_ATOMIC_INT_T	struct __udi_atomic_int
struct __udi_atomic_int {
	_OSDEP_SIMPLE_MUTEX_T amutex;
	int aval;
};
#endif

#ifndef _OSDEP_ATOMIC_INT_INIT
static void
_osdep_atomic_int_init(_OSDEP_ATOMIC_INT_T *atomic_intp,
		       int ival)
{
	_OSDEP_SIMPLE_MUTEX_INIT(&(atomic_intp)->amutex, "amutex");
	atomic_intp->aval = ival;
}

#define _OSDEP_ATOMIC_INT_INIT(atomic_intp, ival) \
	_osdep_atomic_int_init((atomic_intp), (ival))
#endif

#ifndef _OSDEP_ATOMIC_INT_DEINIT
#define _OSDEP_ATOMIC_INT_DEINIT(atomic_intp) \
		_OSDEP_SIMPLE_MUTEX_DEINIT(&(atomic_intp)->amutex)
#endif

#ifndef _OSDEP_ATOMIC_INT_READ
#define _OSDEP_ATOMIC_INT_READ(atomic_intp)	\
		((atomic_intp)->aval)
#endif

#ifndef _OSDEP_ATOMIC_INT_INCR
static void
_osdep_atomic_int_incr(_OSDEP_ATOMIC_INT_T *atomic_intp)
{
	_OSDEP_SIMPLE_MUTEX_LOCK(&(atomic_intp)->amutex);
	atomic_intp->aval++;
	_OSDEP_SIMPLE_MUTEX_UNLOCK(&(atomic_intp)->amutex);
}

#define _OSDEP_ATOMIC_INT_INCR(atomic_intp)	\
	_osdep_atomic_int_incr((atomic_intp))
#endif

#ifndef _OSDEP_ATOMIC_INT_DECR
static void
_osdep_atomic_int_decr(_OSDEP_ATOMIC_INT_T *atomic_intp)
{
	_OSDEP_SIMPLE_MUTEX_LOCK(&(atomic_intp)->amutex);
	(atomic_intp)->aval--;
	_OSDEP_SIMPLE_MUTEX_UNLOCK(&(atomic_intp)->amutex);

}

#define _OSDEP_ATOMIC_INT_DECR(atomic_intp)	\
	_osdep_atomic_int_decr((atomic_intp))
#endif

/*
 * OS-specific parts of various environment structures.
 */
#ifndef _OSDEP_DRIVER_T
#define _OSDEP_DRIVER_T		char
#endif
#ifndef _OSDEP_BUF_T
#define _OSDEP_BUF_T		char
#endif
#ifndef _OSDEP_DEV_NODE_T
#define _OSDEP_DEV_NODE_T	char
#endif
#ifndef _OSDEP_DMA_T
#define _OSDEP_DMA_T		char
#endif
#ifndef _OSDEP_PIO_T
#define _OSDEP_PIO_T		char
#endif
#ifndef _OSDEP_INTR_T
#define _OSDEP_INTR_T		char
#endif

/* 
 * udi_limits_t hooks.
 */
void _udi_get_limits_t(udi_limits_t *limits);

#ifndef _OSDEP_GET_LIMITS_T
#define _OSDEP_GET_LIMITS_T(l) _udi_get_limits_t(l)
#endif

/*
 * Define _OSDEP_IMMED_CALLBACK and the _OSDEP_CALLBACK_LIMIT.
 * We may eventually want to eliminate the limit, and somehow use the
 * stack size instead, to determine a maximum.
 */
#ifndef _OSDEP_IMMED_CALLBACK
#define _OSDEP_IMMED_CALLBACK	_UDI_IMMED_CALLBACK
#endif
#ifndef _OSDEP_CALLBACK_LIMIT
#define _OSDEP_CALLBACK_LIMIT	20
#endif

/*
 * An OS might force to defer a channel operation for reasons other
 * than the target being busy.  Examples might include needing to 
 * defer channel ops while servicing an interrupt.
 * This must evaluate to a C expression returning non-zero if the 
 * call is to be deferred.
 * You will almost certainly want to have the default expression 
 * below 'or'ed into your expression or else you will fail to honor
 * the (common) case of a region being busy.
 */
#ifndef _OSDEP_DEFER_MEI_CALL
#  define _OSDEP_DEFER_MEI_CALL(region, op_flag)  \
    (/* !UDI_QUEUE_EMPTY(&region->reg_queue) || */ REGION_IS_ACTIVE(region))
#endif

/*
 * Even if a channel op call is not completely deferred, you may
 * want to delay execution to a different context, such as a lower
 * interrupt priority level.  These hooks allows an OS to schedule
 * the region execution at a later time, but in a way that's "faster"
 * than _OSDEP_REGION_SCHEDULE.  Both of these are called with the
 * region lock held.  _OSDEP_FASTQ_SCHED is called with the channel
 * op already queued on the region queue.
 */
#ifndef _OSDEP_FASTQ_MEI_CALL
#  define _OSDEP_FASTQ_MEI_CALL(rgn, op_flag)   FALSE
#endif

#ifndef _OSDEP_FASTQ_SCHED
#  define _OSDEP_FASTQ_SCHED(rgn)
#endif


/*
 * _OSDEP_REGION_SCHEDULE is called by udi_mei_call once a mei call 
 * has been deferred onto a region.   This allows the OS to decide 
 * how to schedule the region in case it wants multiple queues.
 * This allows you to remember which regions may need to have their
 * reg_sched_link queues drained.
 * 
 * Typical (though unambitious) usage might approximate:
 *   lock(my_reqion_q_lock)
 *   UDI_ENQUEUE_TAIL(&my_region_q_head, &region->reg_sched_link)
 *   unlock(my_reqion_q_lock)
 *
 * Later, once you wanted to actually execute the target regions,
 * you could do something like:
 *
 *        _OSDEP_MUTEX_LOCK(&my_region_q_lock);
 *        while (!UDI_QUEUE_EMPTY(&my_region_q_head)) {
 *               elem = UDI_DEQUEUE_HEAD(&my_region_q_head);
 *                _OSDEP_MUTEX_UNLOCK(&my_region_q_lock);
 *                rgn = UDI_BASE_STRUCT(elem, _udi_region_t, reg_sched_link);
 *                UDI_RUN_REGION_QUEUE(rgn);
 *        }
 *        _OSDEP_MUTEX_UNLOCK(&my_region_q_lock);
 */
#ifndef _OSDEP_REGION_SCHEDULE
#  define _OSDEP_REGION_SCHEDULE(region)
#endif

/*
 * _OSDEP_CONT_RUN_THIS_REGION(_udi_region_t *rgn)
 * As long as  this expression evaluates 'true', any additional work
 * sitting on the region queue will be executed before the region is left.
 * An OS can provide scheduling flexibility by providing this.
 * 
 * Our default definition will not allow "barnacles" to attach to an
 * an executing interrupt region and be executed at its (presumably 
 * favored) priority.
 * 
 */

#ifndef _OSDEP_CONT_RUN_THIS_REGION
#  define _OSDEP_CONT_RUN_THIS_REGION(rgn) \
   (!(rgn->reg_is_interrupt))
#endif

/*
 *  _OSDEP_MASK_INTR -- mask interrupts from a device as necessary
 *  for interrupt regions.
 * 
 *  Calling convention: TBD.
 */
#ifndef _OSDEP_MASK_INTR
# define _OSDEP_MASK_INTR()
#endif

/*
 *  _OSDEP_UNMASK_INTR -- unmask interrupts from a device.
 * 
 *  Calling convention: TBD.
 */
#ifndef _OSDEP_UNMASK_INTR
# define _OSDEP_UNMASK_INTR()
#endif


/*
 * Memory allocation definitions.
 */
#ifndef _OSDEP_MEM_ALLOC
#define _OSDEP_MEM_ALLOC(size, flags, sleep_ok) \
		((flags) & UDI_MEM_NOZERO ? \
			malloc((size)) : memset(malloc((size)), 0, size))
#endif
#ifndef _OSDEP_MEM_FREE
#define _OSDEP_MEM_FREE(mem) \
		free((mem))
#endif

/*
 * _OSDEP_NEW_ALLOC -- Indicate that there is new alloc daemon work to do.
 */
#ifndef _OSDEP_NEW_ALLOC
#define _OSDEP_NEW_ALLOC()	\
		{ while (_udi_alloc_daemon_work()) ; }
#endif

/* 
 * Initialize the device instance structure.
 */
#ifndef _OSDEP_DEV_NODE_INIT
#  define _OSDEP_DEV_NODE_INIT(x, node)
#endif

/* 
 * Destsroy the device instance structure.
 */
#ifndef _OSDEP_DEV_NODE_DEINIT
#  define _OSDEP_DEV_NODE_DEINIT(node)
#endif

/*
 * PIO services.
 */
#ifndef _OSDEP_PIO_DELAY
#  define _OSDEP_PIO_DELAY(microsec)
#endif

#ifndef _OSDEP_RELEASE_PIO_MAPPING
#  define _OSDEP_RELEASE_PIO_MAPPING(pio_handle)
#endif

#ifndef _OSDEP_PIO_MAP
#  define _OSDEP_PIO_MAP(_pio_handle)
#endif

#ifndef _OSDEP_PIO_UNMAP
#  define _OSDEP_PIO_UNMAP(_pio_handle)
#endif

#ifndef _OSDEP_PIO_BARRIER
#  define _OSDEP_PIO_BARRIER(_pio_handle, operand)
#endif

#ifndef _OSDEP_PIO_SYNC
#  define _OSDEP_PIO_SYNC(_pio_handle, tran_size_idx, operand)
#endif

#ifndef _OSDEP_PIO_SYNC_OUT
#  define _OSDEP_PIO_SYNC_OUT _OSDEP_PIO_SYNC
#endif

/*
 * PIO data types.
 * These are the longest data types that the PIO computational engine
 * (not the reader/writer back-end) will attempt to handle internally
 * for things like addition without falling into the 'looping' code.
 * If you have an efficient 'long long', you might like to use it here.
 * Since that isn't specified in ISO C, we default to 'long', whatever
 * size that may be.
 */
#ifndef _OSDEP_PIO_LONGEST_STYPE
#  define _OSDEP_PIO_LONGEST_STYPE signed long
#endif

#ifndef _OSDEP_PIO_LONGEST_UTYPE
#  define _OSDEP_PIO_LONGEST_UTYPE unsigned long
#endif

#ifndef _OSDEP_PRINTF_LONG_LONG_SPEC
#  define _OSDEP_PRINTF_LONG_LONG_SPEC "%016llx"
#endif

/*
 * Timer services from the host OS.
 */
#ifndef USEC_PER_SEC
#  define USEC_PER_SEC (1000 * 1000)
#endif

#ifndef NSEC_PER_SEC
#  define NSEC_PER_SEC (1000 * USEC_PER_SEC)
#endif

#ifndef _OSDEP_TIMER_T
#define _OSDEP_TIMER_T int
#endif

#ifndef _OSDEP_TIMER_START
#define _OSDEP_TIMER_START(callback_fn, arg, interval, flags) \
		((*(callback_fn)) (arg), 0)
#endif

#ifndef _OSDEP_TIMER_CANCEL
#define _OSDEP_TIMER_CANCEL(callback_fn, arg, timerID)
#endif

/*
 *  By default, walk the short_q every 2 ticks.
 */
#ifndef _OSDEP_SHORT_TIMER_THRESHOLD
#define _OSDEP_SHORT_TIMER_THRESHOLD	2
#endif

/*
 *  By default, walk the long_q every 100 ticks.  (Once a second for 
 *  100HZ tick.)
 */
#ifndef _OSDEP_LONG_TIMER_THRESHOLD
#define _OSDEP_LONG_TIMER_THRESHOLD 100
#endif

/*
 * Timestamps
 */
#ifndef _OSDEP_GET_CURRENT_TIME
#define _OSDEP_GET_CURRENT_TIME(t) 0
#endif
#ifndef _OSDEP_SUBTRACT_TIME
# ifndef _OSDEP_CLK_TCK
#  error  Need to define _OSDEP_CLK_TCK (number of ticks per second)
# endif
#define NANOSECS_PER_TICK	(NSEC_PER_SEC / (_OSDEP_CLK_TCK))

/*
 * Subtract end from start (both udi_timestamp_t) and put the result
 * into the (udi_time_t) result. The Spec is written so that start is
 * constrained to be <= end (wrt udi_time_since & udi_time_between).
 *
 * In our reference implementation, a udi_timestamp_t is a 'unsigned 
 * long' which is at least 32 bits on the current systems.  Expression 
 * overflow is not a problem because the (_ticks % (_OSDEP_CLK_TCK)) 
 * portion will return something in the range 0..99 (for HZ=100), and 
 * the NANOSECONDS_PER_TICK portion is a value 1Billion/HZ, so the 
 * multiplication should produce a value which is < 1Billion, which 
 * fits into an unsigned  32bit value just fine.
 */
#define _OSDEP_SUBTRACT_TIME(end, start, result) \
        { \
            udi_timestamp_t _ticks = (end) - (start); \
            (result).seconds = _ticks / (_OSDEP_CLK_TCK); \
            (result).nanoseconds = (_ticks % (_OSDEP_CLK_TCK)) * \
					NANOSECS_PER_TICK; \
        }

#endif /* _OSDEP_SUBTRACT_TIME */

/*
 * DMA hooks.
 */
#ifndef _OSDEP_DMA_PREPARE
#define _OSDEP_DMA_PREPARE(dmah, gcb)	((udi_status_t)UDI_OK)
#endif
#ifndef _OSDEP_DMA_RELEASE
#define _OSDEP_DMA_RELEASE(dmah) /**/
#endif
#ifndef _OSDEP_DMA_BUF_MAP
#define _OSDEP_DMA_BUF_MAP(dmah, allocm, waitflag)	\
		(((allocm)->_u.dma_buf_map_result.status = \
			UDI_STAT_NOT_SUPPORTED), \
		 FALSE)
#endif
#ifndef _OSDEP_DMA_BUF_MAP_DISCARD
#define _OSDEP_DMA_BUF_MAP_DISCARD(allocm) /**/
#endif
#ifndef _OSDEP_DMA_BUF_UNMAP
#define _OSDEP_DMA_BUF_UNMAP(dmah) /**/
#endif
#ifndef _OSDEP_DMA_SCGTH_FREE
#define _OSDEP_DMA_SCGTH_FREE(dmah) /**/
#endif
#ifndef _OSDEP_DMA_SYNC
#define _OSDEP_DMA_SYNC(dmah, offset, len, flags) /**/
#endif
#ifndef _OSDEP_DMA_SCGTH_SYNC
#define _OSDEP_DMA_SCGTH_SYNC(dmah) /**/
#endif
#ifndef _OSDEP_DMA_MEM_BARRIER
#define _OSDEP_DMA_MEM_BARRIER(dmah) /**/
#endif
#ifndef _OSDEP_DMA_MEM_ALLOC
#define _OSDEP_DMA_MEM_ALLOC(dmah, size, flags) \
		_OSDEP_MEM_ALLOC(size, flags, UDI_WAITOK)
#endif
#ifndef _OSDEP_DMA_MEM_FREE
#define _OSDEP_DMA_MEM_FREE(dmah) /**/
#endif
#ifndef _OSDEP_DMA_MEM_TO_BUF
#define _OSDEP_DMA_MEM_TO_BUF(dmah, buf, offset, size) buf
#endif

/*
 * Bridge mapper hooks
 */
#ifndef _OSDEP_BRIDGE_RDATA_T
#define _OSDEP_BRIDGE_RDATA_T		char
#endif
#ifndef _OSDEP_BRIDGE_CHILD_DATA_T
#define _OSDEP_BRIDGE_CHILD_DATA_T	char
#endif
#ifndef _OSDEP_ENUMERATE_PCI_DEVICE
#define _OSDEP_ENUMERATE_PCI_DEVICE(rdata, cb, enum_level)	FALSE
#endif
#ifndef _OSDEP_ENDIANNESS_QUERY
#define _OSDEP_ENDIANNESS_QUERY(bridge_osinfo)	UDI_DMA_LITTLE_ENDIAN
#endif
#ifndef _OSDEP_ISR_RETURN_T
#define _OSDEP_ISR_RETURN_T	int
#endif
#ifndef _OSDEP_REGISTER_ISR
#define _OSDEP_REGISTER_ISR(isr, gcb, context, enum_osinfo, attach_osinfo) UDI_OK
#endif
#ifndef _OSDEP_UNREGISTER_ISR
#define _OSDEP_UNREGISTER_ISR(isr, gcb, context, enum_osinfo, attach_osinfo) UDI_OK
#endif
#ifndef _OSDEP_GET_ISR_CONTEXT
#define _OSDEP_GET_ISR_CONTEXT(_context) \
		((intr_attach_context_t *)(_context))
#endif
#ifndef _OSDEP_GET_EVENT_INFO
#define _OSDEP_GET_EVENT_INFO(_context) 	NULL
#endif
#ifndef _OSDEP_ISR_RETURN_VAL
#define _OSDEP_ISR_RETURN_VAL(intr_status)	0
#endif
#ifndef _OSDEP_ISR_ACKNOWLEDGE
#define _OSDEP_ISR_ACKNOWLEDGE(intr_status)
#endif
#ifndef _OSDEP_BRIDGE_MAPPER_ASSIGN_CHILD_ID
#define _OSDEP_BRIDGE_MAPPER_ASSIGN_CHILD_ID FALSE
#endif


/* 
 * _OSDEP_INSTANCE_ATTR_SET is used as a backend function to handle any
 * attributes that can't be handled simply in the frontend.
 */
#ifndef _OSDEP_INSTANCE_ATTR_SET
#  define _OSDEP_INSTANCE_ATTR_SET(node, name, value, length, attr_type) UDI_OK
#endif
#ifndef _OSDEP_INSTANCE_ATTR_GET
#  define _OSDEP_INSTANCE_ATTR_GET(node, name, value, length, attr_type) UDI_OK
#endif

/*
 * osdep function to verify that the value is supported for the given 
 * constraint
 */
#ifndef _OSDEP_CONSTRAINTS_ATTR_CHECK
#  define _OSDEP_CONSTRAINTS_ATTR_CHECK(attr, value) TRUE
#endif

/*
 * osdep function to signal the enumeration done
 */
#ifndef _OSDEP_ENUMERATE_DONE
#define _OSDEP_ENUMERATE_DONE(node)
#endif
#endif /* _UDI_OSDEP_DEFAULTS_H */
