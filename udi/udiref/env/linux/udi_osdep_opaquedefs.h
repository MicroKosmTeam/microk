/*
 * File: linux/udi_osdep_opaquedefs.h
 *
 * Linux-specific component of environment.
 * This file helps to isolate Linux kernel headers from UDI headers.
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

#ifndef __UDI_OSDEP_OPAQUEDEFS_H__
#define __UDI_OSDEP_OPAQUEDEFS_H__

/* Tell the Linux Kernel headers we want kernel namespace.  */
#define __KERNEL__ 1

/* Tell the Linux Kernel headers we want to be able to make syscalls.  */
#define __KERNEL_SYSCALLS__ 1

/* Tell the Linux Kernel headers we want to be a module.  */
#define MODULE 1

/*
 * Dynamically turn on the __SMP__ define based on the current build
 * configuration of the system. This is done because sched.h includes SMP
 * headers which result in the compilation failing due to the use of an
 * undefined macro.
 *
 * Note: This is not foolproof. It is possible for the user to change their
 * configuration, but NOT rebuild the kernel resulting in an autoconf.h that
 * indicates building for an SMP kernel while the kernel that is running is
 * NOT SMP. In a case like this, loading the UDI environment module should
 * fail. The autoconf.h file and the current kernel are assumed to be in sync
 * before the UDI environment can be loaded successfully.
 */
#include <linux/config.h>
#include <linux/version.h>
#ifdef CONFIG_SMP
#define __SMP__ 1
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
#define SIMPLER_EVENTS 1
#else
#define SIMPLER_EVENTS 0
#endif

/*
 * When Module Symbol Versions are enabled, we need to be one of the
 * absolutely first header files to load. The modversions.h file
 * causes a ton of mappings of symbol names to occur by using #defines.
 * This pollutes the namespace pretty severely.
 *  eg. #define event event_R7b16c344
 * So, if you're debugging the environment, expect to see some
 * munged symbol names inside structs and local variable decls.
 */

/*
 * Dont modify any existing __NO_VERSION__ preprocessor macros.
 * We want to at least bind against a particular kernel version.
 * That way a user can use the "-f" flag to force the module to
 * load at the least.
 */

#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
#define MODVERSIONS
#endif

#ifdef MODVERSIONS
#ifdef __NO_VERSION__
/*#undef MODVERSIONS*/ /* ???? */
#endif
#include <linux/modversions.h>
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/unistd.h> /* for kernel_thread 2.2.16 */
#include <linux/kernel.h>   /* for all sorts of kernel stuff */

#include <linux/fs.h>
#include <asm/uaccess.h>
#ifdef __alpha__
#include <linux/delay.h>
#else
#include <asm/delay.h>
#endif

#include <linux/time.h>     /* sched.h SHOULD do this, but it doesn't! */
#include <linux/timer.h>    /* kernel timer support */
#include <linux/stddef.h>   /* for offsetof() which is used in udi_init.c */
#include <linux/wait.h>     /* for struct wait_queue */
#include <linux/sched.h>    /* for wake_up() */
#include <linux/string.h>   /* for string utility functions */
#include <linux/pci.h>      /* for PCI config space defs & functions */
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/tqueue.h>
#include <linux/slab.h>     /* For kmalloc() and kfree() */
#include <asm/semaphore.h>
#include <linux/unistd.h>   /* for kernel_thread and other syscalls */

#include <asm/system.h>
/*#include <spinlock.h>*/
#include <linux/smp_lock.h>

/* Opaque data type definitions */
/* zero-length arrays are a gcc extension */
#define STRUCT_ALIGNMENT_PADDING(x) unsigned long PADDING##x[0];
/*#define STRUCT_ALIGNMENT_PADDING*/


/* Allow UDI types to be used... */
#define UDI_VERSION 0x101
#include "udi.h"

/* These don't use kernel headers */
/*
_OSDEP_DRIVER_T
_OSDEP_DMA_T
_OSDEP_ISR_RETURN_T
*/

