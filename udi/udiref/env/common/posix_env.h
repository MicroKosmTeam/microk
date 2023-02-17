
/*
 * File: env/common/posix_env.h
 *
 * UDI Common POSIX Environment definitions for user-space environments.
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

#ifndef _POSIX_ENV_H
#define _POSIX_ENV_H

#ifndef STATIC
#  define STATIC static
#endif

#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <stdio.h>
#undef offsetof				/* Defined in udi_types.h for drivers */
#include <stddef.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/times.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#ifndef _POSIX_OSDEP_MBLK_DATA
#  define  _POSIX_OSDEP_MBLK_DATA
#endif

#ifndef _POSIX_OSDEP_INIT_MBLK
#  define _POSIX_OSDEP_INIT_MBLK(a)
#endif

#ifndef _POSIX_OSDEP_DEINIT_MBLK
#  define _POSIX_OSDEP_DEINIT_MBLK(a)
#endif

#ifndef _POSIX_OSDEP_DUMP_MBLK
#  define _POSIX_OSDEP_DUMP_MBLK(a)
#endif


/*
 * If you want to override _OSDEP_PRINTF, override
 * printf itself.
 */
#define _OSDEP_PRINTF printf
#define _OSDEP_WRITE_DEBUG(str, len) _OSDEP_PRINTF(str)


/*
 * The structure we use to track our allocations.
 */
typedef struct {
	udi_queue_t mblk_qlink;
	size_t size;
	unsigned int allocator_linenum;
	unsigned int signature;
	char *allocator_fname;
	  _POSIX_OSDEP_MBLK_DATA
		/*
		 * Allocated data immediately follows 
		 */
} _mblk_header_t;
void posix_add_to_alloc_log(_mblk_header_t *mem);
void posix_delete_from_alloc_log(_mblk_header_t *mem);

#ifdef _OSDEP_TIMESTAMP_STR
#undef _OSDEP_TIMESTAMP_STR
#endif
#define _OSDEP_TIMESTAMP_STR(B,L,T) udi_snprintf((B),(L),"tick%u",(T));


/*
 *  Timestamp and Timer services.
 */
#define POSIX_TIMER_HZ	100
udi_ubit32_t _udi_get_cur_time_in_ticks(void);

#ifndef _OSDEP_CLK_TCK
#  define _OSDEP_CLK_TCK 40000	/* If the OS supports it this high... */
#endif
#define _OSDEP_GET_CURRENT_TIME(res) \
	{ \
		static struct timeval tv; \
		gettimeofday(&tv, NULL); \
		res = tv.tv_sec * _OSDEP_CLK_TCK + \
			tv.tv_usec / (USEC_PER_SEC / _OSDEP_CLK_TCK); \
	}

#define _OSDEP_SHORT_TIMER_THRESHOLD	1

/*
 * Spec says we must round up to the next interval, so either compute
 * this mess or always add one.
 */
#define _OSDEP_NANOSEC_TO_TICKS(t)	\
	(((t) % (NSEC_PER_SEC / POSIX_TIMER_HZ)) ? \
		((t) / (NSEC_PER_SEC / POSIX_TIMER_HZ)) + 1 : \
		((t) / (NSEC_PER_SEC / POSIX_TIMER_HZ)))
#define _OSDEP_MICROSEC_TO_TICKS(t)	\
	(((t) % (USEC_PER_SEC / POSIX_TIMER_HZ)) ? \
		((t) / (USEC_PER_SEC / POSIX_TIMER_HZ)) + 1 : \
		((t) / (USEC_PER_SEC / POSIX_TIMER_HZ)))
#define _OSDEP_SEC_TO_TICKS(t)		((t) * POSIX_TIMER_HZ)

#define _OSDEP_CURR_TIME_IN_TICKS 	_udi_get_cur_time_in_ticks()
#define _OSDEP_TIME_T_TO_TICKS(t) 	\
	(_OSDEP_SEC_TO_TICKS((t).seconds) + \
	 _OSDEP_NANOSEC_TO_TICKS((t).nanoseconds))

/*
 * This is simple, but very wrong.   I suspect the "right" approach
 * probably involves multiple threads, analyzing the impact of having
 * multiple simultaneous interval timers active on a thread, building
 * a list of timers that are about to expire and so on.
 */
#define _OSDEP_TIMER_T	int

