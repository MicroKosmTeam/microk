/*
 * File: linux/udi_env_init.c
 *
 * When you load the udi_MA kernel module, this is what you're
 * asking to execute. This manages bringup and teardown of the
 * UDI environment in kernel space. It also implements some of
 * the various [udi_*d] kernel_threads you see in a `ps`.
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

#ifdef __NO_VERSION__
#undef __NO_VERSION__
#endif

#include "udi_osdep_opaquedefs.h"
#include <udi_env.h>
#include <linux/smp_lock.h>

/* Important kernel thread initialization functions */
extern void __udi_kthread_common_init(char *thread_name);
extern int __udi_kthread_signal_pending();

_OSDEP_EVENT_T _udi_ev, _udi_alloc_event;
_OSDEP_SEM_T _udi_MA_init_complete;

STATIC int _udi_kill_daemon = 0;

#ifdef INSTRUMENT_MALLOC
extern _OSDEP_SIMPLE_MUTEX_T _udi_allocation_log_lock;
_OSDEP_SIMPLE_MUTEX_T _udi_allocation_log_lock = {{0}};
extern udi_queue_t _udi_allocation_log;
udi_queue_t _udi_allocation_log = {0};
#endif

/* Define module parameters */

/* From udi_init.c */
extern udi_ubit32_t _udi_debug_level;
MODULE_PARM(_udi_debug_level, "i");

/* From udi_piotrans.c */
extern udi_ubit32_t _udi_pio_debug_level;
MODULE_PARM(_udi_pio_debug_level, "i");

/* From udi_MA.c */
extern udi_ubit32_t _udi_default_trace_mask;
MODULE_PARM(_udi_default_trace_mask, "i");

/* Define module information */
MODULE_AUTHOR("ProjectUDI's Prototype and Reference Implementation participants.");
MODULE_DESCRIPTION("UDI Reference Environment, Linux Kernel Module\n"
	      "    UDI for Linux updates:  http://www.stg.com/udi\n");


/*
 * region daemon support
 */

_OSDEP_MUTEX_T _udi_region_q_lock;
_OSDEP_EVENT_T _udi_region_q_event;
udi_queue_t _udi_region_q_head;

STATIC int _udi_env_kthread_init = 0;

typedef struct _udi_kthread_info
{
	volatile int	dead;
	char		*thread_name;
	_OSDEP_SEM_T	running;
} udi_kthread_info_t;

udi_kthread_info_t udi_allocd_tinfo = {0, "udi_allocd", {}},
		   udi_regiond_tinfo = {0, "udi_regiond", {}},
		   udi_envd_tinfo = {0, "udi_envd", {}};
/*
 * Some udi_osdep_dma.c variables we need to initialize.
 */

extern void osdep_dma_init_cache();
extern void osdep_dma_dispose_of_cache();


STATIC int
_udi_alloc_daemon(void *arg)
{
    udi_kthread_info_t *tinfo = (udi_kthread_info_t*)arg;
    __udi_kthread_common_init(tinfo->thread_name);
    _OSDEP_SEM_V(&tinfo->running);
    for (;;)
    {
#ifdef NOISY
        printk("alloc daemon running\n"); 
#endif
        while (_udi_alloc_daemon_work()) {
		;
	}
        _OSDEP_EVENT_WAIT(&_udi_alloc_event);
        if (__udi_kthread_signal_pending())
		_udi_kill_daemon = 1;
        if (_udi_kill_daemon) {
#ifdef NOISY
            printk("alloc daemon dying\n");
#endif
            tinfo->dead = TRUE;
            return 0;
        }
    }
    /* NOT REACHED */
    return 0;
}