/* Mutexes */
typedef struct _udi_osdep_spinlock_opaque {
    STRUCT_ALIGNMENT_PADDING(TOP)
    spinlock_t               lock;       /* this is the standard spinlock */
    volatile unsigned long   spinflags;  /* save and restore flags (irq) */
    char                     *lockname;     /* init info */
    char                     *filename;     /* init info */
    unsigned int             lineno;        /* init info */
    unsigned int             locked_by_cpu; /* lock owner info */
    unsigned long            locked_by_eip; /* lock owner info */
    char                     *locked_by_filename; /* lock owner info */
    unsigned int             locked_by_lineno;    /* lock owner info */
    STRUCT_ALIGNMENT_PADDING(BOT)
} udi_osdep_spinlock_opaque_t;

/* Semaphores */
typedef struct _udi_osdep_semaphore_opaque
{
    STRUCT_ALIGNMENT_PADDING(TOP)
    struct semaphore   instance;
    STRUCT_ALIGNMENT_PADDING(BOT)
} udi_osdep_semaphore_opaque_t;

/* Atomic operations */
typedef struct _udi_osdep_atomic_int_opaque {
    STRUCT_ALIGNMENT_PADDING(TOP)
    atomic_t val;
    STRUCT_ALIGNMENT_PADDING(BOT)
} udi_osdep_atomic_int_opaque_t;

#if SIMPLER_EVENTS

typedef struct _udi_osdep_event_opaque
{
    STRUCT_ALIGNMENT_PADDING(TOP)
    spinlock_t          lock;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
    struct wait_queue   *wait_q;
#else
    wait_queue_head_t   wait_q;
#endif
    volatile int        signaled;
    volatile int        destroyed;
    volatile int        waiting;
    volatile int        waiter_busy;
    volatile int        waiter_waiting;
    STRUCT_ALIGNMENT_PADDING(BOT)
} udi_osdep_event_opaque_t;

#else
typedef struct _udi_osdep_event_opaque
{
    STRUCT_ALIGNMENT_PADDING(TOP)
    spinlock_t          lock;
    volatile long       wait;
    volatile long       signal;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
    struct wait_queue   *wait_q;
#else
    wait_queue_head_t   wait_q;
#endif
    volatile long       init_count;
    udi_osdep_semaphore_opaque_t evil_dead;
    volatile long	dead;
    volatile long       dying;
    struct tq_struct    event_deinit_bh;
    struct tq_struct    event_signal_bh;
    STRUCT_ALIGNMENT_PADDING(BOT)
} udi_osdep_event_opaque_t;
#endif

typedef struct _udi_osdep_dev_node_opaque
{
    STRUCT_ALIGNMENT_PADDING(TOP)
    udi_boolean_t	inited;
    struct pci_dev*	node_devinfo;
    struct _udi_osdep_spinlock_opaque	attr_storage_mutex;
    udi_queue_t		attr_storage;
    struct _udi_osdep_event_opaque	enum_done_ev;
    STRUCT_ALIGNMENT_PADDING(BOT)
} udi_osdep_dev_node_opaque_t;

typedef struct _udi_osdep_pio_opaque {
    STRUCT_ALIGNMENT_PADDING(TOP)
    struct pci_dev *pio_pci_dev;

    /* bounds checking */
    unsigned long baseaddr;
    unsigned long mapped_max_offset;
    unsigned long hw_max_offset;

        union {
                struct {
                        unsigned long addr;
                } io;
                struct {
                        unsigned char* mapped_addr;
                } mem;
                struct {
                        size_t offset;
                } cfg;
        } _u;
    STRUCT_ALIGNMENT_PADDING(BOT)
} udi_osdep_pio_opaque_t;

typedef struct _udi_osdep_driver_opaque {
    STRUCT_ALIGNMENT_PADDING(TOP)
    struct _udi_std_sprops *sprops;
    struct module *mod;
    STRUCT_ALIGNMENT_PADDING(BOT)
} udi_osdep_driver_opaque_t;

#endif