_OSDEP_TIMER_T osdep_timer_start(void (*callback),
				 void *arg,
				 udi_ubit32_t interval,
				 udi_boolean_t repeats);

void osdep_timer_cancel(void (*callback) (void),
			void *arg,
			_OSDEP_TIMER_T tid);

#define _OSDEP_TIMER_START(func, arg, interval, autoreload) \
	osdep_timer_start((void (*))func, arg, interval, autoreload)
#define _OSDEP_TIMER_CANCEL(func, arg, tid) \
	osdep_timer_cancel(func, arg, tid)


void osdep_signal_alloc_daemon(void);

#define _OSDEP_NEW_ALLOC() osdep_signal_alloc_daemon()

/*
 * Switch on MALLOC_POSIX_TYPES to make all mutexes, events and atomic ints
 * be tracked by malloc.
 * Note: If you change this switch's state, you should completely rebuild
 *       the environment. (make distclean ; ./configure ; make)
 */

/* We want the most test-coverage in POSIX, so this will be enabled by default. */
#define MALLOC_POSIX_TYPES

/*
 * Switch on _POSIX_SEMAPHORES if your environment supports Unix 98 semaphores.
 * Note: If you change this switch's state, you should completely rebuild
 *       the environment. (make distclean ; ./configure ; make)
 */

/*
 * Switch on POSIX_TYPES_TRACKER to make posix types get registered with
 * the allocation_log.
 * This switch is disabled when MALLOC_POSIX_TYPES is enabled since all
 * mallocs are automagically tracked.
 * Note: If you change this switch's state, you should completely rebuild
 *       the environment. (make distclean ; ./configure ; make)
 */

/*
 * Let the OSDEP code override the type of PTHREAD_MUTEX we use in our locks.
 * If no type is specified we fall back to PTHREAD_MUTEX_NORMAL which implies no
 * error-checking, so deadlocks may occur
 */
#ifndef _POSIX_MUTEX_TYPE
#ifdef PTHREAD_MUTEX_NORMAL
#define _POSIX_MUTEX_TYPE PTHREAD_MUTEX_NORMAL
#else
#define _POSIX_MUTEX_TYPE 0
#endif
#endif

/*
 * Configuration possibilities for the switches above.
 *
 * Either turn on the switches here, or turn them on globally,
 * but don't turn them on for just a few files at a time!
 */
#if 0

/* Level 0 debugging: struct-like no-malloc-needed data types. */

/* Using "lint" must be done with this configuration. */

/* Currently, only UnixWare deals with lint. */

/***#define lint***/

/*
 * Optional: enable posix semaphores. This should be done in
 * your env/<>-user udiprops.txt's compile_options and
 * udi_osdep.mk's DEFS variable.
 */

/***#define _POSIX_SEMAPHORES***/

#elif 1

/* Level 1 debugging: enable type tracker for resource misuse. */
#define POSIX_TYPES_TRACKER

#elif 1

/* Level 2 debugging: malloc data types. */
#define MALLOC_POSIX_TYPES

#endif

/* Fix up a combination that doesn't make sense. */
#if defined(MALLOC_POSIX_TYPES) && defined(POSIX_TYPES_TRACKER)

/* Mallocing posix types automagicly tracks init/deinit. */
#undef POSIX_TYPES_TRACKER
#endif

#ifdef POSIX_TYPES_TRACKER
#define ALLOCATION_LOG_ADD(mblk_p) \
	{ \
	(mblk_p)->allocator_linenum = __LINE__ ;  \
 	(mblk_p)->allocator_fname = __FILE__   ;  \
	posix_add_to_alloc_log((mblk_p)); \
	}
#define ALLOCATION_LOG_DELETE(mblk_p) \
	{ \
	posix_delete_from_alloc_log((mblk_p)); \
	}
#else
#define ALLOCATION_LOG_ADD(mblk_p)
#define ALLOCATION_LOG_DELETE(mblk_p)
#endif

#ifndef MALLOC_POSIX_TYPES

/****************************************************************************
 * Non-malloc'd versions of posix osdep data types
 ****************************************************************************/

/*
 * _OSDEP_MUTEX_T -- Non-recursive mutex lock
 */
struct _mutex {
	pthread_mutex_t lock;
#ifdef POSIX_TYPES_TRACKER
	_mblk_header_t mblk;
#endif
};

/*
 * To help find lock leaks, 
 */