STATIC int
 _udi_region_daemon(void* arg)
{
    udi_kthread_info_t *tinfo = (udi_kthread_info_t*)arg;
    __udi_kthread_common_init(tinfo->thread_name);
    _OSDEP_SEM_V(&tinfo->running);
  	for (;;) {
		udi_queue_t *elem;
		_udi_region_t *rgn;
#ifdef NOISY    
                printk("region daemon running\n");
#endif

		_OSDEP_EVENT_WAIT(&_udi_region_q_event);
		/*
		 * Note that we have to grab and release the mutex
		 * on the deferred region queue while in the loop because
		 * the region we're about to run may need to queue something
		 * onto the very region we're running.
		 *
		 * A potential optimization is to reduce the number of
		 * lock roundtrips in this exercise...
		 */
		_OSDEP_MUTEX_LOCK(&_udi_region_q_lock);
		while (!UDI_QUEUE_EMPTY(&_udi_region_q_head)) {
			elem = UDI_DEQUEUE_HEAD(&_udi_region_q_head);
			rgn = UDI_BASE_STRUCT(elem, _udi_region_t,
					      reg_sched_link);

			rgn->in_sched_q = FALSE;
			_OSDEP_MUTEX_UNLOCK(&_udi_region_q_lock);
			_udi_run_inactive_region(rgn);
			_OSDEP_MUTEX_LOCK(&_udi_region_q_lock);
		}
		_OSDEP_MUTEX_UNLOCK(&_udi_region_q_lock);
		if (__udi_kthread_signal_pending())
			_udi_kill_daemon = 1;
		if (_udi_kill_daemon) {
#ifdef NOISY
                        printk("region daemon dying\n");
#endif
			tinfo->dead = TRUE;
			return 0;
			/*
			 * NOTREACHED
			 */
		}
	}

}

int _udi_allocd_pid = -1;
int _udi_regiond_pid = -1;

int 
kthread_init_module (void *arg)
{
    udi_kthread_info_t *tinfo = (udi_kthread_info_t*)arg;
#if DEBUG
    long loop;
#endif
    /*
     * This kthread init is required so that resources we allocate have
     * the same memory space and file system configuration that the other
     * created kthreads have.
     * All UDI resources MUST be inited/deinited in this kthread context!
     */
    if (tinfo == NULL && tinfo->thread_name != NULL)
    	__udi_kthread_common_init(tinfo->thread_name);

#ifdef INSTRUMENT_MALLOC
    _OSDEP_SIMPLE_MUTEX_INIT(&_udi_allocation_log_lock, "udi_memd");
    UDI_QUEUE_INIT(&_udi_allocation_log);
#endif

#if DEBUG
#define MEMTEST_MAX 8193
#define MEMTEST_MIN 1
    _OSDEP_PRINTF("udi_env: Starting memory allocator tests.\n");
    for (loop = MEMTEST_MIN; loop < MEMTEST_MAX; loop++) {
	void *mem;
	mem = _OSDEP_MEM_ALLOC(loop, 0, UDI_WAITOK);
	if (mem) {
		memset(mem, 0, loop);
		_OSDEP_MEM_FREE(mem);
	}
	else {
		_OSDEP_PRINTF("udi memup: Failed to alloc %ld bytes.\n", loop);
	}
    }
    for (loop = MEMTEST_MAX; loop > MEMTEST_MIN; loop--) {
	void *mem;
	mem = _OSDEP_MEM_ALLOC(loop, 0, UDI_WAITOK);
	if (mem) {
		memset(mem, 0, loop);
		_OSDEP_MEM_FREE(mem);
	}
	else {
		_OSDEP_PRINTF("udi memdown: Failed to alloc %ld bytes.\n", loop);
	}
    }
    _OSDEP_PRINTF("udi_env: Finished memory allocator tests.\n");
#endif

    _OSDEP_EVENT_INIT (&_udi_ev);
    _OSDEP_EVENT_INIT (&_udi_alloc_event);
   
    osdep_dma_init_cache(); 
    
    osdep_timer_init();

    _udi_alloc_init ();
    _udi_MA_init ();

    UDI_QUEUE_INIT(&_udi_region_q_head);
    _OSDEP_MUTEX_INIT(&_udi_region_q_lock, "udi_rgn_q");
    _OSDEP_EVENT_INIT(&_udi_region_q_event);
    _OSDEP_SEM_INIT(&udi_allocd_tinfo.running, 0); /* Allocate acquired */
    _OSDEP_SEM_INIT(&udi_regiond_tinfo.running, 0); /* Allocate acquired */

    printk ("udi: module udi_MA spawning alloc_daemon\n");
    _udi_allocd_pid = kernel_thread(_udi_alloc_daemon, 
			&udi_allocd_tinfo, 0/*CLONE_FS*/);
    if (_udi_allocd_pid < 0)
    {
            printk("udi: failed to start the allocator daemon.\n");
               return -1;
    }

    printk ("udi: module udi_MA spawning region_daemon\n");
    _udi_regiond_pid = kernel_thread(_udi_region_daemon, 
			&udi_regiond_tinfo, 0/*CLONE_FS*/);
    if (_udi_regiond_pid < 0)
    {
      printk("udi: failed to start the region controller daemon.\n");
            return -1;
    }

    /* wait for a join with the alloc daemon */
    _OSDEP_SEM_P(&udi_allocd_tinfo.running);
    _OSDEP_SEM_V(&udi_allocd_tinfo.running);
    _OSDEP_SEM_DEINIT(&udi_allocd_tinfo.running);
    /* wait for a join with the region daemon */
    _OSDEP_SEM_P(&udi_regiond_tinfo.running);
    _OSDEP_SEM_V(&udi_regiond_tinfo.running);
    _OSDEP_SEM_DEINIT(&udi_regiond_tinfo.running);
             
    printk("udi: module udi_MA loaded\n");
    _udi_env_kthread_init = 1;
    _OSDEP_SEM_V(&_udi_MA_init_complete);
    if (tinfo != NULL)
	tinfo->dead = TRUE;
    MOD_DEC_USE_COUNT;
    return 0;
}

