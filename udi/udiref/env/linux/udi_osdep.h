/*
 * File: env/linux/udi_osdep.h
 *
 * This file contains macros and types that pertain specifically to
 * the Linux OS. Macros listed in this file override those in the
 * env/common/udi_osdep_defaults.h
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


#ifndef _UDI_OSDEP_H
#define _UDI_OSDEP_H

/* 
 *  Tell UDI environment that we can support physical I/O.
 */
#define _UDI_PHYSIO_SUPPORTED 1

#define _OSDEP_PRINTF_LONG_LONG_SPEC "%016llx"

/*
 * Define STATIC here to the "static" keyword so that functions declared as
 * such actually are static functions.
 */
#ifndef DEBUG
#  ifndef STATIC
#    define STATIC static
#  endif  /* STATIC */
#else
#  ifndef STATIC
#    define STATIC
#  endif
#endif	/* DEBUG */

/* misc prototype */
extern void _udi_get_limits_t(udi_limits_t * limits);
extern void osdep_timer_init(void);
extern void osdep_timer_deinit(void);


/*
 * Redefine printf just in case someone
 * tries to do one from the kernel (very bad)
 */
extern void _udi_osdep_printf(const char *format, ...)
        __attribute__ ((format (printf, 1, 2)));

/*
 * _OSDEP_PRINTF -- Debugging printf
 *
 * Use the kernel "printk()" for debug output
 */

#define _OSDEP_PRINTF _udi_osdep_printf
#define _OSDEP_WRITE_DEBUG(s,x) _udi_osdep_printf("%s", s)

/*
 * _OSDEP_DEBUG_BREAK -- Enter debugger
 *
 * If KDB is installed, stop in debugger, else ASSERT(0).
 */

extern void _udi_osdep_debug_break(void);
#define _OSDEP_DEBUG_BREAK _udi_osdep_debug_break()


/* Map common functions to udi functions. */
#ifndef strcpy
#define strcpy udi_strcpy
#endif
#ifndef strcmp
#define strcmp udi_strcmp
#endif
#ifndef strcat
#define strcat udi_strcat
#endif
#ifndef strncpy
#define strncpy udi_strncpy
#endif
#ifndef strncmp
#define strncmp udi_strncmp
#endif
#ifndef strncat
#define strncat udi_strncat
#endif
#ifndef memcpy
#define memcpy udi_memcpy
#endif
#ifndef memcmp
#define memcmp udi_memcmp
#endif
#ifndef strlen
#define strlen udi_strlen
#endif

/*
 * _OSDEP_ASSERT -- Assertion check
 *
 */

extern void _udi_osdep_assert(const char *file, int line, const char *expr_text)
      __attribute__ ((noreturn));