udi_queue_t mutex_queue_head;

#define _OSDEP_MUTEX_T	struct _mutex

#ifdef lint
#define _OSDEP_MUTEX_INIT(mutexp, lock_name) \
		{ \
		static pthread_mutexattr_t mutexattr; \
		pthread_mutexattr_init(&mutexattr); \
		pthread_mutexattr_settype(&mutexattr, _POSIX_MUTEX_TYPE); \
		pthread_mutex_init(mutexp.lock, &mutexattr); \
		ALLOCATION_LOG_ADD(&((mutexp).mblk)); \
		}

#define _OSDEP_MUTEX_LOCK(mutexp) 	\
		pthread_mutex_lock((mutexp).lock)

#define _OSDEP_MUTEX_UNLOCK(mutexp) 	\
		pthread_mutex_unlock((mutexp).lock)

#define _OSDEP_MUTEX_DEINIT(mutexp)	\
		ALLOCATION_LOG_DELETE(&((mutexp).mblk));

#else

#define _OSDEP_MUTEX_INIT(mutexp, lock_name) \
		{ \
		static pthread_mutexattr_t mutexattr; \
		int _init_return; \
		pthread_mutexattr_init(&mutexattr); \
		pthread_mutexattr_settype(&mutexattr, _POSIX_MUTEX_TYPE); \
		_init_return = pthread_mutex_init(&((mutexp)->lock), &mutexattr); \
		assert(0 == _init_return); \
		ALLOCATION_LOG_ADD(&((mutexp)->mblk)); \
		}

#define _OSDEP_MUTEX_LOCK(mutexp) 	\
		{ \
		int _lock_return; \
		_lock_return = pthread_mutex_lock(&(mutexp)->lock); \
		assert(0 == _lock_return); \
		}

#define _OSDEP_MUTEX_UNLOCK(mutexp) 	\
		{ \
		int _unlock_return; \
		_unlock_return = pthread_mutex_unlock(&(mutexp)->lock); \
		assert(0 == _unlock_return);\
		}

#define _OSDEP_MUTEX_DEINIT(mutexp) \
		{ \
		ALLOCATION_LOG_DELETE(&(mutexp)->mblk); \
		}

#endif /* lint */

/* 
 *  Event Macros.
 */
struct __osdep_event_t {
	pthread_cond_t osdep_cond;
	_OSDEP_MUTEX_T osdep_mutex;
	udi_ubit32_t osdep_flag;
};

#define _OSDEP_EVENT_T	struct __osdep_event_t

#define _OSDEP_EVENT_WAIT(ev) \
	{ \
	  _OSDEP_MUTEX_LOCK(&(ev)->osdep_mutex); \
	  while ( ! (ev)->osdep_flag ) { \
	     pthread_cond_wait(&(ev)->osdep_cond, &(ev)->osdep_mutex.lock); \
	  } \
	  (ev)->osdep_flag = 0 ; \
	  _OSDEP_MUTEX_UNLOCK(&(ev)->osdep_mutex); \
	}