extern int udi_env_pid;
int udi_env_pid = -1;

int 
init_module(void)
{
    printk("UDI Reference Environment, Linux Kernel Module\n"
           "    UDI for Linux updates:  http://www.stg.com/udi\n"
	   "Copyright (c) 1995-2001; Compaq Computer Corporation; Hewlett-Packard\n"
	   "Company; Interphase Corporation; The Santa Cruz Operation, Inc;\n"
	   "Software Technologies Group, Inc; and Sun Microsystems, Inc\n"
	   "(collectively, the \"Copyright Holders\").  All rights reserved.\n");

    MOD_INC_USE_COUNT;
    _OSDEP_SEM_INIT(&_udi_MA_init_complete, 0); /* Allocate acquired */
#ifndef UDISETUP_USES_KTHREADS
    /*
     * Make a thread to initialize the mod and return immediately. This way the
     * kernel sees that we initialized correctly even if the module crashes. We
     * should still be able to remove a dead module as long as we were
     * initialized.
     */
#ifdef DEBUG
    printk("this_module = %p\n", &__this_module); /* for debugging purposes */
#endif
    udi_env_pid = kernel_thread( kthread_init_module,
				&udi_envd_tinfo, 0/*CLONE_FS*/);
#else
    udi_env_pid = kthread_init_module(NULL);
#endif
    if (udi_env_pid < 0)
    {
        printk(KERN_ERR "Failed to fork udi_env\n");
        _OSDEP_SEM_DEINIT(&_udi_MA_init_complete);
        MOD_DEC_USE_COUNT;
        return udi_env_pid;
    }
#ifdef DEBUG
    printk("udi_env_pid = %d\n", udi_env_pid);
#endif
    _OSDEP_SEM_P(&_udi_MA_init_complete);
#ifdef DEBUG
    printk("UDI Environment ready.\n");
#endif
    _OSDEP_SEM_V(&_udi_MA_init_complete);
    return 0;
}