#define _ASSERT    _OSDEP_ASSERT
#define ASSERT     _OSDEP_ASSERT
#define _OSDEP_ASSERT(expr) \
	do {if(!(expr)) _udi_osdep_assert(__FILE__, __LINE__, #expr);} while(0)

#ifndef __UDI_OSDEP_OPAQUEDEFS_H__
#include "udi_osdep_opaquetypes.h"
#endif
/*
 * _OSDEP_ATOMIC_INT_T -- atomic int
 *
 * This implementation is faster than the default one
 */
typedef struct _udi_osdep_atomic_int_opaque udi_atomic_int_t;
#define _OSDEP_ATOMIC_INT_T udi_atomic_int_t

extern void _udi_osdep_atomic_int_init(_OSDEP_ATOMIC_INT_T*, int);
#define _OSDEP_ATOMIC_INT_INIT _udi_osdep_atomic_int_init

extern void _udi_osdep_atomic_int_deinit(_OSDEP_ATOMIC_INT_T*);
#define _OSDEP_ATOMIC_INT_DEINIT _udi_osdep_atomic_int_deinit

extern int _udi_osdep_atomic_int_read(_OSDEP_ATOMIC_INT_T*);
#define _OSDEP_ATOMIC_INT_READ _udi_osdep_atomic_int_read

extern void _udi_osdep_atomic_int_incr(_OSDEP_ATOMIC_INT_T*);
#define _OSDEP_ATOMIC_INT_INCR _udi_osdep_atomic_int_incr

extern void _udi_osdep_atomic_int_decr(_OSDEP_ATOMIC_INT_T*);
#define _OSDEP_ATOMIC_INT_DECR _udi_osdep_atomic_int_decr

/*
 * _OSDEP_EVENT_T -- Event synchronization variable
 *
 * Events can: be reused; only get one wait/signal pair per use;
 * and the wait/signal pair can come in the wrong order.
 *
 * The old implementation used spinlocks around a wait queue and
 * signalled flag.  A race condition could exist, so we must use
 * an additional flag.  The additional flag is set when someone
 * goes to sleep and then possibly waits around in a bottom half
 * handler for that task to get itself into the wait queue.
 *
 *         -- Sid Cammeresi
 */
typedef struct _udi_osdep_event_opaque udi_event_t;
#define _OSDEP_EVENT_T udi_event_t

inline void _udi_osdep_event_init(_OSDEP_EVENT_T *ev);
#define _OSDEP_EVENT_INIT _udi_osdep_event_init

inline void _udi_osdep_event_deinit(_OSDEP_EVENT_T *ev);
#define _OSDEP_EVENT_DEINIT _udi_osdep_event_deinit

inline void _udi_osdep_event_wait(_OSDEP_EVENT_T *ev);
#define _OSDEP_EVENT_WAIT _udi_osdep_event_wait

inline void _udi_osdep_event_signal(_OSDEP_EVENT_T *ev);
#define _OSDEP_EVENT_SIGNAL _udi_osdep_event_signal
    
inline void _udi_osdep_event_clear(_OSDEP_EVENT_T *ev);
#define _OSDEP_EVENT_CLEAR _udi_osdep_event_clear

/* Semaphores. Net Mapper currently uses these. */
typedef struct _udi_osdep_semaphore_opaque _udi_osdep_semaphore_t;

#define _OSDEP_SEM_T _udi_osdep_semaphore_t

inline void _udi_osdep_sem_init(_OSDEP_SEM_T *sem, long initial);
#define _OSDEP_SEM_INIT _udi_osdep_sem_init

inline void _udi_osdep_sem_p(_OSDEP_SEM_T *sem);
#define _OSDEP_SEM_P _udi_osdep_sem_p

inline void _udi_osdep_sem_v(_OSDEP_SEM_T *sem);
#define _OSDEP_SEM_V _udi_osdep_sem_v

inline void _udi_osdep_sem_deinit(_OSDEP_SEM_T *sem);
#define _OSDEP_SEM_DEINIT _udi_osdep_sem_deinit

 
/*
 * Memory allocation definitions
 *
 * Use the kernel kmalloc() to allocate the memory.
 * In an attempt to optimize memory allocation and provide
 * ok memory debugging, you can use two defines to control
 * the level of memory debugging you want.
 * LOW_OVERHEAD_MALLOC: inline kmalloc/kfree. (No debugging.)
 * No defines: (Default) track memory allocations by pre-pending
 *             a header.
 * INSTRUMENT_MALLOC: tack on a header and a footer. Keep a
 *             queue of all memory allocations so we can walk
 *             the pool of UDI allocated memory.
 */
#ifdef LOW_OVERHEAD_MALLOC
#ifdef INSTRUMENT_MALLOC
#error "INSTRUMENT_MALLOC not available with LOW_OVERHEAD_MALLOC"
#endif
inline void *_udi_mem_alloc(udi_size_t size, int flags, int sleep_ok)
{
	void *mem;
	mem = kmalloc(size, (sleep_ok?GFP_KERNEL:GFP_ATOMIC));
	if (mem && !(flags & UDI_MEM_NOZERO))
		udi_memset(mem, 0, size);
	return mem;
}
#define _OSDEP_MEM_ALLOC(size, flags, sleep_ok) \
	_udi_mem_alloc(size, flags, sleep_ok)
#define _OSDEP_MEM_FREE(mem) \
	kfree(mem);

#else

#ifdef INSTRUMENT_MALLOC
  extern void * _udi_wrapped_alloc_inst(udi_size_t size, int flags, int sleep, int line, char *f);
  #define _OSDEP_MEM_ALLOC(size, flags, sleep_ok) \
                _udi_wrapped_alloc_inst(size, flags, sleep_ok, __LINE__, __FILE__)
  void _udi_wrapped_free_inst(void *, int line, char *f);
  #define _OSDEP_MEM_FREE(mem) \
                _udi_wrapped_free_inst(mem, __LINE__, __FILE__)
#else
  extern void * _udi_wrapped_alloc(udi_size_t size, int flags, int sleep);
  #define _OSDEP_MEM_ALLOC(size, flags, sleep_ok) \
                _udi_wrapped_alloc(size, flags, sleep_ok)
  void _udi_wrapped_free(void *);
  #define _OSDEP_MEM_FREE(mem) \
                _udi_wrapped_free(mem)
#endif /*INSTRUMENT_MALLOC*/

#endif /*LOW_OVERHEAD_MALLOC*/
/*
 * _OSDEP_NEW_ALLOC
 *
 * wake up alloc daemon
 */
 
extern _OSDEP_EVENT_T _udi_alloc_event; 
#define _OSDEP_NEW_ALLOC() _OSDEP_EVENT_SIGNAL(&_udi_alloc_event)

/*
 * Device Node Macros
 */
typedef struct _udi_osdep_dev_node_opaque _udi_osdep_dev_node;
inline void _udi_dev_node_init(_udi_driver_t *driver, _udi_dev_node_t *node);
inline void _udi_dev_node_deinit(_udi_dev_node_t *node);

#define _OSDEP_DEV_NODE_T       _udi_osdep_dev_node
#define _OSDEP_DEV_NODE_INIT    _udi_dev_node_init
#define _OSDEP_DEV_NODE_DEINIT  _udi_dev_node_deinit


#define _OSDEP_MATCH_DRIVER _udi_MA_match_children
struct _udi_dev_node;
struct _udi_driver *_udi_MA_match_children(struct _udi_dev_node *child_node);

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

#define _OSDEP_INSTANCE_ATTR_SET __osdep_instance_attr_set
#define _OSDEP_INSTANCE_ATTR_GET __osdep_instance_attr_get

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
typedef struct _udi_osdep_spinlock_opaque udi_osdep_spinlock_t;

#define _OSDEP_MUTEX_T	udi_osdep_spinlock_t

extern void _udi_osdep_mutex_init(_OSDEP_MUTEX_T *mutexp, char *lockname,
		char *filename, unsigned int lineno);
#define _OSDEP_MUTEX_INIT(mutexp, lockname) \
	_udi_osdep_mutex_init(mutexp, lockname, __FILE__, __LINE__)

extern void _udi_osdep_mutex_deinit(_OSDEP_MUTEX_T *mutexp);
#define _OSDEP_MUTEX_DEINIT _udi_osdep_mutex_deinit

extern void _udi_osdep_mutex_lock(_OSDEP_MUTEX_T *mutexp,
		char *filename, unsigned int lineno);
#define _OSDEP_MUTEX_LOCK(mutexp) \
	_udi_osdep_mutex_lock(mutexp, __FILE__, __LINE__)

extern void _udi_osdep_mutex_unlock(_OSDEP_MUTEX_T *mutexp);
#define _OSDEP_MUTEX_UNLOCK _udi_osdep_mutex_unlock

/*******************/
/** LATER - MAYBE **/
/*******************/

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
 * Timer services
 */

/*
 * _OSDEP_TIMER_T -- the type linux references timers by
 * This is an index into our table.
 * _OSDEP_TICKS_T -- this should match the type of jiffies.
 */
typedef int _osdep_timer_t;
typedef unsigned long _osdep_ticks_t;
#define _OSDEP_TIMER_T _osdep_timer_t
#define _OSDEP_TICKS_T _osdep_ticks_t
 
extern _osdep_ticks_t osdep_get_jiffies(void);
#define TICKS() osdep_get_jiffies()

#define _OSDEP_CURR_TIME_IN_TICKS TICKS()
#define _OSDEP_GET_CURRENT_TIME(t) (t) = TICKS()

extern _OSDEP_TICKS_T _osdep_time_t_to_ticks(udi_time_t interval);
#define _OSDEP_TIME_T_TO_TICKS _osdep_time_t_to_ticks

/*
 * _OSDEP_SUBTRACT_TIME -- Calculate the difference between two times
 */
extern void osdep_subtract_time(_OSDEP_TIMER_T op1, _OSDEP_TIMER_T op2, udi_time_t *result);
#define _OSDEP_SUBTRACT_TIME(op1, op2, result) \
    osdep_subtract_time(op1, op2, &result)

/*
 * _OSDEP_TIMER_START -- start a system timer
 */
typedef void (*osdep_timer_callback_t)(void *);
_OSDEP_TIMER_T osdep_timer_start(osdep_timer_callback_t timer_done, void *arg,
                                 _OSDEP_TICKS_T interval, int repeat);
#define _OSDEP_TIMER_START(callback_fn, arg, interval, flags) \
    osdep_timer_start((osdep_timer_callback_t)callback_fn, arg, interval, flags)

/*
 * _OSDEP_TIMER_CANCEL -- cancel a pending system timer
 */
extern void osdep_timer_cancel(_OSDEP_TIMER_T the_timer);
#define _OSDEP_TIMER_CANCEL(func, cb, tid) osdep_timer_cancel(tid) 


/*
 * Driver Management
 */

typedef struct _udi_sprops_idx {
    int num;
    void *off;
} _udi_sprops_idx_t;

typedef struct _udi_osdep_driver_opaque _udi_osdep_driver_t;
#define _OSDEP_DRIVER_T _udi_osdep_driver_t

/************************************************************************
 * End of portability macros.
 ************************************************************************/
 
/*
 * DMA definitions.
 */

#define CONTIGUOUS 1

typedef struct {
	void *dma_mem;
	udi_size_t ofs, size;
	udi_queue_t link;
} _osdep_dma_alloc_t;

typedef struct {
        udi_queue_t link;
        struct _udi_buf_memblk* memblk;
	udi_ubit8_t* mem_space;
        _osdep_dma_alloc_t* dma_alloc;
	void *dma_mem;
	udi_size_t offset, size;
} __osdep_dma_remap_t;


typedef struct {
	unsigned long phys_align, scgth_phys_align, phys_boundary;
	unsigned char dmasize, scgth_dmasize, max_scgth, scgth_max_scgth, scgth_max_el;
	udi_ubit32_t scgth_prefix_bytes;
	int scgth_phys, data_phys;
	udi_queue_t remap_queue;
	udi_size_t els_in_seg, scgth_num_segments, dma_size, dma_len, dma_gran;
	void* copy_mem;
	_osdep_dma_alloc_t* scgth_alloc, *dma_alloc;
	int waitflag;
} __osdep_dma_t;

#define _OSDEP_DMA_T		__osdep_dma_t

struct _udi_dma_handle;
struct _udi_alloc_marshal;

#define _OSDEP_DMA_PREPARE(dmah,gcb)	\
		_udi_dma_prepare(dmah)
extern udi_status_t _udi_dma_prepare(struct _udi_dma_handle *dmah);

#define _OSDEP_DMA_PREPARE_MIGHT_BLOCK	1

#define _OSDEP_DMA_RELEASE(dmah) 	\
		_udi_dma_release(dmah)
extern void _udi_dma_release(struct _udi_dma_handle *dmah);

#define _OSDEP_DMA_BUF_MAP(dmah, allocm, waitflag)	\
		_udi_dma_buf_map(dmah, allocm, waitflag)
extern udi_boolean_t _udi_dma_buf_map(struct _udi_dma_handle *dmah,
			       struct _udi_alloc_marshal *allocm,
			       int waitflag);

#define _OSDEP_DMA_BUF_MAP_DISCARD(allocm)	\
		_udi_dma_buf_map_discard(allocm)
extern void _udi_dma_buf_map_discard(struct _udi_alloc_marshal *allocm);

#define _OSDEP_DMA_BUF_UNMAP(dmah)	\
		_udi_dma_buf_unmap(dmah)
extern void _udi_dma_buf_unmap(struct _udi_dma_handle *dmah);

#define _OSDEP_DMA_SCGTH_FREE(dmah)	\
		_udi_dma_scgth_free(dmah)
extern void _udi_dma_scgth_free(struct _udi_dma_handle *dmah);

#define _OSDEP_DMA_MEM_ALLOC(dmah, size, flags) \
		_udi_dma_mem_alloc(dmah, size, flags)
extern void *_udi_dma_mem_alloc(struct _udi_dma_handle *dmah,
			 udi_size_t size,
			 int flags);

#define _OSDEP_DMA_MEM_FREE(dmah) \
		_udi_dma_mem_free(dmah)
extern void _udi_dma_mem_free(struct _udi_dma_handle* dmah);

#define _OSDEP_RELEASE_PIO_MAPPING _udi_release_pio_mapping

/*
 * region-specific defines and declarations
 */

extern udi_queue_t _udi_region_q_head;
extern _OSDEP_MUTEX_T _udi_region_q_lock;
extern _OSDEP_EVENT_T _udi_region_q_event;

#define _OSDEP_REGION_SCHEDULE(region) \
	region->in_sched_q = TRUE; \
	REGION_UNLOCK(region); \
	_OSDEP_MUTEX_LOCK(&_udi_region_q_lock); \
	UDI_ENQUEUE_TAIL(&_udi_region_q_head, &region->reg_sched_link);\
	_OSDEP_MUTEX_UNLOCK(&_udi_region_q_lock); \
	_OSDEP_EVENT_SIGNAL(&_udi_region_q_event); 

inline udi_boolean_t _osdep_in_intr();
inline void _osdep_mask_intr();
inline void _osdep_unmask_intr();

#define MIN_STACK_SIZE 		  256		/* 
						 * leave at least xx bytes in
						 * stack when nesting calls
						 */
						   
inline udi_boolean_t _osdep_low_on_stack();

#define _OSDEP_CONT_RUN_THIS_REGION(rgn) \
	(/*!_osdep_in_intr() && */!((rgn)->reg_is_interrupt))

#define _OSDEP_MASK_INTR _osdep_mask_intr
#define _OSDEP_UNMASK_INTR _osdep_unmask_intr

#define _OSDEP_IMMED_CALLBACK(cb, call_type, ops, callback, argc, callback_args) {  \
        _udi_alloc_marshal_t *allocm = &(cb)->cb_allocm;                     \
        _OSDEP_ASSERT(allocm->alloc_state == _UDI_ALLOC_IDLE);               \
        if (_osdep_low_on_stack()) {               \
            _UDI_DO_CALLBACK(callback, cb,				     \
			_UDI_CALLBACK_ARGLIST_##argc callback_args );	     \
        } else {                                                             \
	    (cb)->cb_flags |= _UDI_CB_CALLBACK;				\
            (cb)->op_idx = call_type;					\
            (cb)->cb_allocm.alloc_ops = (ops);				\
	    _UDI_SET_RESULT(SET_RESULT##call_type ,allocm, 		\
                        _UDI_CALLBACK_ARGS_##argc callback_args);	\
            (cb)->cb_func = (_udi_callback_func_t *)callback;		\
            allocm->alloc_state = _UDI_ALLOC_IMMED_DONE;		\
            _udi_queue_callback(cb);					\
        }                                                               \
}

#ifndef __powerpc__
#define _OSDEP_DEFER_MEI_CALL(region, op_flag) \
    (/* !UDI_QUEUE_EMPTY(&region->reg_queue) || */ \
     (op_flag & UDI_OP_LONG_EXEC) || REGION_IS_ACTIVE(region) || \
     _osdep_low_on_stack())
#else
#define _OSDEP_DEFER_MEI_CALL(region, op_flag)  (1)
#endif
    
/*
 * Define host endianness.
 */
#include <endian.h>
#define _OSDEP_HOST_IS_BIG_ENDIAN (__BIG_ENDIAN == __BYTE_ORDER)
#if _OSDEP_HOST_IS_BIG_ENDIAN
#define _OSDEP_BIG_ENDIAN 1
/*#define _OSDEP_ENDIANNESS_QUERY(bridgeinfo) UDI_DMA_BIG_ENDIAN*/
#endif

extern udi_boolean_t 
_osdep_constraints_attr_check(udi_dma_constraints_attr_t type, 
			      udi_ubit32_t value);
#define _OSDEP_CONSTRAINTS_ATTR_CHECK _osdep_constraints_attr_check

extern void
_udi_osdep_enumerate_done(_udi_dev_node_t *udi_node);
#define _OSDEP_ENUMERATE_DONE _udi_osdep_enumerate_done


/* 
 * PIO definitions.
 */
#if _OSDEP_HOST_IS_BIG_ENDIAN
typedef long _OSDEP_LONGEST_PIO_TYPE;
typedef unsigned long _OSDEP_LONGEST_PIO_UTYPE;
#define _OSDEP_UDI_UBIT64_T unsigned long
#define _OSDEP_UDI_SBIT64_T signed long
#else
typedef long long _OSDEP_LONGEST_PIO_TYPE;
typedef unsigned long long _OSDEP_LONGEST_PIO_UTYPE;
#define _OSDEP_UDI_UBIT64_T unsigned long long
#define _OSDEP_UDI_SBIT64_T signed long long
#endif

typedef struct _udi_osdep_pio_opaque _udi_osdep_pio_t;

#define _OSDEP_PIO_T _udi_osdep_pio_t
#define _OSDEP_GET_PIO_MAPPING	_udi_get_pio_mapping
extern void osdep_pio_delay(long);
#define _OSDEP_PIO_DELAY	osdep_pio_delay
#define _OSDEP_RELEASE_PIO_MAPPING _udi_release_pio_mapping

/* Memory coherency */
#if 0
/*
 * We can't prototype these functions here due to intricate
 * header file dependencies.
 */
extern void _udi_osdep_pio_barrier(_udi_pio_handle_t *_pio_handle,
		_OSDEP_PIO_LONGEST_STYPE operand);
extern void _udi_osdep_pio_sync(_udi_pio_handle_t *_pio_handle,
		udi_index_t tran_size_idx, _OSDEP_PIO_LONGEST_STYPE operand);
extern void _udi_osdep_pio_sync_out(_udi_pio_handle_t *_pio_handle,
		udi_index_t tran_size_idx, _OSDEP_PIO_LONGEST_STYPE operand);
#endif

#define _OSDEP_PIO_BARRIER(_pio_handle, operand) \
		_udi_osdep_pio_barrier(_pio_handle, operand)
#define _OSDEP_PIO_SYNC(_pio_handle, tran_size_idx, operand) \
		_udi_osdep_pio_sync(_pio_handle, tran_size_idx, operand)
#define _OSDEP_PIO_SYNC_OUT(_pio_handle, tran_size_idx, operand) \
		_udi_osdep_pio_sync_out(_pio_handle, tran_size_idx, operand)

#endif /* _UDI_OSDEP_H */