#define _OSDEP_EVENT_INIT(ev) _OSDEP_EVENT_INIT_HELPER(ev, __FILE__, __LINE__)
#define _OSDEP_EVENT_INIT_HELPER(ev, file, line)  \
	pthread_cond_init(&(ev)->osdep_cond, NULL); \
	_OSDEP_MUTEX_INIT(&(ev)->osdep_mutex, file # line); \
	(ev)->osdep_flag = 0;

#define	_OSDEP_EVENT_SIGNAL(ev) \
	{ \
	  _OSDEP_MUTEX_LOCK(&(ev)->osdep_mutex); \
	  (ev)->osdep_flag = 1; \
	  _OSDEP_MUTEX_UNLOCK(&(ev)->osdep_mutex); \
	  pthread_cond_signal(&(ev)->osdep_cond); \
	}
#define _OSDEP_EVENT_DEINIT(ev) \
	_OSDEP_MUTEX_DEINIT(&(ev)->osdep_mutex);

#else

/****************************************************************************
 * Malloc'd versions of posix osdep data types
 ****************************************************************************/

extern int debug_osdep_mutex_t;
extern int debug_osdep_event_t;
extern int debug_osdep_atomic_int_t;

/*
 * _OSDEP_MUTEX_T -- Non-recursive mutex lock
 * This implementation is based on using malloc to help validate
 * resource allocation and disposal. An in-kernel implementation of UDI
 * might not use this implementation since it is sub-optimal. But it
 * may need to use it if there are problems with sharing stack based
 * variables between kernel threads.
 */

struct _osdep_mutex {
	pthread_mutexattr_t mutexattr;
#ifdef POSIX_TYPES_TRACKER
	_mblk_header_t mblk;
#endif
	pthread_mutex_t lock;
	char *lockname;
};

udi_queue_t mutex_queue_head;

typedef struct _osdep_mutex _osdep_mutex;
typedef _osdep_mutex *_osdep_mutex_t;

#define _OSDEP_MUTEX_T	_osdep_mutex_t

#ifdef lint
#define _OSDEP_MUTEX_INIT(mutex_param, lock_name) \
		{ \
		_osdep_mutex *mutexp; \
		mutexp = (_osdep_mutex*)_OSDEP_MEM_ALLOC( \
				sizeof(_osdep_mutex), 0, UDI_WAITOK); \
		_OSDEP_ASSERT(mutexp != NULL); \
		if (debug_osdep_mutex_t) { \
			memset(mutexp, 0xCC, sizeof(_osdep_mutex)); \
		} \
		*(mutex_param) = mutexp; \
		pthread_mutexattr_init(&mutexp->mutexattr); \
		pthread_mutexattr_settype(&mutexp->mutexattr, \
						_POSIX_MUTEX_TYPE); \
		pthread_mutex_init(&mutexp->lock, &mutexp->mutexattr); \
		}

#define _OSDEP_MUTEX_LOCK(mutexp) 	\
		pthread_mutex_lock(&(*(mutexp))->lock)
#define _OSDEP_MUTEX_UNLOCK(mutexp) 	\
		pthread_mutex_unlock(&(*(mutexp))->lock)

#define _OSDEP_MUTEX_DEINIT(mutexp) \
		{ \
		int rv; \
		if (EBUSY == pthread_mutex_trylock(&(*(mutexp))->lock)) { \
			perror("pthread_mutex_deinit\n"); \
		} else { \
			pthread_mutex_exit(&(*(mutexp))->lock); \
		} \
		rv = pthread_mutex_destroy(&(*(mutexp))->lock); \
		ASSERT(rv == 0); \
		(void)pthread_mutexattr_destroy(&(*(mutexp))->mutexattr) ; \
		if (debug_osdep_mutex_t) { \
			memset(*(mutexp), 0xDD, sizeof(_osdep_mutex)); \
		} \
		_OSDEP_MEM_FREE(*(mutexp)); \
		*(mutexp) = NULL; \
		}
#else

#define _OSDEP_MUTEX_INIT(mutex_param, lock_name) \
		{ \
		int _init_return; \
		_osdep_mutex *mutexp; \
		mutexp = (_osdep_mutex*)_OSDEP_MEM_ALLOC( \
					sizeof(_osdep_mutex), 0, UDI_WAITOK); \
		if (debug_osdep_mutex_t > 1) { \
			_OSDEP_PRINTF("MUTEX_INIT:%s:%s:%d(%p:%p)\n", \
			( (lock_name) == NULL ? "<unnamed>" : (lock_name) ), \
				__FILE__, __LINE__, (mutex_param), mutexp); \
		} \
		_OSDEP_ASSERT(mutexp != NULL); \
		if (debug_osdep_mutex_t) { \
			memset(mutexp, 0xCC, sizeof(_osdep_mutex)); \
		} \
		*(mutex_param) = mutexp; \
		mutexp->lockname = (lock_name); \
		pthread_mutexattr_init(&mutexp->mutexattr); \
		pthread_mutexattr_settype(&mutexp->mutexattr, \
						_POSIX_MUTEX_TYPE); \
		_init_return = pthread_mutex_init(&mutexp->lock, \
						&mutexp->mutexattr); \
		assert(0 == _init_return); \
		ALLOCATION_LOG_ADD(&((mutexp)->mblk)); \
		}
#define _OSDEP_MUTEX_LOCK(mutexp) \
		{ \
		int _lock_return; \
		if (debug_osdep_mutex_t > 1) { \
			_OSDEP_PRINTF("MUTEX_LOCK:%s:%s:%d(%p:%p)\n", \
				(*(mutexp))->lockname, __FILE__, __LINE__, \
				mutexp, *(mutexp)); \
		} \
		_lock_return = pthread_mutex_lock(&(*(mutexp))->lock); \
		assert(0 == _lock_return); \
		}

#define _OSDEP_MUTEX_UNLOCK(mutexp) \
		{ \
		int _unlock_return; \
		if (debug_osdep_mutex_t > 1) { \
			_OSDEP_PRINTF("MUTEX_UNLOCK:%s:%s:%d(%p:%p)\n", \
				(*(mutexp))->lockname, __FILE__, __LINE__, \
				 mutexp, *(mutexp)); \
		} \
		_unlock_return = pthread_mutex_unlock(&(*(mutexp))->lock); \
		assert(0 == _unlock_return); \
		}

#define _OSDEP_MUTEX_DEINIT(mutexp) \
		{ \
		int rv; \
		if (debug_osdep_mutex_t > 1) { \
			_OSDEP_PRINTF("MUTEX_DEINIT:%s:%s:%d(%p:%p)\n", \
				(*(mutexp))->lockname, __FILE__, __LINE__, \
				mutexp, *(mutexp)); \
		} \
		rv = pthread_mutex_destroy(&(*(mutexp))->lock); \
		_OSDEP_ASSERT(rv == 0); \
		(void)pthread_mutexattr_destroy(&(*(mutexp))->mutexattr); \
		ALLOCATION_LOG_DELETE(&((*(mutexp))->mblk)); \
		if (debug_osdep_mutex_t) { \
			memset(*(mutexp), 0xDD, sizeof(_osdep_mutex)); \
		} \
		_OSDEP_MEM_FREE(*(mutexp)); \
		*(mutexp) = NULL; \
		}

#endif /* lint */


/* 
 *  Event Macros.
 */
struct _osdep_event {
	pthread_cond_t osdep_cond;
	_OSDEP_MUTEX_T osdep_mutex;
	udi_ubit32_t osdep_flag;
};

typedef struct _osdep_event _osdep_event;
typedef struct _osdep_event *_osdep_event_t;

#define _OSDEP_EVENT_T	_osdep_event_t

#define _OSDEP_EVENT_WAIT(ev) \
	{ \
	  if (debug_osdep_event_t > 1) { \
		_OSDEP_PRINTF("EVENT_WAIT:%s:%d(%p:%p)\n", \
					__FILE__, __LINE__, ev,*ev); \
	  } \
	  _OSDEP_MUTEX_LOCK(&((*(ev))->osdep_mutex)); \
	  while ( ! (*(ev))->osdep_flag ) { \
	     pthread_cond_wait(&((*(ev))->osdep_cond), \
					&((*(ev))->osdep_mutex->lock)); \
	  } \
	  (*(ev))->osdep_flag = 0 ; \
	  _OSDEP_MUTEX_UNLOCK(&((*(ev))->osdep_mutex)); \
	}

#define _OSDEP_EVENT_INIT(ev_var) _OSDEP_EVENT_INIT_HELPER(ev_var, __FILE__, __LINE__)
#define _OSDEP_EVENT_INIT_HELPER(ev_var, file, line)  \
	{ \
	_osdep_event *ev; \
	ev = (_osdep_event*)_OSDEP_MEM_ALLOC(sizeof(_osdep_event), 0, UDI_WAITOK); \
	if (debug_osdep_event_t > 1) { \
		_OSDEP_PRINTF("EVENT_INIT:%s:%d(%p:%p)\n", \
				__FILE__, __LINE__, (ev_var), ev); \
	} \
	if (debug_osdep_event_t) memset(ev, 0xCC, sizeof(_osdep_event)); \
	_OSDEP_ASSERT(ev != NULL); \
	*(ev_var) = ev; \
	pthread_cond_init(&(ev->osdep_cond), NULL); \
	_OSDEP_MUTEX_INIT(&(ev->osdep_mutex), file # line); \
	ev->osdep_flag = 0; \
	}

#define	_OSDEP_EVENT_SIGNAL(ev) { \
	if (debug_osdep_event_t > 1) { \
		_OSDEP_PRINTF("EVENT_SIGNAL:%s:%d(%p:%p)\n", \
				__FILE__, __LINE__, (ev), *(ev)); \
	} \
	_OSDEP_MUTEX_LOCK(&(*(ev))->osdep_mutex); \
	(*(ev))->osdep_flag = 1; \
	_OSDEP_MUTEX_UNLOCK(&(*(ev))->osdep_mutex); \
	pthread_cond_signal(&(*(ev))->osdep_cond); \
	}

#define _OSDEP_EVENT_DEINIT(ev) { \
	if (debug_osdep_event_t > 1) { \
		_OSDEP_PRINTF("EVENT_DESTROY:%s:%d(%p:%p)\n", \
				__FILE__, __LINE__, (ev), *(ev)); \
	} \
	pthread_cond_destroy(&(*(ev))->osdep_cond); \
	_OSDEP_MUTEX_DEINIT(&(*(ev))->osdep_mutex); \
	if (debug_osdep_event_t) memset(*(ev), 0xDD, sizeof(_osdep_event)); \
	_OSDEP_MEM_FREE(*(ev)); \
	*(ev) = NULL; \
	}