STATIC void 
udi_unload_complete (void)
{
    _OSDEP_EVENT_SIGNAL (&_udi_ev);
}

void 
cleanup_module (void)
{
    if (_udi_env_kthread_init != 1)
        return;
    /*
     * Right now, we are executing in a kthread that doesn't have the
     * same context as the kthread we spawned earlier in init_module.
     * So, let's switch into a similar context... problem: exiting
     * back to the shell will result in a kernel oops.
     */
    /*__udi_kthread_common_init("udi_MA-cleanup_module");*/

    _udi_MA_deinit (udi_unload_complete);
    _OSDEP_EVENT_WAIT (&_udi_ev);
    _udi_kill_daemon = 1;
    _OSDEP_EVENT_SIGNAL(&_udi_alloc_event);
    /*
     * TBD: Wait for an acknowledgement from the alloc daemon that it died.
     *  Try wait4(_udi_allocd_pid);
     */
    /* For now, just sleep until the tinfo->dead flag changes. */
    while (udi_allocd_tinfo.dead == FALSE) {
        schedule();
    }
    _OSDEP_EVENT_SIGNAL(&_udi_region_q_event);
    /*
     * TBD: Wait for an acknowledgement from the region daemon that it died.
     *  Try wait4(_udi_regiond_pid);
     */
    /* For now, just sleep until the tinfo->dead flag changes. */
    while (udi_regiond_tinfo.dead == FALSE) {
        schedule();
    }
    osdep_dma_dispose_of_cache();

    osdep_timer_deinit();
    _OSDEP_EVENT_DEINIT(&_udi_ev);
    _OSDEP_EVENT_DEINIT(&_udi_alloc_event);
    _OSDEP_EVENT_DEINIT(&_udi_region_q_event);
    _OSDEP_MUTEX_DEINIT(&_udi_region_q_lock);
    _OSDEP_SEM_DEINIT(&_udi_MA_init_complete);
#ifdef INSTRUMENT_MALLOC
    _OSDEP_SIMPLE_MUTEX_DEINIT(&_udi_allocation_log_lock);
#endif
    printk ("udi: module udi_MA unloaded\n");
}

void 
__udi_kthread_common_init(char *thread_name)
{
	struct fs_struct *fs;
#define SIGS_TO_CATCH (sigmask(SIGKILL) | sigmask(SIGINT) | sigmask(SIGTERM))
#if NOISY
	/*
	 * Host: SMP kernel running on a UP machine.
	 * Problem: kernel thread init panics.
	 */
	int dbg_kthread_init = 1;
#else
	int dbg_kthread_init = 0;
#endif

	lock_kernel();
	if (dbg_kthread_init) printk("Unmapping userland\n");
	exit_mm(current); /* unmap userland */
	current->session = 1;
	current->pgrp = 1;
	current->tty = NULL; /* don't associate with the current tty */
	/* act like the mercilless init task */
	if (dbg_kthread_init) printk("Unmapping fs access.\n");
	exit_fs(current);
	if (dbg_kthread_init) printk("Setting fs access = init_task.fs.\n");
	fs = init_task.fs;
	current->fs = fs;
	atomic_inc(&fs->count);

	if (dbg_kthread_init) printk("Setting kthread task signal info.\n");	
	siginitsetinv(&current->blocked, SIGS_TO_CATCH);

	if (dbg_kthread_init) printk("Assigning current->command_line\n");
	sprintf(current->comm, "%s", thread_name); /* set our command name */
#ifdef NOISY
	_OSDEP_PRINTF("%s's task struct is at %p.\n", thread_name, current);
#endif
	if (dbg_kthread_init) printk("Unlocking kernel.\n");
	unlock_kernel();
	if (dbg_kthread_init) printk("Done kthread_common_init\n");
}

/* ...
 * if (__udi_kthread_signal_pending())
 *    shutdown_environment...
 * ...
 */
int 
__udi_kthread_signal_pending()
{
	return signal_pending(current);
}

