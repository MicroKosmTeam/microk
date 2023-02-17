/*
 * File: linux/udi_osdep.c
 *
 * Linux-specific component of environment.
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

#ifdef FASTLOG
#define LOG_BUF_SIZE 2*1024*1024UL
unsigned long logxi = 0;
unsigned char logx[LOG_BUF_SIZE];
#endif

#ifdef INSTRUMENT_MALLOC
/*
 * Keep track of some allocation statistics
 */
unsigned long no_kmallocs = 0, no_kfrees = 0, no_vmallocs = 0, no_vfrees = 0;
#endif

/*
 * To combat kernel & UDI namespace conflicts, all kernel
 * specific data types and function calls were made opaque.
 * The opaque definitions are defined here.
 * Anyone who expects to use the members of the opaque data
 * types (mappers, etc) must use the #include order shown below.
 */

/* Start required #include order to access opaque defns */
#include "udi_osdep_opaquedefs.h"
#include <udi_env.h>
#include <udi_std_sprops.h>
#include <linux/wait.h> /* Pickup waitqueue types */
/* End required #include order to access opaque defns */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
#define SIMPLER_EVENTS 1
#else
#define SIMPLER_EVENTS 0
#endif

#ifndef MAX_ORDER
/* A Linux 2.2.x limit */
#define MAX_ORDER 5
#endif

#ifndef DECLARE_WAITQUEUE
#define DECLARE_WAITQUEUE(wait, current) \
	struct wait_queue (wait) = { .task = (current), .next = NULL };
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
#define __set_task_state(tsk, new_state) do { (tsk)->state = (new_state); } while (0)

#if defined (__i386__)
#define local_irq_save(flags) do { __save_flags(flags); __cli(); } while (0)
#define local_irq_restore(flags)  __restore_flags(flags)
#else
#define local_irq_save(flags) do { save_flags(flags); cli(); } while (0)
#define local_irq_restore(flags) restore_flags(flags)
#endif

#ifndef init_waitqueue_head
#define init_waitqueue_head(q) init_waitqueue(q)
#endif
#endif	

#define LINUX_MAX_LEGAL_ALLOC ((1 << MAX_ORDER) * PAGE_SIZE)

#include <linux/vmalloc.h>
#include <asm/io.h> /* virt_to_bus */
#include <linux/wrapper.h> /* mem_map_* */

/*
 * Functions meant for udisetup wrapper code to call into.
 * Keep these synchronized with tools/linux/osdep_make.h.
 */
int _udi_meta_load(const char *modname, udi_mei_init_t * meta_info, 
		  _udi_sprops_idx_t * sprops, struct module *mod);
int _udi_meta_unload(const char *modname);

int _udi_mapper_load(const char *modname, udi_init_t * init_info, 
		    _udi_sprops_idx_t * sprops, struct module *mod);
int _udi_mapper_unload(const char *modname);

int _udi_driver_load(const char *modname, udi_init_t * init_info, 
		    _udi_sprops_idx_t * sprops, struct module *mod);
void _udi_driver_unload(const char *modname);

/* **** */

void _udi_osdep_printf(const char *fmt, ...)
{
#ifdef FASTLOG
    char* out2;
#endif
    /* TBD: This 200 char limitation should be fixed. */
    char out[200];
    int k;
    va_list args;
    va_start(args, fmt);
    k = vsprintf(out, fmt, args);
#ifdef FASTLOG
    out2 = &logx[logxi];
    if (k + logxi > LOG_BUF_SIZE) {
    	memcpy(out2, out, LOG_BUF_SIZE - logxi);
	memcpy(logx, out + LOG_BUF_SIZE - logxi, k + logxi - LOG_BUF_SIZE);
    } else
	memcpy(out2, out, k);
	
    logxi += k;
    logxi %= LOG_BUF_SIZE;
#endif    	
    va_end(args);
    /* Don't let printk interpret this string. */
#ifndef FASTLOG
    printk("%s", out);
#else
    console_print(out);
#endif
}

#ifdef CONFIG_KDB
#include <linux/kdb.h>
#endif

/* Called by the environment, usually. */
void _udi_osdep_assert(const char *file, int line, const char *expr_text)
{
    _OSDEP_PRINTF("%s:%d assertion failed: %s\n", file, line, expr_text);
    /*
     * Assertions are fatal. We need to cause the OS to stop this thread
     * of execution... dereference null, write a bad value to it.
     */
#ifdef CONFIG_KDB
    _OSDEP_PRINTF("Entering kdb... skip the NULL deref to ignore assert.\n");
    KDB_ENTER();
#endif
    if (in_interrupt()) {
	/*
	 * Failing on interrupt is painful. Backtraces wont be saved.
	 * Now would be a good time to install a bottom half handler and
	 * exit the interrupt handler.
	 * 
	 * That's not an easy solution, so lets just delay so the
	 * user can see the error before the kernel prints out a
	 * monster backtrace due to the following null dereference.
	 */
	 volatile unsigned long x = 0xFFFFFFFF;
	_OSDEP_PRINTF("Delaying system PANIC due to a failed assertion on interrupt...\n");
	while (x--) {;}
    }
dontReturn:
    *(int*)0 = 0xDEADFEE1;
    goto dontReturn;
}

/* Called by UDI drivers */
void __udi_assert(const char *str, const char *file, int line)
{
    /*
     * TBD: cause a udi_region_kill to happen to the driver that
     * failed an assertion.
     */
    _udi_osdep_assert(file, line, str);
    return;
}

void _udi_osdep_debug_break(void)
{
#ifdef CONFIG_KDB
    KDB_ENTER();
#else
    /*
     * Production drivers SHOULD NOT be using udi_debug_break,
     * with non-debug kernels, spank you!
     */
    volatile int x = 0;
    _OSDEP_ASSERT(x && "udi_debug_break is not to be used in RELEASE builds.");
#endif
}


/* Debug flags */
extern int _udi_osdep_sem_debug;
extern int _udi_osdep_event_debug;
extern int _udi_osdep_mutex_debug;

int _udi_osdep_sem_debug = 0;
int _udi_osdep_event_debug = 0;
int _udi_osdep_mutex_debug = 0;


void _udi_osdep_sem_init(_OSDEP_SEM_T *sem, long initial)
{
    if (_udi_osdep_sem_debug)
        _OSDEP_PRINTF("SEM_INIT(%p) by %p\n", sem, __builtin_return_address(0));
    udi_memset(sem, 0x00, sizeof(*sem));
    sema_init(&sem->instance, initial);
}

void _udi_osdep_sem_p(_OSDEP_SEM_T *sem)
{
    /*int res; */
#if 0
    unsigned long flags;
#endif
    if (_udi_osdep_sem_debug)
        _OSDEP_PRINTF("SEM_P(%p) by %p\n", sem, __builtin_return_address(0));
    
#if 0
    spin_lock_irqsave(&current->sigmask_lock, flags);
    current->sigpending = 0;
    spin_unlock_irqrestore(&current->sigmask_lock, flags);
#endif
#if 0
    res = down_interruptible(&sem->instance);
    if (_udi_osdep_sem_debug)
	_OSDEP_PRINTF("SEM_P(%p) by %p, down_interruptible returned %d, signal=%#08x%08x\n",
		      sem, __builtin_return_address(0), res, (u32)current->signal.sig[1],(u32)current->signal.sig[0]);	

    spin_lock_irqsave(&current->sigmask_lock, flags);
    recalc_sigpending(current);		      
    spin_unlock_irqrestore(&current->sigmask_lock, flags);
#else
    down(&sem->instance);
#endif
}

void _udi_osdep_sem_v(_OSDEP_SEM_T *sem)
{
    if (_udi_osdep_sem_debug)
        _OSDEP_PRINTF("SEM_V(%p) by %p\n", sem, __builtin_return_address(0));
    up(&sem->instance);
}

void _udi_osdep_sem_deinit(_OSDEP_SEM_T *sem)
{
    if (_udi_osdep_sem_debug)
        _OSDEP_PRINTF("SEM_DEINIT(%p) by %p\n", sem, __builtin_return_address(0));
    udi_memset(sem, 0x00, sizeof(*sem));
}


/* A Tale of Two EVENT_* Implementations */

#if SIMPLER_EVENTS
/* A much simpler event implementation. */
/*
 * This code is ready for production, but temporarily disabled
 * until kernel_thread death's are tracked.
 * If you enable this path, you must also enable the #if 0
 * in udi_osdep_opaquedefs.h to pick up the matching OSDEP_EVENT_T
 * data structure.
 */

void _udi_osdep_event_init(_OSDEP_EVENT_T *ev)
{
    ev->lock = SPIN_LOCK_UNLOCKED;
    init_waitqueue_head(&ev->wait_q);
    /* state indicators */
    ev->signaled = 0;
    ev->destroyed = 0;
    ev->waiting = 0;
    /* counters */
    ev->waiter_waiting = 0;
    ev->waiter_busy = 0;
}

void _udi_osdep_event_wait(_OSDEP_EVENT_T *ev)
{
    unsigned long flags;
    DECLARE_WAITQUEUE(wait, current);

    if (_udi_osdep_event_debug) {
        _OSDEP_PRINTF("EVENT_WAIT(%p) by %p\n", ev, 
		      __builtin_return_address(0));
    }
    spin_lock_irqsave(&ev->lock, flags);
    ev->waiting = 1;
    add_wait_queue(&ev->wait_q, &wait);

    while (!ev->signaled && !ev->destroyed) {
        spin_unlock_irqrestore(&ev->lock, flags);
	__set_task_state(current, TASK_UNINTERRUPTIBLE);
        schedule(); /* sleep until wake_up */
        spin_lock_irqsave(&ev->lock, flags);
    }
    
    __set_task_state(current, TASK_RUNNING);
    remove_wait_queue(&ev->wait_q, &wait);
    ev->signaled = 0;
    ev->waiting = 0;
    spin_unlock_irqrestore(&ev->lock, flags);
}