#endif /* MALLOC_POSIX_TYPES */

/*
 * Atomic Int Macros.
 * These use the Posix semaphores [an optional extension]. If we don't have
 * this support we fall back to the heavyweight _OSDEP_SIMPLE_MUTEX lock
 * alternative
 */

#ifdef _POSIX_SEMAPHORES

struct _osdep_atomic_int {
	sem_t sem;
#ifdef POSIX_TYPES_TRACKER
	_mblk_header_t mblk;
#endif
};
typedef struct _osdep_atomic_int _osdep_atomic_int;

#ifndef MALLOC_POSIX_TYPES
typedef _osdep_atomic_int _osdep_atomic_int_t;
#else
typedef _osdep_atomic_int *_osdep_atomic_int_t;
#endif

#define	_OSDEP_ATOMIC_INT_T	_osdep_atomic_int_t

#else  /* No semaphore support - use old version */

struct _osdep_atomic_int {
	pthread_mutex_t amutex;
	int aval;
#ifdef POSIX_TYPES_TRACKER
	_mblk_header_t mblk;
#endif
};
typedef struct _osdep_atomic_int _osdep_atomic_int;

#ifndef MALLOC_POSIX_TYPES
typedef _osdep_atomic_int _osdep_atomic_int_t;
#else
typedef _osdep_atomic_int *_osdep_atomic_int_t;
#endif

