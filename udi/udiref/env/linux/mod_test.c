/*
 * File: linux/mod_test.c
 *
 * Unit test the Linux Kernel environment.
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

/*
 * This is the only file in this module that will contain the version info,
 * so remove the __NO_VERSION__ macro.
 */
#undef __NO_VERSION__

#define UDI_KERNEL_NAMESPACE 0

#if UDI_KERNEL_NAMESPACE

#include "udi_osdep_opaquedefs.h"
#include <udi_env.h>

#else

#define __KERNEL__ 1
#define __KERNEL_SYSCALLS__ 1
#define MODULE 1
#include <linux/config.h>
#include <linux/version.h>
#ifdef CONFIG_SMP
#define __SMP__ 1
#endif

#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
#define MODVERSIONS
#endif

#ifdef MODVERSIONS
#ifdef __NO_VERSION__
#endif
#include <linux/modversions.h>
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/unistd.h> /* for kernel_thread 2.2.16 */

#include <linux/types.h>    /* for standard types like size_t */
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
#include <linux/smp_lock.h>
#endif /* UDI_KERNEL_NAMESPACE */

#include <linux/delay.h>
#include <linux/kmod.h>

#include <linux/pci.h>
#include <asm/io.h>
#include <asm/semaphore.h>

static int worker_thread(void *tinfo);
static int mapper_thread(void *tinfo);

/* Everyone knows about these variables */
struct pci_dev *pci_dev;
void *vhwaddr; /* address of the virtual hardware address pointer */
unsigned int osbase, oslen;

/* These are per-thread variables */
typedef struct thread_info {
	struct semaphore 	started_mutex, complete_mutex;
	volatile unsigned char	ready, go, done;
} thread_info_t;

thread_info_t mapper_tinfo, worker_tinfo;
int mapper_pid, worker_pid;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/******************************************************************************
 * Function: init_module()
 *
 * Abstract: This is the entry point for the mod_test.o loadable module.
 *****************************************************************************/
int init_module( void )
{
	unsigned int hwbase, hwmask;
	unsigned short command;
	unsigned char latency;

	vhwaddr = 0;
	memset(&mapper_tinfo, 0, sizeof(mapper_tinfo));
	memset(&worker_tinfo, 0, sizeof(worker_tinfo));

	printk("Hello kernel\n");
/* Let's play with some known networking devices. */
#if 1
/* Asante FriendlyNet 10/100 PCI */
#define TULIP_COMPATIBLE 1
#define PCI_VENDOR 0x11ad
#define PCI_DEVICE 0xc115
#else
/* 3com stuff here */
#define E3H_COMPATIBLE 1
#define PCI_VENDOR 0x107b
#define PCI_DEVICE 0x9055
#endif

	/* Find first device. */
	pci_dev = pci_find_device(PCI_VENDOR, PCI_DEVICE, NULL);
	if (pci_dev == NULL)
		return -ENODEV;

	/* Get BAR0 hardware address */
	pci_read_config_dword(pci_dev, PCI_BASE_ADDRESS_1, &hwbase);
	pci_write_config_dword(pci_dev, PCI_BASE_ADDRESS_1, 0xFFFFFFFF);
	pci_read_config_dword(pci_dev, PCI_BASE_ADDRESS_1, &hwmask);
	pci_write_config_dword(pci_dev, PCI_BASE_ADDRESS_1, hwbase);

	/* Assume memory mapped */
	oslen = (~(hwmask & ~0x0F) + 1);

	/* Get other registers */
	pci_read_config_word(pci_dev, PCI_COMMAND, &command);
	pci_read_config_byte(pci_dev, PCI_LATENCY_TIMER, &latency);

	/* Get osbase */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,3,0)
	osbase = pci_dev->base_address[1];
#else
	osbase = pci_resource_start(pci_dev, 1);