/*
 * Context: This function must be callable at interrupt time.
 */
void _udi_osdep_event_signal(_OSDEP_EVENT_T *ev)
{
    unsigned long flags;

    if (_udi_osdep_event_debug)
        _OSDEP_PRINTF("EVENT_SIGNAL(%p) by %p\n", ev, __builtin_return_address(0));
    spin_lock_irqsave(&ev->lock, flags);
    if (ev->waiting == 0) {
    /*
     * This does not necessarily indicate an error condition.
     * Say the regiond is busy working on something and is told to schedule.
     * It was doing work, so therefore, it wasn't waiting.
     * This could be used as an indicator to how busy a particular event is...
     */
        ev->waiter_busy++; /* This is a guess that the event waiter was busy. */

        /* If a waiter isn't waiting, we can't wake it up! */
	ev->signaled = 1;
        spin_unlock_irqrestore(&ev->lock, flags);
	return;
    } else {
        ev->waiter_waiting++;
    }
    ev->signaled = 1;
    spin_unlock_irqrestore(&ev->lock, flags);

    wake_up(&ev->wait_q);
}

void _udi_osdep_event_clear(_OSDEP_EVENT_T *ev)
{
    unsigned long flags;

    if (_udi_osdep_event_debug)
        _OSDEP_PRINTF("EVENT_CLEAR(%p) by %p\n", ev, __builtin_return_address(0));
    spin_lock_irqsave(&ev->lock, flags);
    ev->signaled = 0;
    spin_unlock_irqrestore(&ev->lock, flags);
}

void _udi_osdep_event_deinit(_OSDEP_EVENT_T *ev)
{
    unsigned long flags;

    if (_udi_osdep_event_debug)
        _OSDEP_PRINTF("EVENT_DEINIT(%p) by %p\n", ev, __builtin_return_address(0));

    spin_lock_irqsave(&ev->lock, flags);
    ev->signaled = 1;
    ev->destroyed = 1;
    spin_unlock_irqrestore(&ev->lock, flags);

    wake_up(&ev->wait_q);
    /*schedule(); giving the wake_up a chance to run hides the error. */

#if 1 /*on/off*/
    /*
     * This code should eventually be removed when thread-death is tracked
     * for all of the UDI environment daemons.
     * It is not invalid to deinit an event before it has been signaled.
     * However, it is most likely that the programmer didn't intend for this
     * to happen.
     */
    /* Tell the user that an event was waiting. This is a programmer's error. */
    if (ev->waiting != 0) {
        _OSDEP_PRINTF("udi_osdep_event_deinit: ENV PROGRAMMER ERROR: a waiting event was deinited.\n");
        /* rather than crashing, wait for the event to wake up! */
        while (ev->waiting != 0) { /* ev->signaled works too */
#if DEBUG
            _OSDEP_PRINTF("udi_osdep_event_deinit: waiting for an event to wake up.\n");
#endif
            schedule_timeout(30 * HZ / 1000); /* 30ms */
            /*
             * TBD: If timeout finishes a few times, we should
             * kill the waiting kthread.
             */
        }
	/* Let's schedule again and see if they start waiting.. */
        schedule();
        if (ev->waiting != 0) {
            _OSDEP_PRINTF("udi_osdep_event_deinit: it's hopeless, the waiter is waiting again.\n");
            /*
             * If someone saw the above printf, check the environment code.
             * It is likely that some flag telling the EVENT_WAITer to
             * never wait again was not processed.
             */
        }
    }
#if DEBUG
    _OSDEP_PRINTF("udi_osdep_event_deinit: of %d signals sent, this event's waiter was busy %d times, and waiting %d times.\n",
	ev->waiter_busy + ev->waiter_waiting,
	ev->waiter_busy, ev->waiter_waiting);
#endif /*DEBUG*/
#endif /*on/off*/
}

#else

/*
 * _OSDEP_EVENT_T -- Event synchronization variable
 *
 * Events can: be reused; only get one wait/signal pair per use;
 * and the wait/signal pair can come in the wrong order.
 *
 * The original implementation used spinlocks around a wait queue and
 * signalled flag.  A race condition could exist, so we must use
 * an additional flag.  The additional flag is set when someone
 * goes to sleep and then possibly waits around in a bottom half
 * handler for that task to get itself into the wait queue.
 *
 * The "wait" flag is used to prevent a window of opportunity
 * between this code in event_wait:
 *      spin_unlock_irqrestore (&ev->lock, flags);
 *      interruptible_sleep_on (&ev->wait_q);
 *
 * The "dead" & "dying" flags are used to enforce the execution
 * and death of the bottom half waiter.
 */

void __udi_event_signal_wait(_OSDEP_EVENT_T *ev);

STATIC const struct tq_struct _udi_event_signal_bh = {
    NULL,
    0,
    (void (*)(void *)) __udi_event_signal_wait,
    NULL
};

/* For closing a window of opportunity. */
void __udi_event_signal_wait(_OSDEP_EVENT_T *ev)
{
    if ((ev->wait_q == NULL) || (ev->wait_q->task == NULL))
    {
        if (_udi_osdep_event_debug)
            _OSDEP_PRINTF("signal_wait: rescheduling.\n");
        queue_task(&ev->event_signal_bh, &tq_immediate);
        mark_bh(IMMEDIATE_BH);
        return;
    }
    wake_up/*_interruptible*/(&ev->wait_q);
}


/* 
 * Initialize an event variable
 */
void _udi_osdep_event_init(_OSDEP_EVENT_T *ev)
{   
    if (_udi_osdep_event_debug)
        _OSDEP_PRINTF("EVENT_INIT(%p) by %p\n", ev, __builtin_return_address(0));
    udi_memset(ev, 0x00, sizeof(*ev));
    ev->lock = SPIN_LOCK_UNLOCKED;
    ev->wait = ev->dead = ev->dying = ev->signal = 0;
    ev->wait_q = NULL;
    /* Prepare the embedded bottom half handlers. */
    ev->event_signal_bh = _udi_event_signal_bh;
    ev->event_signal_bh.data = ev;
}

/* 
 * De-initialize an event variable
 */
void _udi_osdep_event_deinit(_OSDEP_EVENT_T *ev)
{   
    unsigned long flags;
    if (_udi_osdep_event_debug)
        _OSDEP_PRINTF("EVENT_DEINIT(%p) by %p\n", ev, __builtin_return_address(0));

    /* if the event was outstanding, signal it to end. */
    spin_lock_irqsave (&ev->lock, flags);
    ev->dying = 1;
    if (_udi_osdep_event_debug)
        _OSDEP_PRINTF("Preparing deinit_wait.\n");
    
    if (ev->wait)
    {
	if (_udi_osdep_event_debug)
          _OSDEP_PRINTF("Event (%p) waiting... telling it to end.\n", ev);
    	_OSDEP_SEM_INIT(&ev->evil_dead, 0); /* allocate as acquired */
	spin_unlock_irqrestore (&ev->lock, flags);
	_udi_osdep_event_signal(ev);
        _OSDEP_SEM_P(&ev->evil_dead); /* Acquiring this lock means no waits outstanding. */
	_OSDEP_SEM_DEINIT(&ev->evil_dead);
    }
    else
	spin_unlock_irqrestore (&ev->lock, flags);


    if (_udi_osdep_event_debug)
        _OSDEP_PRINTF("deinit_wait finished.\n");
    _OSDEP_SEM_DEINIT(&ev->evil_dead);
    udi_memset(ev, 0x00, sizeof(*ev));
    ev->dead = 1;
}


/*
 * Wait for an event to be signalled
 *
 * Append the current task (thread) to the wait queue (EVENT_T) passed in
 * and yield till we are woken up.
 */
void _udi_osdep_event_wait(_OSDEP_EVENT_T *ev)
{
    unsigned long flags;
    DECLARE_WAITQUEUE(wait, current);

    if (_udi_osdep_event_debug)
	_OSDEP_PRINTF("EVENT_WAIT(%p) by %p\n", ev, 
		__builtin_return_address(0));
    spin_lock_irqsave(&ev->lock, flags);
    add_wait_queue(&ev->wait_q, &wait);
    while ((!ev->signal) && (!ev->dead) && (!ev->dying)) {
	ev->wait = 1;
	__set_task_state(current, TASK_UNINTERRUPTIBLE);
        spin_unlock_irqrestore(&ev->lock, flags);
	schedule();
	spin_lock_irqsave(&ev->lock, flags);
    }
    __set_task_state(current, TASK_RUNNING);
    remove_wait_queue(&ev->wait_q, &wait);

    ev->wait = ev->signal = 0; /* reinit */
    spin_unlock_irqrestore(&ev->lock, flags);
    /* 
     * Release the semaphore. Say that we've been signalled. 
     * 
     * However, only do that if we're actually dying. We want to make sure
     * that before the V operation the semaphore counter is 0.
     * To that end, we initialize the semaphore in __osdep_event_deinit to 0
     */
    if (ev->dying) {
	if (_udi_osdep_event_debug)
	   _OSDEP_PRINTF("signalling semaphore because we're dying...\n");
	_OSDEP_SEM_V(&ev->evil_dead);
    }
}


/*
 * Signal an event
 *
 * Wake up all of the tasks that are waiting for this signal.
 */
void _udi_osdep_event_signal(_OSDEP_EVENT_T *ev)
{
    unsigned long flags;
    if (ev->signal) return;
    if (_udi_osdep_event_debug)
        _OSDEP_PRINTF("EVENT_SIGNAL(%p) by %p\n", ev, __builtin_return_address(0));
    spin_lock_irqsave(&ev->lock, flags);
    if (ev->dead)
    {
	ev->signal = 1;
        spin_unlock_irqrestore(&ev->lock, flags);
        return;
    }
    ev->signal = 1;
    if (ev->wait) wake_up(&ev->wait_q);
    spin_unlock_irqrestore(&ev->lock, flags);
}