#define	_OSDEP_ATOMIC_INT_T	_osdep_atomic_int_t

#endif /* _POSIX_SEMAPHORES */

extern void _osdep_atomic_int_init(_OSDEP_ATOMIC_INT_T *,
				   int);
extern void _osdep_atomic_int_incr(_OSDEP_ATOMIC_INT_T *);
extern void _osdep_atomic_int_decr(_OSDEP_ATOMIC_INT_T *);
extern void _osdep_atomic_int_deinit(_OSDEP_ATOMIC_INT_T *);
extern int _osdep_atomic_int_read(_OSDEP_ATOMIC_INT_T *);

#define	_OSDEP_ATOMIC_INT_INIT(atomic_intp, ival)	\
	_osdep_atomic_int_init((atomic_intp), (ival))

#define	_OSDEP_ATOMIC_INT_INCR(atomic_intp)		\
	_osdep_atomic_int_incr((atomic_intp))

#define	_OSDEP_ATOMIC_INT_DECR(atomic_intp)		\
	_osdep_atomic_int_decr((atomic_intp))

#define	_OSDEP_ATOMIC_INT_DEINIT(atomic_intp)		\
	_osdep_atomic_int_deinit((atomic_intp))

#define	_OSDEP_ATOMIC_INT_READ(atomic_intp)		\
	_osdep_atomic_int_read((atomic_intp))



/*
 * Provide instrumented memory allocators that ultimately just call 
 * malloc(). 
 */

void *osdep_malloc(size_t size,
		   int flags,
		   int sleep_ok,
		   int line,
		   char *fname);
void osdep_free(void *buf);

extern volatile size_t Allocd_mem;

#define _OSDEP_MEM_ALLOC(size, flags, sleep_ok) \
                osdep_malloc((size), (flags), (sleep_ok), __LINE__, __FILE__)
#define _OSDEP_MEM_FREE(mem) \
                osdep_free((mem))

/*
 *  PIO customization.
 */
#define _OSDEP_PIO_DELAY(usec) usleep(usec)

/*
 * Tracing and logging customization
 */

extern FILE *_udi_log_file;
extern udi_trevent_t def_tracemask, def_ml_tracemask;