#endif
	printk("hwbase=%08X hwmask=%08X osbase=%08X oslen=%08X\n", hwbase, hwmask, osbase, oslen);
	printk("command=%04hX latency=%02X\n", command, latency);

	/*
	 * Try spawning off a kernel_thread and executing this code in it.
	 * maybe ioremap's virtual memory mappings are not shared between
	 * the UDI processes?
	 */
	printk("Spawning worker thread...\n");
	worker_pid = kernel_thread(worker_thread, (void*)&worker_tinfo, 0/*CLONE_VM*/);
	if (worker_pid < 0) {
		printk("Failed to start worker thread %d\n", worker_pid);
	}
	else {
		printk("Worker kernel thread pid = %d\n", worker_pid);
	}

	/* Wait for worker to become ready */
	while (worker_tinfo.ready == FALSE) {
		schedule();
	}

	/* Create mapper. Wait for it to say it's ready. */
	printk("Spawning mapper thread...\n");
	mapper_pid = kernel_thread(mapper_thread, (void*)&mapper_tinfo, 0/*CLONE_VM*/);
	if (mapper_pid < 0) {
		printk("Failed to start mapper thread %d\n", mapper_pid);
	}
	else {
		printk("Mapper kernel thread pid = %d\n", mapper_pid);
	}
	while (mapper_tinfo.ready == FALSE) {
		schedule();
	}
	printk("vhwaddr = %p\n", vhwaddr);

	/* Tell worker to start */
	worker_tinfo.go = TRUE;

	/* Wait for worker to finish before unmapping. */
	while (worker_tinfo.done == FALSE) {
		schedule();
	}
	printk("Worker thread exited.\n");

	/* Tell mapper to unmap */
	mapper_tinfo.go = TRUE;
	while (mapper_tinfo.done == FALSE) {
		schedule();
	}
	printk("Mapper thread exited.\n");

	/* Return an error since we're done doing our mischeif. */	
	return 0/*-ENODEV*/;
}

static int mapper_thread(void *tinfo)
{
	thread_info_t *mapper_tinfo = (thread_info_t*)tinfo;

	/* name ourselves. */
	lock_kernel();
	sprintf(current->comm, "mod_test-mapper");
	unlock_kernel();

	printk("Mapper thread: hello world\n");

	mapper_tinfo->ready = TRUE;
	vhwaddr = ioremap(osbase, oslen);
	printk("vhwaddr = %p\n", vhwaddr);

	while (mapper_tinfo->go == FALSE) {
		schedule();
	}
	iounmap(vhwaddr);

	mapper_tinfo->done = TRUE;
	return 0;
}

#if TULIP_COMPATIBLE
static int worker_thread(void *tinfo)
{
	thread_info_t *worker_tinfo = (thread_info_t*)tinfo;
	unsigned long csr;
	unsigned short command;
	unsigned char latency;

	/* name ourselves. */
	lock_kernel();
	sprintf(current->comm, "mod_test-worker");
	unlock_kernel();

	printk("Worker thread: hello world\n");

	worker_tinfo->ready = TRUE;
	while (worker_tinfo->go == FALSE) {
		schedule();
	}
	
	/* Read some settings */
	pci_read_config_word(pci_dev, PCI_COMMAND, &command);
	pci_read_config_byte(pci_dev, PCI_LATENCY_TIMER, &latency);

	/* Adjust some config space settings */
	command |= PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY;
	pci_write_config_word(pci_dev, PCI_COMMAND, command);
	if (latency < 32)
		pci_write_config_word(pci_dev, PCI_LATENCY_TIMER, 64);

	/* disable tx & rx */
	#define CSR6 0x30
	csr = readl(vhwaddr + CSR6);
	printk("old csr6 = %08lX\n", csr);
	csr &= 0x2002;
	printk("new csr6 = %08lX\n", csr);
	writel(csr, (char*)vhwaddr + CSR6);

	/* clear missed packet counter */
	#define CSR8 0x40
	csr = readl((char*)vhwaddr + CSR8);
	printk("Missed packet counter: %08lX\n", csr);

	/* check if 21141 is 21140 compatible. */
	#define CSR9 0x48
	csr = readl((char*)vhwaddr + CSR9);
	printk("Is 21140 compatible: %08lX\n", csr & 0x8000);

	worker_tinfo->done = TRUE;
	return 0;
}
#elif E3H_COMPATIBLE
static int worker_thread(void *input)
{
	struct pci_dev *pci_dev = (struct pci_dev*)input;
	return 0;
}
#endif

/******************************************************************************
 * Function: cleanup_module()
 *
 * Abstract: This is the exit point for the mod_test.o loadable module.
 *****************************************************************************/
void cleanup_module( void )
{
}