/* 
 * Reset an event
 */    
void _udi_osdep_event_clear(_OSDEP_EVENT_T *ev)
{   
    unsigned long flags;
    if (_udi_osdep_event_debug)
        _OSDEP_PRINTF("EVENT_CLEAR(%p) by %p\n", ev, __builtin_return_address(0));
    spin_lock_irqsave (&ev->lock, flags);
    ev->signal = 0;
    spin_unlock_irqrestore (&ev->lock, flags);
}
#endif


/* Mutexen */

/*
 * Initialize an _OSDEP_MUTEX_T
 *
 * This must be called after (statically or dynamically) allocating an
 * _OSDEP_MUTEX_T before using it. Must not be called from interrupt level or
 * while holding any locks.
 */
void _udi_osdep_mutex_init(_OSDEP_MUTEX_T *mutexp, char *lockname,
				char *filename, unsigned int lineno)
{
    if (_udi_osdep_mutex_debug)
        _OSDEP_PRINTF("MUTEX_INIT(%p,%s) by %p\n", mutexp, lockname, __builtin_return_address(0));
    udi_memset(mutexp, 0x00, sizeof(*mutexp));
    mutexp->lock = SPIN_LOCK_UNLOCKED;
    spin_lock_init(&(mutexp->lock));
    mutexp->lockname = lockname;
    mutexp->filename = filename;
    mutexp->lineno = lineno;
}


/*
 * De-initialize an _OSDEP_MUTEX_T
 *
 * This must be called before freeing an _OSDEP_MUTEX_T. Must not be
 * called from interrupt level or while holding any locks.
 */
void _udi_osdep_mutex_deinit(_OSDEP_MUTEX_T *mutexp)
{
    if (_udi_osdep_mutex_debug)
        _OSDEP_PRINTF("MUTEX_DEINIT(%p) by %p\n", mutexp, __builtin_return_address(0));
    /* clear out the struct? */
    udi_memset(mutexp, 0x00, sizeof(*mutexp));
    mutexp->lock = SPIN_LOCK_UNLOCKED;
}

/*
 * Lock an _OSDEP_MUTEX_T
 *
 * If necessary to prevent a nested call to _OSDEP_MUTEX_LOCK()
 * from being called at interrupt level, must disable interrupts.
 */
/****************************************************************
 *
 *  We could use native locks to advantage, but the UDI 
 *  code doesn't let us preserve/restore the interrupt
 *  enable/disable state.
 *
 *  I save the state in a temporary on entry, then after 
 *  acquiring the lock, we save the state in a location
 *  attached to the lock. 
 *
 *  When we unlock, the saved state needs to be saved in
 *  a temporary, then we unlock, then we set the interrupt
 *  enable/disable state.
 *
 *  NOTA BENE:  If you don't think you need the temporary 
 *  value, then you shouldn't be editing here.
 *
 ****************************************************************/
#define DEBUG_MUTEXES 1

void _udi_osdep_mutex_lock(_OSDEP_MUTEX_T *mutexp, char *filename, unsigned int lineno)
{
    unsigned long flags;
    unsigned long frozen = 0x40000000;
    unsigned int cpuid = smp_processor_id();
    if (_udi_osdep_mutex_debug)
        _OSDEP_PRINTF("MUTEX_LOCK(%p) by %p\n", mutexp, __builtin_return_address(0));
    local_irq_save(flags);
#ifdef __SMP__
    while (!spin_trylock(&(mutexp->lock))) {
#if DEBUG_MUTEXES
	if (!--frozen) {
#if 0
		printk("UDI mutex stuck on CPU%d:%p: ",
			cpuid, __builtin_return_address(0));
		printk("mutex{[%p:%s],[%s:%d]} ",
			mutexp, mutexp->lockname,
			mutexp->filename, mutexp->lineno);
		printk(" owned by CPU%d:%p:%s:%d\n",
			mutexp->locked_by_cpu, (void*)mutexp->locked_by_eip,
			mutexp->locked_by_filename, mutexp->locked_by_lineno);
#else
		printk("UDI mutex stuck on CPU%d:%p: "
			"mutex{[%p:%s],[%s:%d]} "
			" owned by CPU%d:%p:%s:%d\n",
			cpuid, __builtin_return_address(0),
			mutexp, mutexp->lockname,
			mutexp->filename, mutexp->lineno,
			mutexp->locked_by_cpu, (void*)mutexp->locked_by_eip,
			mutexp->locked_by_filename, mutexp->locked_by_lineno);
#endif
		frozen = 0x40000000;
	}
#endif
    }
#else /* __SMP__ */
    /* spin_trylock has no defined behavior for non-SMP kernels */
    spin_lock(&(mutexp->lock));
    (void)frozen; /* make it look used. */
#endif /* __SMP__ */
    mutexp->spinflags = flags;
#if DEBUG_MUTEXES
    mutexp->locked_by_cpu = cpuid;
    mutexp->locked_by_eip = (unsigned long)__builtin_return_address(0);
    mutexp->locked_by_filename = filename;
    mutexp->locked_by_lineno = lineno;
#endif
}


/*
 * _OSDEP_MUTEX_UNLOCK() -- Unlock an _OSDEP_MUTEX_T
 *
 * The lock must have been previously locked by _OSDEP_MUTEX_LOCK().
 * If _OSDEP_MUTEX_LOCK() disabled interrupts, this must re-enable them.
 */
void _udi_osdep_mutex_unlock(_OSDEP_MUTEX_T *mutexp)
{
    unsigned long flags = mutexp->spinflags;
    if (_udi_osdep_mutex_debug)
        _OSDEP_PRINTF("MUTEX_UNLOCK(%p) by %p\n", mutexp, __builtin_return_address(0));
#if DEBUG_MUTEXES
    if (mutexp->locked_by_eip == 0) {
	_OSDEP_PRINTF("MUTEX_UNLOCK(%p) was already unlocked!\n", mutexp);
    }
    mutexp->locked_by_cpu = mutexp->locked_by_eip = 0;
    mutexp->locked_by_filename = NULL;
    mutexp->locked_by_lineno = 0;
#endif
    spin_unlock(&(mutexp->lock));
    local_irq_restore(flags);
}

/* *************************************************************** */

/* stdarg.h thing */
#include <stdarg.h>
#ifndef __va_arg_type_violation
void __va_arg_type_violation(void);
void __va_arg_type_violation(void)
{
    volatile int x = 0;
    ASSERT(x && "udi_MA.__va_arg_type_violation: please pass values > 4 bytes large!");
dontReturn:
    goto dontReturn;
}
#endif


udi_ubit32_t udi_strtou32(const char *s, char **endptr, int base)
{
    /*
     * Swallow leading whitespaces
     */
    while (*s && (*s == ' ' || *s == '\t')) s++;
    return ((udi_ubit32_t) simple_strtoul(s, endptr, base));
}



/*
 * Function to fill in udi_limit_t struct for region creation.
 */
void _udi_get_limits_t(udi_limits_t * limits)
{
    /* These could ultimately be tied into the KMA */
    limits->max_legal_alloc = LINUX_MAX_LEGAL_ALLOC;
    limits->max_safe_alloc = UDI_MIN_ALLOC_LIMIT;
    limits->max_trace_log_formatted_len = _OSDEP_TRLOG_LIMIT;
    limits->max_instance_attr_len = UDI_MIN_INSTANCE_ATTR_LIMIT;
    limits->min_curtime_res = 1000000000 / HZ;
    limits->min_timer_res = limits->min_curtime_res;
}

extern _OSDEP_SEM_T _udi_MA_init_complete;

void EnterUDIContext() {
    long flags;
    /* Modify signal mask so we don't freeze up if they send a signal. */
    spin_lock_irqsave(&current->sigmask_lock, flags);
    siginitsetinv(&current->blocked, 0); /* disable all signals */
/*sigmask(SIGINT) | sigmask(SIGKILL) | sigmask(SIGSTOP));*/
    recalc_sigpending(current);
    spin_unlock_irqrestore(&current->sigmask_lock, flags);
    _OSDEP_SEM_P(&_udi_MA_init_complete);
}

void ExitUDIContext() {
    _OSDEP_SEM_V(&_udi_MA_init_complete);
    if (signal_pending(current)) {
        int sig;
        long flags;
        siginfo_t info;
        spin_lock_irqsave(&current->sigmask_lock, flags);
        sig = dequeue_signal(&current->blocked, &info);
        spin_unlock_irqrestore(&current->sigmask_lock, flags);
        switch (sig) {
            case SIGKILL:
               printk("Hey, don't SIGKILL an insmod\n");
               break;
            case SIGINT:
               printk("Hey, don't SIGINT an insmod\n");
               break;
            case SIGSTOP:
               printk("Hey, don't SIGSTOP an insmod\n");
               break;
            default:
               printk("Hey, don't confuse me with signal %d\n", sig);
               break;
        }
    }
}

/*
 * Returns TRUE if the given .udiprops section represents a 
 * metalanguage library.
 */
static udi_boolean_t 
_udi_module_is_meta(char *sprops_text, int sprops_size, char **provides)
{
	char *sprops;
	int idx = 0;
	int i;
	
	sprops = sprops_text;
	for (idx = 0; idx < sprops_size; i++) {
		if (udi_strncmp(&sprops[idx], "provides ", 9) == 0) {
			*provides = &sprops[idx+9];
			return TRUE;
		}
		idx = idx + strlen(&sprops[idx]) + 1;
	}
	*provides = NULL;
	return FALSE;

}