#define _OSDEP_TRLOG_INIT { int ii;					\
                            _udi_log_file = fopen("UDI.log", "w");	\
                            _udi_trlog_initial = def_tracemask;		\
			    for (ii=0; ii < 256; ii++)			\
			        _udi_trlog_ml_initial[ii] = def_ml_tracemask; }

#define _OSDEP_TRLOG_DONE { fclose(_udi_log_file); }

#define _OSDEP_LOG_DATA(S,L,CB) ( fwrite((S), 1, (L), _udi_log_file), \
                                  fwrite("\n", 1, 1, _udi_log_file), \
                                  fflush(_udi_log_file) )

/* todo: who closes the log file and when? */

/*
 * udi_limits_t hooks.
 */
void posix_get_limit_t(udi_limits_t *);

#define _OSDEP_GET_LIMITS_T(l) \
	posix_get_limit_t(l)


#ifndef KWQ
#include <udi_std_sprops.h>
extern _udi_std_sprops_t *_udi_get_elf_sprops(const char *exec_img_file,
					      const char *driver_obj_file,
					      const char *driver_name);
typedef _udi_std_sprops_t _udi_sprops_t;
#endif

typedef enum {
	bt_unknown,
	bt_pci,
	bt_system
} posix_bus_type_t;

typedef struct posix_sysbus {
	udi_ubit32_t sysbus_mem_addr_lo;
	udi_ubit32_t sysbus_mem_addr_hi;
	udi_ubit32_t sysbus_mem_size;
	udi_ubit32_t sysbus_io_addr_lo;
	udi_ubit32_t sysbus_io_addr_hi;
	udi_ubit32_t sysbus_io_size;
	udi_ubit32_t sysbus_intr0;
	udi_ubit32_t sysbus_intr1;
} posix_sysbus_t;

typedef struct posix_dev_node {
	FILE *backingstore;
	posix_bus_type_t bus_type;
	posix_sysbus_t sysbus_data;
	unsigned long node_pci_unit_address;
} posix_dev_node_t;

#define POSIX_DEV_NODE_T  posix_dev_node_t posix_dev_node;

extern int posix_dev_node_init(_udi_driver_t *driver,
			       _udi_dev_node_t *udi_node);
extern int posix_dev_node_deinit(_udi_dev_node_t *udi_node);


/* this is the osinfo for the intr_attach_context */

struct _posix_intr_osinfo {
	void *udi_attach_info;
	int (*isr) (void *);
};
typedef struct _posix_intr_osinfo _posix_intr_osinfo_t;

#define _OSDEP_INTR_T _posix_intr_osinfo_t

/* move the udi_attach_info pointer to this struct */

/* move the isr pointer to this struct */

/* LEFT OFF HERE */

/* A struct that simplifies bridge mapper enumeration. */
struct posix_pci_device_item {
	udi_ubit32_t device_present;
	udi_ubit32_t pci_vendor_id;	/* 16 bits */
	udi_ubit32_t pci_device_id;	/* 16 bits */
	udi_ubit32_t pci_revision_id;	/* 8 bits */
	udi_ubit32_t pci_base_class;	/* 8 bits */
	udi_ubit32_t pci_sub_class;	/* 8 bits */
	udi_ubit32_t pci_prog_if;	/* 8 bits */
	udi_ubit32_t pci_subsystem_vendor_id;	/* 16 bits */
	udi_ubit32_t pci_subsystem_id;	/* 16 bits */
	udi_ubit32_t pci_unit_address;	/* 16 bits */
	udi_ubit32_t pci_slot;		/* 8 bits */
};
typedef struct posix_pci_device_item posix_pci_device_item_t;

/*
 * This function should return how many posix_pci_device_item_t
 * elements it needs, cooresponding to PCI devices in the system.
 */
extern int posixuser_num_pci_devices(void);
#ifndef POSIXUSER_NUM_PCI_DEVICES
#define POSIXUSER_NUM_PCI_DEVICES() 0
#endif

/*
 * This function should fill in the cooresponding
 * posix_pci_device_item_t elements of the passed dev_list.
 */
extern void posixuser_enumerate_pci_devices(
			posix_pci_device_item_t *dev_list);

#ifndef POSIXUSER_ENUMERATE_PCI_DEVICES
#define POSIXUSER_ENUMERATE_PCI_DEVICES(dev_list)
#endif