/*
 * External entry point called by glue code to register a driver with the UDI 
 * environment.
 *   initp - pointer to this module's udi_{init,meta}_info.
 *   sprops_text - pointer to ascii text image of udiprops.txt
 *   sprops_length - length of above.
 * Returns a handle that should be passed back to _udi_module_unload()
 * to deregister that module.   Only _udi_module_load and _udi_module_unload
 * know the contents of that handle is a _udi_module_data_t *.
 */

typedef struct {
        const char *md_name;
        void * md_meta;     
} _udi_module_data_t;

void *
_udi_module_load(void *initp, char *sprops_text, int sprops_length, 
		 struct module *mod)
{
	_OSDEP_DRIVER_T driver_info;
	udi_boolean_t is_meta;
	_udi_driver_t *driver;
	const char *myname;
	char *provided_meta;
	_udi_module_data_t *md;

	EnterUDIContext();
#ifdef DEBUG
    	_OSDEP_PRINTF("_udi_module_load(init:%p,sprops:%p(%d))\n",
		      initp, sprops_text, sprops_length);
#endif

	driver_info.sprops = NULL;
	driver_info.mod = mod;

	md = _OSDEP_MEM_ALLOC(sizeof(*md), 0, UDI_WAITOK);
	
	driver_info.sprops = udi_std_sprops_parse(sprops_text, 
						  sprops_length, NULL);
	is_meta = _udi_module_is_meta(driver_info.sprops->props_text,
				      driver_info.sprops->props_len,
				      &provided_meta) ;
	driver_info.sprops->is_meta = is_meta;
	myname = _udi_get_shortname_from_sprops(driver_info.sprops);
	md->md_name = myname;

	if (is_meta) {
		char provided_metab[512];
		char *s = provided_meta;
		char *d = provided_metab;


		/* 
		 * Copy the 'provides' name from the sprops to a local
		 * buffer.   This is  temporary until we can add an
		 * 'sprops_get_provides...' facility.
		 */
		while (*s != ' ') {
			*d++ = *s++;
		}
		*d = '\0';

		md->md_meta = _udi_MA_meta_load(provided_metab, initp, 
						driver_info);
		ExitUDIContext();
		return md;
	}

	driver = _udi_MA_driver_load(myname, initp, driver_info);
	if (driver != NULL) {
		int r;
		r = _udi_MA_install_driver(driver);
		if (r != UDI_OK) {
#ifdef DEBUG
			_OSDEP_PRINTF("Driver load failed: reason: %d\n", r);
#endif
			ExitUDIContext();
			return NULL;
		}
	}
	ExitUDIContext();
	return md;
}

/*
 * Given a handle returned by an earlier _udi_module_load, unbind and
 * destroy that module regardless of type.
 * 
 * Called by the kernel upon unload of a module.
 */

int
_udi_module_unload(void *mod)
{
	_udi_module_data_t *md = (_udi_module_data_t *) mod;

	EnterUDIContext();

	if (md == NULL) {
		return 0;
	}

	if (md->md_name) {
		_udi_MA_destroy_driver_by_name(md->md_name);
	}

	if (md->md_meta) {
		_udi_MA_meta_unload(md->md_name);
	}
	_OSDEP_MEM_FREE(md);
	ExitUDIContext();
	return 0;
}

char *_udi_msgfile_search(const char *filename, udi_ubit32_t msgnum,
			  const char *locale)
{
#ifdef DEBUG
    _OSDEP_PRINTF("_udi_msgfile_search(%s, %d, %s)\n", filename, msgnum,
		  locale);
#endif
    return NULL;
}

#if 0
/* Use this so we dont have to use lseek to modify the f_pos. */
int sys_read_offset(int fd, char *buf, int buf_len, loff_t * offset)
{
    int retval;
    struct file *file;

    if (fd < 0) {
	return retval = fd;
    }
    lock_kernel();
    file = (struct file *) fget(fd);	/* must be balanced with fput() */
    retval = -EACCES;
    if (file && file->f_dentry && file->f_op && file->f_op->read) {
	retval = file->f_op->read(file, buf, buf_len, offset);
    }
    fput(file);

  out:
    unlock_kernel();
    return retval;
}
#endif

void _udi_readfile_getdata(const char *filename, udi_size_t offset,
			   char *buf, udi_size_t buf_len,
			   udi_size_t * act_len)
{
#if DEBUG
    _OSDEP_PRINTF("NOT IMPLEMENTED: _udi_readfile_getdata(%s,_)\n", filename);
#endif
#if 0
/*
Fix this: the kernel exports sys_open/sys_read... but
          insmod refuses to pick it up at load time.
Until this is enabled, anything in udiprops.txt is not seen
by the environment.
*/

/* Pretty similar to how the UnixWare port works */
    int fd;
    int res;

    /* Open up the readable file */
    if ((fd = _sys_open(filename, O_RDONLY, O_RDONLY)) < 0) {
	_OSDEP_PRINTF("Unable to open readable file: '%s'  err: %d\n",
		      filename, fd);
	return;
    }

    _sys_lseek(fd, (loff_t *) & offset, 0);
    if ((res = _sys_read(fd, buf, buf_len)) < 0) {
	_OSDEP_PRINTF("Unable to read readable file: '%s'  err: %d\n",
		      filename, res);
	sys_close(fd);
	*act_len = 0;
	return;
    }

    _sys_close(fd);

    *act_len = buf_len - res;
    return;
#endif
}

/* 
 * Kernel memory allocator wrappers
 */
#ifndef LOW_OVERHEAD_MALLOC
/*
 * A header that is prepended to all allocated requests.
 * We stash some info in this header.   It could later be used to
 * store debugging info such as the address of the allocation caller.
 */
/*
 * TBD: Enhancements that would be cool are:
 *      a checksumed header.
 *      read and write protected header and footer.
 */

/*
 * Header and Footer are one page each.
 */
#define HEADER_PAD_SIZE (PAGE_SIZE / sizeof (unsigned long)) 

#if defined(INSTRUMENT_MALLOC) && !defined(HEADER_PAD_SIZE)
#define HEADER_PAD_SIZE (PAGE_SIZE / sizeof (unsigned long))
#endif

#if defined(INSTRUMENT_MALLOC) && !defined(FOOTER_PAD_SIZE)
#define FOOTER_PAD_SIZE (PAGE_SIZE / sizeof (unsigned long))
#endif
typedef union {

/*
 *  The header fields that carry information and the padding are overlaid
 *  to give one page of header data.
 */
    struct {	
#ifdef INSTRUMENT_MALLOC
      udi_queue_t mblk_qlink;
      unsigned int allocator_linenum;
      char *allocator_fname;
#endif
      udi_size_t size;
      void *phys_mem;
      int flags;
      int sleep_ok;
      unsigned long PAD[0];
   } c;
#ifdef HEADER_PAD_SIZE
    unsigned long header_stamp[HEADER_PAD_SIZE]; 
    /* For bounds checking */
#endif
} _mblk_header_t;

typedef struct {
#ifdef FOOTER_PAD_SIZE
    unsigned long footer_stamp[FOOTER_PAD_SIZE / sizeof(unsigned long)]; 
    /* For bounds checking */
#endif
} _mblk_footer_t;

#define	_UDI_HEAD_VALID_STAMP	0xDEADBEEFUL
#define	_UDI_FOOT_VALID_STAMP	0xDEADCAFEUL
#define	_UDI_HEAD_INVALID_STAMP	0xDEADBABEUL
#define	_UDI_FOOT_INVALID_STAMP	0xDEADF00DUL

#ifdef INSTRUMENT_MALLOC
extern _OSDEP_SIMPLE_MUTEX_T _udi_allocation_log_lock;
extern udi_queue_t _udi_allocation_log;
STATIC void _udi_dump_mempool(void);
STATIC udi_boolean_t _udi_validate_mblk(_mblk_header_t *mblk);
#endif

#ifdef __powerpc__
/*
 * For some reason, using vmalloc on LinuxPPC causes freezes.
 * It possibly has stronger memory protection in regards to how
 * we did a exit_mm(); in the initialization of the kthread.
 */
#define USE_VMALLOC 0
#else
#define USE_VMALLOC 1
#endif

#define HOLD_MEMORY 0

#if HOLD_MEMORY
 #define generic_virt_to_phys(ADDRESS,PROCESS) \
   ((char*)( \
	pte_page( \
		*pte_offset( \
			pmd_offset( \
				pgd_offset(PROCESS->mm,(unsigned long)ADDRESS), \
					(unsigned long) ADDRESS \
			),(unsigned long) ADDRESS \
		) \
	) | ( ((unsigned long) ADDRESS) & (~PAGE_MASK)) ))

/* Page table translation functions */
static unsigned long uvirt_to_kva(pgd_t *pgd, unsigned long user_vaddr);
static unsigned long kvirt_to_pa(unsigned long user_vaddr);
#if UNUSED
static unsigned long uvirt_to_bus(struct task_struct *task,
						unsigned long user_vaddr);
static unsigned long kvirt_to_bus(unsigned long user_vaddr);
#endif

/* Page table bit modifying functions */
unsigned long _udi_osdep_hold_memory(void *vaddr, unsigned long size);
unsigned long _udi_osdep_unhold_memory(void *vaddr, unsigned long size);

static unsigned long uvirt_to_kva(pgd_t *pgd, unsigned long user_vaddr)
{
	unsigned long ret = 0UL;
	pmd_t *pmd;
	pte_t *ptep, pte;

	if (!pgd_none(*pgd)) {
		pmd = pmd_offset(pgd, user_vaddr);
		if (!pmd_none(*pmd)) {
			ptep = pte_offset(pmd, user_vaddr);
			pte = *ptep;
			if(pte_present(pte))
				ret = (pte_page(pte)|(user_vaddr&(PAGE_SIZE-1)));
		}
	}
	return ret;
}

static unsigned long kvirt_to_pa(unsigned long user_vaddr)
{
	unsigned long va, kva, ret;

	va = VMALLOC_VMADDR(user_vaddr);
	kva = uvirt_to_kva(pgd_offset_k(va), va);
	ret = __pa(kva);
	return ret;
}

#if UNUSED
static unsigned long uvirt_to_bus(struct task_struct *task,
							unsigned long user_vaddr)
{
	unsigned long kva, ret;

	kva = uvirt_to_kva(pgd_offset(task->mm, user_vaddr), user_vaddr);
	ret = virt_to_bus((void *)kva);
	return ret;
}

static unsigned long kvirt_to_bus(unsigned long user_vaddr)
{
	unsigned long va, kva, ret;

	va = VMALLOC_VMADDR(user_vaddr);
	kva = uvirt_to_kva(pgd_offset_k(va), va);
	ret = virt_to_bus((void *)kva);
	return ret;
}
#endif

unsigned long _udi_osdep_hold_memory(void *vaddr, unsigned long size)
{
/*
 * vaddr must be a vmalloc'd memory.
 * kmalloc'd memory can be directly modified by
 * mem_map_reserve(MAP_NR(kma_mem)).
 * Pages MUST exist when this function is called. If the memory
 * is newly created, it must be zero'd out first.
 */
	unsigned long phys_addr, cur_vaddr, num_pages;
	if (vaddr && size)
	{
		cur_vaddr = (unsigned long)vaddr;
		num_pages = 1 + (size >> PAGE_SHIFT);
		printk("hold_memory(%p, %ld) %ld pages\n", vaddr, size, num_pages);
		while (num_pages--)
		{
			/* The page might not exist yet. Write to it. */
			*(volatile unsigned long*)cur_vaddr = -1;
#if 0
			phys_addr = kvirt_to_pa(cur_vaddr);
			_OSDEP_ASSERT(phys_addr != 0);
			/* Set page-is-locked bits. */
			mem_map_reserve(MAP_NR(__va(phys_addr)));
#endif
			cur_vaddr += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
		return 1 + (size >> PAGE_SHIFT);
	}
	return 0;
}

unsigned long _udi_osdep_unhold_memory(void *vaddr, unsigned long size)
{
	unsigned long phys_addr, cur_vaddr, num_pages;

	if (vaddr && size)
	{
		cur_vaddr = (unsigned long)vaddr;
		num_pages = 1 + (size >> PAGE_SHIFT);
		printk("unhold_memory(%p, %ld) %ld pages\n", vaddr, size, num_pages);
		while (num_pages--)
		{
#if 0
			phys_addr = kvirt_to_pa(cur_vaddr);
			_OSDEP_ASSERT(phys_addr != 0);
			mem_map_unreserve(MAP_NR(__va(phys_addr)));
#endif
			cur_vaddr += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
		return 1 + (size >> PAGE_SHIFT);
	}
	return 0;
}
#endif /*HOLD_MEMORY*/

extern int _udi_force_fail;
int _udi_force_fail = 0;
#define _OSDEP_INSTRUMENT_ALLOC_FAIL (_udi_force_fail & 1)

#ifdef INSTRUMENT_MALLOC
/*
 * Write-Protect a page or restore Read-/Write Permission.
 * Only the page that address lives in is (un-)protected.
 */
static __inline void protect_page(unsigned long address, udi_boolean_t protect)
{
    pgd_t *pgd;
    pmd_t *pmd;
    pte_t *pte;

    /*	
     * This doesn't (generally) work for unmapped kmalloc'd memory.
     * However, it won't do anything nasty if we call it on kmalloc'd memory,
     * so it's safe to do it.
     */
   
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
    spin_lock(&init_mm.page_table_lock);
#endif
    /*
     * Get the page for this address.
     */
    address = address & PAGE_MASK;
#ifdef _PAGE_PROTECT_DEBUG
    _OSDEP_PRINTF("%sprotecting page %p.\n", protect ? "" : "un", 
					     (void *)address);
#endif
    pgd = pgd_offset_k(address); 
    if (pgd) {
  	pmd = pmd_offset(pgd, address);
	if (pmd) {
		pte = pte_offset(pmd, address);
		if (pte_present(*pte)) {
			if (protect) {
				set_pte(pte, pte_wrprotect(*pte));
			} else {
				set_pte(pte, pte_mkwrite(*pte));
			}
		} else {
#ifdef _PAGE_PROTECT_DEBUG
	  	  _OSDEP_PRINTF("no pte for address %p.\n", (void*)address);
#endif
		}
	} else {
#ifdef _PAGE_PROTECT_DEBUG
	  _OSDEP_PRINTF("no pmd for address %p.\n", (void*)address);
#endif
	}
   } else {
#ifdef _PAGE_PROTECT_DEBUG
     _OSDEP_PRINTF("no pgd for address %p.\n", (void*)address);
#endif
   }
   /*
    * Flush the CPU's TLB cache, so it knows about the page bit updates.
    */ 
   __flush_tlb_one(address);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
   spin_unlock(&init_mm.page_table_lock);
#endif
}
#endif

#ifdef INSTRUMENT_MALLOC
void *_udi_wrapped_alloc_inst(udi_size_t size, int flags, int sleep_ok,
			 int line, char *fname)
#else
void *_udi_wrapped_alloc(udi_size_t size, int flags, int sleep_ok)
#endif
{
#if defined(HEADER_PAD_SIZE) || defined(FOOTER_PAD_SIZE)
    int loop;
#endif
    _mblk_header_t *mem; /* Actual requested block, including header */
    udi_boolean_t physical;
#ifdef INSTRUMENT_MALLOC
    _mblk_footer_t *foot;	/* footer pointer */
    _mblk_header_t *new_mem;
#endif
    char *dbuf;			/* Beginning of user-visible data buf */
    unsigned long total_size;   /* whole block size */

    if ((_OSDEP_INSTRUMENT_ALLOC_FAIL) && (sleep_ok == UDI_NOWAIT)) {
        return 0;
    }

#if defined(INSTRUMENT_MALLOC) && (defined(HEADER_PAD_SIZE) || \
				   defined(FOOTER_PAD_SIZE))
    /*
     * Align header and footer on page boundaries
     */
    size = PAGE_ALIGN(size);
#endif

    total_size = sizeof(_mblk_header_t) + size + sizeof(_mblk_footer_t);
#if USE_VMALLOC
    physical = !(sleep_ok && (total_size > PAGE_SIZE));
#else
    physical = TRUE;
#endif	
#if USE_VMALLOC
    if (!physical) {
	/* vmalloc'd memory is page aligned at the top and bottom. */
	unsigned long container_size;
	container_size = total_size | (PAGE_SIZE-1);
    	mem = vmalloc(total_size);
#ifdef INSTRUMENT_MALLOC
	no_vmallocs++;
#endif
	/*
	 * If we start running out of memory before filling up
	 * memory, we might need to change the memory address
	 * space that the UDI daemons belong to. It would be very cool
	 * to just track memory on a per-driver basis by making
	 * each driver have its own thread.
	 */
	_OSDEP_ASSERT((mem != NULL) && "vmalloc returned NULL");
#if HOLD_MEMORY
	/* round up to the nearest page end. */
	if ((container_size >> PAGE_SHIFT) != (total_size >> PAGE_SHIFT)) {
		printk("size = %d total_size = %ld rounded = %ld\n", size, total_size, container_size);
		_OSDEP_ASSERT((container_size >> PAGE_SHIFT) == (total_size >> PAGE_SHIFT));
	}
	udi_memset(mem, 0, container_size);
	_udi_osdep_hold_memory(mem, total_size);
#endif
    }
    else
#endif
    {
	if (sleep_ok) {
		mem = kmalloc(total_size, GFP_KERNEL);
#ifdef INSTRUMENT_MALLOC
		no_kmallocs++;
#endif
		_OSDEP_ASSERT((mem != NULL) && "kmalloc(GFP_KERNEL) returned NULL.");
	}
	else {
		mem = kmalloc(total_size, GFP_ATOMIC);
#ifdef INSTRUMENT_MALLOC
		no_kmallocs++;
#endif
	}
    }
    if (!mem) {
	return 0;
    }
#ifdef INSTRUMENT_MALLOC
    if (physical) {
	/*
	 * Play dirty. Let's remap the physical pages to a virtual
	 * memory area so that we can protect them. Since ioremap won't
	 * remap physical RAM, we need to trick it into thinking the pages
	 * aren't really there. We do this by marking the associated 
	 * struct page's as "reserved". Of course, this extra mapping has to
	 * be freed prior to releasing the memory, and we also have to undo
	 * the "reservation" in udi_wrapped_free(_inst).
	 */
	struct page *pg;
	char *end = (char*)mem + PAGE_ALIGN(total_size) - 1;
	for (pg = virt_to_page(mem); pg <= virt_to_page(end); pg ++) {
		set_bit(PG_reserved, &pg->flags); 
	};
	new_mem = ioremap(__pa(mem), total_size);
	if (!new_mem) {
#ifdef _PROTECT_PAGES_DEBUG
		_OSDEP_PRINTF("remapping of physical pages to virtual area "
			      "failed for memory at %p.\n", mem);
#endif
		new_mem = mem; 	/* try to resume gracefully */

		/*
		 * NOTE: ioremap can fail above because it will try to allocate
		 * the memory for the page tables using GFP_KERNEL priority.
		 * This isn't usually a problem, because the kernel will only
		 * block if it can (i.e. it won't try to block during an
		 * interrupt), but if it can't, the allocation can fail. It's
		 * not a very likely case, but it can happen, so we need to
		 * handle it gracefully. Also note that if the mapping fails,
		 * we can't write-protect the pages.
		 */
	}	
	new_mem->c.phys_mem = mem;
   	mem = new_mem;
     }
#endif
    
    dbuf = ((char *)mem) + sizeof(_mblk_header_t);
#ifdef INSTRUMENT_MALLOC
    foot = (_mblk_footer_t*)(dbuf + size);
#endif
#ifdef HEADER_PAD_SIZE
    for (loop = sizeof(mem->c) / sizeof(unsigned long); loop < HEADER_PAD_SIZE;
	 loop++) {
        mem->header_stamp[loop] = _UDI_HEAD_VALID_STAMP;
    }
#endif
    mem->c.size = size;
    mem->c.flags = flags;
    mem->c.sleep_ok = sleep_ok;
#ifdef DEBUG
    if (flags & UDI_MEM_MOVABLE) {
	_OSDEP_PRINTF(__FUNCTION__ ": ignoring UDI_MEM_MOVABLE flag.\n");
    }
#endif


    if (!(flags & UDI_MEM_NOZERO)) {
	udi_memset(dbuf, 0x00, size);
    }
#if 1 /*DEBUG*/
    else {
        /* Spray memory with uninitialized data constant. */
	/*udi_memset(dbuf, 0xC1, size);*/
	udi_memset(dbuf, 0x00, size);
    }
#endif

#ifdef INSTRUMENT_MALLOC
    /* Fill in the rest of the header */
    mem->c.allocator_linenum = line;
    mem->c.allocator_fname = fname;

    /* Fill in the footer */
#ifdef FOOTER_PAD_SIZE
    for (loop = 0; loop < FOOTER_PAD_SIZE / sizeof(unsigned long); loop++) {
	foot->footer_stamp[loop] = _UDI_FOOT_VALID_STAMP;
    }
#endif
#ifdef HEADER_PAD_SIZE 
    {
	/*
	 * Before we hang in the new allocation, we have to unprotect the
	 * page of the allocation that is before us, because the link member
	 * is in the protected header. After we hang ourselves into the queue,
	 * that page is again protected.
	 */
	udi_queue_t *prev = NULL;
  	_OSDEP_SIMPLE_MUTEX_LOCK(&_udi_allocation_log_lock);
	if (!UDI_QUEUE_EMPTY(&_udi_allocation_log)) 
	   prev = UDI_LAST_ELEMENT(&_udi_allocation_log);
	if (prev) {
	  protect_page((unsigned long)prev, FALSE);
	}
	UDI_ENQUEUE_TAIL(&_udi_allocation_log, &mem->c.mblk_qlink);
	if (prev) {
	  protect_page((unsigned long)prev, TRUE);
	}
   }
#else
   _OSDEP_SIMPLE_MUTEX_LOCK(&_udi_allocation_log_lock);
   UDI_ENQUEUE_TAIL(&_udi_allocation_log, &mem->c.mblk_qlink);
#endif
   _OSDEP_SIMPLE_MUTEX_UNLOCK(&_udi_allocation_log_lock);
/*
 * Write-protect the header and the footer.
 */
#ifdef HEADER_PAD_SIZE
    protect_page((unsigned long)mem, TRUE);
#endif
#ifdef FOOTER_PAD_SIZE
    protect_page((unsigned long)foot, TRUE);
#endif
#endif
    return dbuf;
}

#ifdef INSTRUMENT_MALLOC
void _udi_wrapped_free_inst(void *buf, int line, char *fname)
#else
void _udi_wrapped_free(void *buf)
#endif
{
    /*
     * Walk backward in our buffer to find the header.  Use that
     * info for kmem_free.
     */
    unsigned long total_size;
    char *dbuf;			/* Beginning of user-visible data buf */
#if defined(HEADER_PAD_SIZE) || defined(FOOTER_PAD_SIZE)
    unsigned long loop;
#endif
#ifdef INSTRUMENT_MALLOC
    _mblk_footer_t *foot;	/* footer pointer */
    char *phys_mem, *end;
    struct page *pg;
   
#endif
    int fail = 0;
    _mblk_header_t *mem = (_mblk_header_t *) ((char *) buf -
					      sizeof(_mblk_header_t));

    dbuf = (char *) ((char *) mem + sizeof(_mblk_header_t));

#ifdef INSTRUMENT_MALLOC
    if (_udi_validate_mblk(mem)) {
        fail |= 16;
    }
#endif

#ifdef HEADER_PAD_SIZE
    if (mem->header_stamp[sizeof(mem->c) / sizeof(unsigned long)] == 
	_UDI_HEAD_INVALID_STAMP) {
	_OSDEP_PRINTF(__FUNCTION__": header: Don't double free memory blocks!\n");
	fail |= 1;
    } else if (mem->header_stamp[sizeof(mem->c) / sizeof(unsigned long)] != 
	_UDI_HEAD_VALID_STAMP) {
	_OSDEP_PRINTF(__FUNCTION__": header: Corrupt memory block:"
		" expected hstamp %lx, got %lx\n",
		_UDI_HEAD_VALID_STAMP, mem->header_stamp[sizeof(mem->c) / 
						       sizeof (unsigned long)]);
	fail |= 2;
    }
#endif

#if defined(INSTRUMENT_MALLOC) && defined(FOOTER_PAD_SIZE)
    foot = (_mblk_footer_t*)(dbuf + mem->c.size);
    if (foot->footer_stamp[0] == _UDI_FOOT_INVALID_STAMP) {
	_OSDEP_PRINTF(__FUNCTION__": footer: Don't double free memory blocks!\n");
	fail |= 4;
    } else if (foot->footer_stamp[0] != _UDI_FOOT_VALID_STAMP) {
	_OSDEP_PRINTF(__FUNCTION__": footer: Corrupt memory block:"
		" expected stamp %lx, got %lx\n",
		_UDI_FOOT_VALID_STAMP, foot->footer_stamp[0]);
	fail |= 8;
    }
#endif

    if (fail) {
#ifdef INSTRUMENT_MALLOC
	_OSDEP_PRINTF("Failed block allocated from %s:%d.\n", mem->c.allocator_fname, 
			mem->c.allocator_linenum);
#endif
	_OSDEP_PRINTF("Failed block size: 0x%lx  flags:0x%x fail code:0x%x\n",
			(unsigned long)mem->c.size, mem->c.flags, fail);
#if 0 /* Very verbose */
	_udi_dump_mempool();

#ifdef INSTRUMENT_MALLOC
	_OSDEP_PRINTF("Failed block allocated from %s:%d.\n", mem->c.allocator_fname, 
			mem->c.allocator_linenum);
#endif
	_OSDEP_PRINTF("Failed block size: 0x%lx  flags:0x%x fail code:0x%x\n",
			(unsigned long)mem->c.size, mem->c.flags, fail);
#endif /* Very verbose */
	ASSERT(fail == 0);
    }
/*
 * Unprotect the header and the footer.
 */
#ifdef INSTRUMENT_MALLOC
#ifdef FOOTER_PAD_SIZE
    protect_page((unsigned long)foot, FALSE);
#endif
#ifdef FOOTER_PAD_SIZE
    protect_page((unsigned long)mem, FALSE);
#endif
#endif
    
#ifdef HEADER_PAD_SIZE
    for (loop = sizeof(mem->c) / sizeof(unsigned long); loop < HEADER_PAD_SIZE; 
	 loop++) {
    	mem->header_stamp[loop] = _UDI_HEAD_INVALID_STAMP;
    }
#endif
#ifdef INSTRUMENT_MALLOC
#ifdef HEADER_PAD_SIZE 
    {
	/*
 	 * Remove this allocation from the allocations queue.
  	 * Since the next and/or previous allocation in the queue will be
	 * write-protected, these must be unprotected first, then we can
	 * dequeue, and then we re-protect those pages.
	 */
	udi_queue_t* prev = NULL, *next = NULL;
        _OSDEP_SIMPLE_MUTEX_LOCK(&_udi_allocation_log_lock);
        if (UDI_LAST_ELEMENT(&_udi_allocation_log) != &mem->c.mblk_qlink) {
	   next = UDI_NEXT_ELEMENT(&mem->c.mblk_qlink);
	}
    	if (UDI_FIRST_ELEMENT(&_udi_allocation_log) != &mem->c.mblk_qlink) {
	   prev = UDI_PREV_ELEMENT(&mem->c.mblk_qlink);
	}
	if (next) { 
		protect_page((unsigned long)next, FALSE);
	}
	if (prev) {  
		protect_page((unsigned long)prev, FALSE);
	}
        UDI_QUEUE_REMOVE(&mem->c.mblk_qlink);
	if (next) {  
		protect_page((unsigned long)next, TRUE);
	}
	if (prev) {  
		protect_page((unsigned long)prev, TRUE);
	}
    }
#else
     _OSDEP_SIMPLE_MUTEX_LOCK(&_udi_allocation_log_lock);
     UDI_QUEUE_REMOVE(&mem->c.mblk_qlink);
#endif
#ifdef FOOTER_PAD_SIZE
    /* We're now safe to make the block look invalid. */
    _OSDEP_SIMPLE_MUTEX_UNLOCK(&_udi_allocation_log_lock);
    for (loop = 0; loop < FOOTER_PAD_SIZE / sizeof(unsigned long); loop++) {
       foot->footer_stamp[loop] = _UDI_FOOT_INVALID_STAMP;
    }
    udi_memset(dbuf, 0xCD, mem->c.size);
#endif
#endif

    total_size = sizeof(_mblk_header_t) + mem->c.size + sizeof(_mblk_footer_t);
#if USE_VMALLOC
    if (mem->c.sleep_ok && (total_size > PAGE_SIZE)) {
#if HOLD_MEMORY
	_udi_osdep_unhold_memory(mem, total_size);
#endif
    	vfree(mem);
#ifdef INSTRUMENT_MALLOC
	no_vfrees++;
#endif
    }
    else
#endif
    {
#ifdef INSTRUMENT_MALLOC
	/*
	 * Release the virtual mapping for this kmalloc'd memory.
	 * This includes calling vfree on the mapping (the physical pages
	 * won't be freed because they are marked as "reserved"), then
	 * unmarking the pages, then kfreeing the memory. 
	 */
	phys_mem = mem->c.phys_mem;
	if ((void *)mem != phys_mem) {
	  /* 
	   * If this isn't true, the phys->virt remapping failed.
	   * Thus, only do the extra magic below if we actually have
	   * a valid mapping.
	   */
	  vfree(mem);
	  end = (char *)phys_mem + PAGE_ALIGN(total_size) - 1;
	  for (pg = virt_to_page(phys_mem); pg <= virt_to_page(end); pg++) {
		clear_bit(PG_reserved, &virt_to_page(__pa(pg))->flags);
  	  }
	}
    	kfree(phys_mem);
	no_kfrees++;
#else
	kfree(mem);
#endif
    }
}

#ifdef INSTRUMENT_MALLOC
/* TRUE for invalid/corrupt memory blocks. */
STATIC udi_boolean_t _udi_validate_mblk(_mblk_header_t *mblk)
{
	unsigned long loop, error_count = 0;
#ifdef FOOTER_PAD_SIZE
	_mblk_footer_t *foot;
	char *dbuf;
#endif
#ifdef HEADER_PAD_SIZE
        /* Validate the header. */
        for (loop = sizeof(mblk->c) / sizeof(unsigned long); 
	     loop < HEADER_PAD_SIZE; loop++) {
		if (mblk->header_stamp[loop] != _UDI_HEAD_VALID_STAMP) {
			_OSDEP_PRINTF("UDI allocated memory block header invalid at %p. Wanted 0x%08lX, got %08lX\n",
				&mblk->header_stamp[loop],
				_UDI_HEAD_VALID_STAMP,
				mblk->header_stamp[loop]);
			error_count++;
		}
	}
#endif
#ifdef FOOTER_PAD_SIZE
        /* Validate the footer. */
	dbuf = ((char*)mblk) + sizeof(_mblk_header_t);
        foot = (_mblk_footer_t*)(dbuf + mblk->c.size);
        for (loop = 0; loop < FOOTER_PAD_SIZE / sizeof(unsigned long); loop++) {
		unsigned long fstampval = _UDI_FOOT_VALID_STAMP;
		if (memcmp(&foot->footer_stamp[loop], &fstampval,
						sizeof(fstampval)) != 0) {
			memcpy(&fstampval, &foot->footer_stamp[loop],
							sizeof(fstampval));
			_OSDEP_PRINTF("UDI allocated memory block footer invalid at %p. Wanted 0x%08lX, got %08lX\n",
				&foot->footer_stamp[loop],
				_UDI_FOOT_VALID_STAMP,
				fstampval);
			error_count++;
		}
	}
#endif
    	if (error_count)
		_OSDEP_PRINTF("mblk INVALID: Allocated from %s:%d."
				" size is %ld(0x%lx)/%p\n",
		     mblk->c.allocator_fname,
		     mblk->c.allocator_linenum,
		     (unsigned long)mblk->c.size,
		     (unsigned long)mblk->c.size,
		     (char *) mblk + sizeof(_mblk_header_t));
	return error_count;
}

/* returns TRUE if it was a valid memory pointer. */
udi_boolean_t _udi_find_in_mempool(void *ptr)
{
    /* walk the allocated memory list, searching for some block. */
    udi_queue_t *listp = &_udi_allocation_log;
    udi_queue_t *e, *t;

    UDI_QUEUE_FOREACH(listp, e, t) {
	_mblk_header_t *mblk = (_mblk_header_t *) e;
	char *dataptr = (char*)mblk + sizeof(_mblk_header_t);
	if ((void*)ptr >= (void*)dataptr) {
		char *endofmblk = (char*)mblk + sizeof(_mblk_header_t) + mblk->c.size;
	 	if ((void*)ptr < (void*)endofmblk) {
		_OSDEP_PRINTF("%p = %p+(%ld)0x%lx: allocated from %s:%d,"
			" size %ld(0x%lx)\n",
		     ptr,
		     dataptr,
		     (unsigned long)((void*)ptr - (void*)dataptr),
		     (unsigned long)((void*)ptr - (void*)dataptr),
		     mblk->c.allocator_fname,
		     mblk->c.allocator_linenum,
		     (unsigned long)mblk->c.size,
		     (unsigned long)mblk->c.size);
		return TRUE;
		}
	}
    }
    _OSDEP_PRINTF("%p: is not from the UDI memory pool.\n", ptr);
    return FALSE;
}

STATIC void _udi_check_mempool(void)
{
    /* walk the allocated memory list, validating the header/footer. */
    udi_queue_t *listp = &_udi_allocation_log;
    udi_queue_t *e, *t;

    UDI_QUEUE_FOREACH(listp, e, t) {
	_mblk_header_t *mblk = (_mblk_header_t *) e;
        _udi_validate_mblk(mblk);
    }
}

STATIC void _udi_dump_mempool(void)
{
    udi_queue_t *listp = &_udi_allocation_log;
    udi_queue_t *e, *t;
    udi_size_t sz = 0;

    UDI_QUEUE_FOREACH(listp, e, t) {
	_mblk_header_t *mblk = (_mblk_header_t *) e;
	sz += mblk->c.size;
	_OSDEP_PRINTF("Allocated from %s:%d. size is %ld(0x%lx)/%p\n",
		     mblk->c.allocator_fname,
		     mblk->c.allocator_linenum,
		     (unsigned long)mblk->c.size,
		     (unsigned long)mblk->c.size,
		     (char *) mblk + sizeof(_mblk_header_t));
#if 0
	if (debug_output_aborted())
	    break;
#endif
    }
    _OSDEP_PRINTF("Total allocated memory: %ld(0x%lx) bytes\n", sz, sz);
}

#endif /*INSTRUMENT_MALLOC*/

#endif /*LOW_OVERHEAD_MALLOC*/

/*
 * osdep_pio_delay -- short delay in trans lists
 * Delays for delay microseconds.
 * PIO calls this with a signed long.
 */

void
osdep_pio_delay(long delay)
{
    /*
     * udelay may not work correctly for more than a few ms, so we call
     * it repeatedly 1ms at a time.
     */

#define _OSDEP_MAX_UDELAY	1000

    while (delay > _OSDEP_MAX_UDELAY) {
	udelay(_OSDEP_MAX_UDELAY);
	delay -= _OSDEP_MAX_UDELAY;
    }
    udelay(delay);
}

/*
 * Timer stuff
 */

/* 
 * struct containes the callback function and the control block to pass that
 * function
 * a pointer to this struct will be passed to our wrapper function 
 * osdep_timer_ind
 */

typedef struct osdep_timer_info_t {
    struct timer_list timer;
    void (*func)(void *);
    void *arg;
    unsigned long instance;
    udi_queue_t link;
} osdep_timer_info_t;

typedef struct _udi_timerd_tinfo {
	volatile int	dead;
	char		*thread_name;
	_OSDEP_SEM_T	running;
} udi_timerd_tinfo_t;

udi_timerd_tinfo_t udi_timerd_tinfo = {0, "udi_timerd", {}};

#define OSDEP_MAX_TIMERS	64

STATIC osdep_timer_info_t osdep_timers[OSDEP_MAX_TIMERS];
STATIC _OSDEP_MUTEX_T osdep_timer_lock;
STATIC int osdep_next_timer;
STATIC udi_queue_t _osdep_timer_q;
STATIC _OSDEP_EVENT_T _osdep_timer_q_ev;
STATIC udi_boolean_t _osdep_kill_timer_daemon;

extern void __udi_kthread_common_init(char*);
extern int __udi_kthread_signal_pending(); 

STATIC int
_osdep_timer_daemon(void* arg)
{
   udi_timerd_tinfo_t *tinfo = (udi_timerd_tinfo_t*)arg;
   __udi_kthread_common_init(tinfo->thread_name);
   /*
    * run at highest priority so that we don't miss timers if we can help it
    */ 
   current->policy = SCHED_FIFO;
   current->rt_priority = 99;
   current->need_resched = 1;
   _OSDEP_SEM_V(&tinfo->running);
   
   for (;;) {
	udi_queue_t *elem;
	osdep_timer_info_t* ti;
	unsigned long inst;
#ifdef TIMER_NOISY
	_OSDEP_PRINTF("timer daemon running.\n");
#endif
	_OSDEP_EVENT_WAIT(&_osdep_timer_q_ev);

	_OSDEP_MUTEX_LOCK(&osdep_timer_lock);
	while (!UDI_QUEUE_EMPTY(&_osdep_timer_q)) {
	    elem = UDI_DEQUEUE_HEAD(&_osdep_timer_q);
	    ti = UDI_BASE_STRUCT(elem, osdep_timer_info_t, link);
	    inst = ti->instance;
	    if (ti->func) {
	    	_OSDEP_MUTEX_UNLOCK(&osdep_timer_lock);
	    	ti->func(ti->arg);
	    	_OSDEP_MUTEX_LOCK(&osdep_timer_lock);
	    }
	
    	    /*
             * Function may have cancelled a repeating timer, and it even may 
             * have been reused. If so, nothing more to do.
             */

    	     if (ti->instance == inst && ti->func) {
    		/*
     		 * delete the timer so it is again available
     		 */
    		del_timer(&ti->timer);
    		ti->func = NULL;
    	     }	 
	     _OSDEP_EVENT_CLEAR(&_osdep_timer_q_ev);
	}
    	_OSDEP_MUTEX_UNLOCK(&osdep_timer_lock);

	if (__udi_kthread_signal_pending())
	    _osdep_kill_timer_daemon = TRUE;
	if (_osdep_kill_timer_daemon) {
#ifdef NOISY
	    printk("timer daemon dying\n");
#endif
            tinfo->dead = 1;
	    return 0;
	    /*
	     * NOT REACHED
	     */
	}
    }
}

int _udi_timerd_pid;

void
osdep_timer_init()
{
#ifdef NOISY
    _OSDEP_PRINTF("osdep_timer_init called\n");
#endif
    udi_memset(osdep_timers, 0x00, sizeof(osdep_timers));
    _OSDEP_SEM_INIT(&udi_timerd_tinfo.running, 0);
    _OSDEP_MUTEX_INIT(&osdep_timer_lock, "timer lock");
    osdep_next_timer = 0;
    UDI_QUEUE_INIT(&_osdep_timer_q);
    _OSDEP_EVENT_INIT(&_osdep_timer_q_ev);
    _osdep_kill_timer_daemon = FALSE;
    _udi_timerd_pid = kernel_thread(_osdep_timer_daemon, (void*)&udi_timerd_tinfo, 0/*CLONE_FS*/);
    _OSDEP_ASSERT((_udi_timerd_pid >= 0) && "udi: failed to start the timer daemon.");
    _OSDEP_SEM_P(&udi_timerd_tinfo.running);
    _OSDEP_SEM_DEINIT(&udi_timerd_tinfo.running);
}

void
osdep_timer_deinit()
{
#ifdef NOISY
    _OSDEP_PRINTF("osdep_timer_deinit called\n");
#endif
    _osdep_kill_timer_daemon = TRUE;
    _OSDEP_EVENT_SIGNAL(&_osdep_timer_q_ev);
    /* Wait for timer daemon to die */
    while (udi_timerd_tinfo.dead == FALSE) {
        schedule();
    }
    _OSDEP_EVENT_DEINIT(&_osdep_timer_q_ev);
    _OSDEP_MUTEX_DEINIT(&osdep_timer_lock);
}

/*
 * osdep_timer_ind is called when a kernel timer times out
 * its parameter is a pointer to a osdep_timer_info struct describing
 * the receipient of this indication 
 *
 * WARNING 
 * If the driver is unloaded with active timers, the system will panic when
 * the timer fires because the callback function will no longer be present.
 * Somebody (probably the driver deinit function in the common env code)
 * should kill all pending timers in case a driver that is shutting down does 
 * not do that. Or can we check for this case here and catch it?
 *
 * The above remark is no longer pertinent, as the only timer being used now-
 * adays is the heartbeat, which is always present if a udi timer is present.
 * Still, if that ever changes again, the problem will resurface, so we leave
 * the remark in here.  			(burk@stg.com 09/07/2000)
 */

STATIC void
osdep_timer_ind(unsigned long data)
{
    osdep_timer_info_t* ti;
    
#ifdef TIMER_NOISY
    _OSDEP_PRINTF("Timer %ld fired.\n", data);
#endif

    _OSDEP_ASSERT(data >= 0 && data < OSDEP_MAX_TIMERS);
    
    ti = &osdep_timers[data];

    /*
     * Queue the timer request to the timer daemon
     */
   
    _OSDEP_MUTEX_LOCK(&osdep_timer_lock);
    UDI_ENQUEUE_TAIL(&_osdep_timer_q, &ti->link);
    _OSDEP_MUTEX_UNLOCK(&osdep_timer_lock);

    /*
     * wake up the timer daemon
     */
    _OSDEP_EVENT_SIGNAL(&_osdep_timer_q_ev);
}    

/*
 * osdep_timer_start creates a timer 
 */

_OSDEP_TIMER_T osdep_timer_start(osdep_timer_callback_t timer_done, void *arg,
    _OSDEP_TICKS_T interval, int repeat)
{
    int i;
    osdep_timer_info_t *ti = NULL;

#ifdef TIMER_NOISY
    _OSDEP_PRINTF("osdep_timer_start: creating timer for %ld repeat %d\n",
	interval, repeat);
#endif

    _OSDEP_MUTEX_LOCK(&osdep_timer_lock);

#ifdef TIMER_NOISY
    _OSDEP_PRINTF("osdep_timer_start: acquired timer mutex\n");
#endif

    /*
     * Find an available timer.
     */

    for (i = osdep_next_timer; i < OSDEP_MAX_TIMERS; i++) {
	if (!osdep_timers[i].func) {
	    ti = &osdep_timers[i];
	    ti->func = timer_done;
	    ti->instance++;
	    break;
	}
    }

    if (!ti) {
	for (i = 0; i < osdep_next_timer; i++) {
	    if (!osdep_timers[i].func) {
		ti = &osdep_timers[i];
		ti->func = timer_done;
		ti->instance++;
		break;
	    }
	}
    }

    osdep_next_timer = i + 1;
    if (osdep_next_timer >= OSDEP_MAX_TIMERS) {
	osdep_next_timer = 0;
    }

    _OSDEP_MUTEX_UNLOCK(&osdep_timer_lock);

    _OSDEP_ASSERT(ti != NULL);

    ti->arg = arg;
    
#ifdef TIMER_NOISY
    _OSDEP_PRINTF("initing timer %d\n", i);
#endif
    init_timer(&ti->timer);
    ti->timer.expires = jiffies + interval;
    ti->timer.data = i;
    ti->timer.function = osdep_timer_ind;

#ifdef TIMER_NOISY
    _OSDEP_PRINTF("adding timer %d\n", i);
#endif
    add_timer(&ti->timer);

#ifdef TIMER_NOISY
    _OSDEP_PRINTF("returning timer %d\n", i);
#endif
    return i;
}

/*
 * osdep_timer_cancel cancel a pending timer
 */

void
osdep_timer_cancel(_OSDEP_TIMER_T the_timer)
{
    osdep_timer_info_t *ti = &osdep_timers[the_timer];

#ifdef NOISY
    _OSDEP_PRINTF("Canceling timer %d\n", the_timer);
#endif

    _OSDEP_ASSERT(the_timer >= 0 && the_timer < OSDEP_MAX_TIMERS);

    /* 
     * This is OK even if it's already been deleted (we may be called
     * from the timeout function).
     */

    del_timer(&ti->timer);

    _OSDEP_MUTEX_LOCK(&osdep_timer_lock);

    ti->func = NULL;

    _OSDEP_MUTEX_UNLOCK(&osdep_timer_lock);
}

void osdep_subtract_time(_OSDEP_TIMER_T op1, _OSDEP_TIMER_T op2, udi_time_t *result)
{
    result->seconds = ((op1) - (op2)) / HZ;
    result->nanoseconds = (((op1) - (op2)) % HZ ) * 1000000000 / HZ;
}

_OSDEP_TICKS_T osdep_get_jiffies()
{
    return jiffies;
}

_OSDEP_TICKS_T _osdep_time_t_to_ticks(udi_time_t i)
{
   return ((i).seconds * HZ + (i).nanoseconds * HZ / 1000000000);
}

void _udi_osdep_atomic_int_init(_OSDEP_ATOMIC_INT_T* t, int i) 
{
    udi_memset(t, 0x0, sizeof(*t));
    atomic_set(&t->val, i);
}

void _udi_osdep_atomic_int_deinit(_OSDEP_ATOMIC_INT_T* t)
{
    udi_memset(t, 0x00, sizeof(*t));
}

int _udi_osdep_atomic_int_read(_OSDEP_ATOMIC_INT_T* t)
{
    return atomic_read(&t->val);
}

void _udi_osdep_atomic_int_incr(_OSDEP_ATOMIC_INT_T* t)
{
    atomic_inc(&t->val);
}

void _udi_osdep_atomic_int_decr(_OSDEP_ATOMIC_INT_T* t)
{
    atomic_dec(&t->val);
}

udi_boolean_t
_osdep_constraints_attr_check(udi_dma_constraints_attr_t type,
	 		      udi_ubit32_t value)
{
    switch (type) {
	case UDI_DMA_ELEMENT_ALIGNMENT_BITS:
	case UDI_DMA_SCGTH_ALIGNMENT_BITS:
	    return (value <= PAGE_SHIFT);
	case UDI_DMA_ADDR_FIXED_BITS:
	    return (value < 31);
	case UDI_DMA_ADDR_FIXED_TYPE:
	    return (value != UDI_DMA_FIXED_VALUE);
	case UDI_DMA_SLOP_IN_BITS:
	case UDI_DMA_SLOP_OUT_BITS:
	case UDI_DMA_SLOP_OUT_EXTRA:
	    return (value == 0);
	case UDI_DMA_SCGTH_PREFIX_BYTES:
	    return (value == 0);
	default:
	    return TRUE;
    }
}
	    			    
inline void 
_osdep_mask_intr() 
{
    cli();
}

inline void
_osdep_unmask_intr()
{
    sti();
}

inline udi_boolean_t
_osdep_in_intr()
{
    return (udi_boolean_t)in_interrupt();
}

inline udi_boolean_t
_osdep_low_on_stack()
{
#if defined(__i386__)
	register unsigned long esp;
	unsigned long ts_end = (unsigned long)(current) + sizeof(struct
			task_struct);
	asm("mov %%esp, %0" : "=r" (esp));
	return ((esp - ts_end) < MIN_STACK_SIZE);
#elif defined (__powerpc__)
	register unsigned long sp;
	unsigned long ts_end = (unsigned long)(current) + sizeof(struct 
			task_struct);
	asm("mr %0, %%r1" : "=r" (sp));
	return ((sp - ts_end) < MIN_STACK_SIZE);
#else
	return TRUE;	/*
			 * For other architectures, this means to always defer
			 * ops. This is slow, expensive, etc., but at least it
			 * won't keep on using stack that isn't there.
			 */
#endif
}		
			