/* this is the osinfo for the bridge_region */
typedef struct _osdep_bridge_child_data {
	int irq;
} __osdep_bridge_child_data_t;
#define _OSDEP_BRIDGE_CHILD_DATA_T __osdep_bridge_child_data_t

udi_status_t _posix_stub_register_isr(int (*isr) (void *),
				      udi_cb_t *gcb,
				      void *context,
				      void *enumeration_osinfo,
				      void *attach_osinfo);
udi_status_t _posix_stub_unregister_isr(int (*isr) (void *),
					udi_cb_t *gcb,
					void *context,
					void *enumeration_osinfo,
					void *attach_osinfo);
void _posix_stub_isr_acknowledge(udi_ubit8_t intr_status);
void *_posix_stub_get_isr_context(void *attach_osinfo);
void *_posix_stub_get_event_info(void *_context);
int _posix_stub_isr_return_val(udi_ubit8_t intr_status);

#define _OSDEP_ENUMERATE_PCI_DEVICE _posix_stub_enumerate_pci_device
#define _OSDEP_REGISTER_ISR             _posix_stub_register_isr
#define _OSDEP_UNREGISTER_ISR           _posix_stub_unregister_isr
#define _OSDEP_ISR_ACKNOWLEDGE          _posix_stub_isr_acknowledge
#define _OSDEP_GET_ISR_CONTEXT          _posix_stub_get_isr_context
#define _OSDEP_GET_EVENT_INFO           _posix_stub_get_event_info
#define _OSDEP_ISR_RETURN_VAL           _posix_stub_isr_return_val

#define _OSDEP_INSTRUMENT_ALLOC_FAIL	(rand() < alloc_thresh)
extern unsigned int callback_limit;
extern unsigned int posix_force_cb_realloc;
extern unsigned int posix_defer_mei_call;

#define _OSDEP_CALLBACK_LIMIT		(callback_limit)
#define _OSDEP_FORCE_CB_REALLOC (posix_force_cb_realloc && \
			((ifp->if_meta->meta_idx != 0)  && (op_idx != 0)))


udi_status_t __osdep_instance_attr_set(struct _udi_dev_node *node,
				       const char *name,
				       const void *value,
				       udi_size_t length,
				       udi_ubit8_t type);

udi_size_t __osdep_instance_attr_get(struct _udi_dev_node *node,
				     const char *name,
				     void *value,
				     udi_size_t length,
				     udi_ubit8_t *type);

/*
#define _OSDEP_INSTANCE_ATTR_SET (node, name, value, len, type) \
		__osdep_instance_attr_set(node, name, value, len, type)
#define _OSDEP_INSTANCE_ATTR_GET (node, name, value, len, type) \
		__osdep_instance_attr_get(node, name, value, len, type)
*/

#define _OSDEP_INSTANCE_ATTR_SET __osdep_instance_attr_set
#define _OSDEP_INSTANCE_ATTR_GET __osdep_instance_attr_get

extern udi_queue_t my_region_q_head;
extern _OSDEP_MUTEX_T my_region_q_lock;
extern _OSDEP_EVENT_T rgn_daemon_event;

#define _OSDEP_DEFER_MEI_CALL(region, op_flag)  (posix_defer_mei_call || \
       !UDI_QUEUE_EMPTY(&region->reg_queue) || REGION_IS_ACTIVE(region) || op_flag & UDI_OP_LONG_EXEC)

/*
 * NOTE: the REGION_LOCK is a SIMPLE_MUTEX_T and so can be taken inside the
 *      _OSDEP_MUTEX_LOCK. However, we are called with the lock held so we
 *      must drop it (after marking the region as being scheduled) before
 *      acquiring the _OSDEP_MUTEX_LOCK
 */
#define _OSDEP_REGION_SCHEDULE(region) \
	_OSDEP_ASSERT(region->in_sched_q == FALSE); \
        region->in_sched_q = TRUE; \
        REGION_UNLOCK(region); \
	_OSDEP_MUTEX_LOCK(&my_region_q_lock); \
	UDI_ENQUEUE_TAIL(&my_region_q_head, &region->reg_sched_link); \
	_OSDEP_MUTEX_UNLOCK(&my_region_q_lock); \
	_OSDEP_EVENT_SIGNAL(&rgn_daemon_event);

#endif /* _POSIX_ENV_H */
