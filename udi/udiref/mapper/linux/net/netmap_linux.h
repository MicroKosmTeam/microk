/*
 * File: linux/net/netmap_linux.h
 *
 * Linux-specific NIC mapper private decls.
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

#ifndef NETMAP_LINUX_H
#define	NETMAP_LINUX_H

#define	NETMAP_OSDEP_H

#if 0
#define NSRMAP_CB_ALLOC_BATCH osnsr_cb_alloc_batch 
#endif

#define OSNSR_BIND_CB_NUM	8

typedef struct netmap_osdep {
    struct net_device	*devp;		/* Linux net device structure */
    _OSDEP_SEM_T	enable_sem;	/* signals when enable is done */
    _OSDEP_SEM_T	disable_sem;
    _OSDEP_SEM_T	info_sem;	/* semaphore protecting the stats */
    udi_cb_alloc_batch_call_t *callback;
    udi_index_t count;
    udi_size_t buf_size;
    udi_buf_path_t path_handle;
    void *orig_context;
    udi_nic_rx_cb_t* rxcb_chain1;
    udi_nic_rx_cb_t* rxcb_chain2;

    udi_cb_t * bind_cb_pool;
} netmap_osdep_t;

#define	NETMAP_OSDEP_SIZE	sizeof(netmap_osdep_t)

/*
 * This structure is pointed to by priv in the Linux device struct.
 */

typedef struct osnsr_info {
    struct nsrmap_region_data *rdata;
    struct nsrmap_Tx_region_data *txdata;
    struct net_device_stats stats;
    struct dev_mc_list *mc_cur;		/* Current multicast list */
    int cur_size;			/* Size of addrs in above list */
    struct dev_mc_list *mc_add;		/* Multicast addrs to be added */
    int add_size;			/* Size of addrs in above list */
    struct dev_mc_list *mc_del;		/* Multicast addrs to be deleted */
    int del_size;			/* Size of addrs in above list */
    char *buf;				/* Buffer used to send addrs to ND */
    unsigned short flags_current;	/* current IF flags */
    unsigned short flags_new;		/* new IF flags */
    void* mac_addr;			/* new mac address to be set */
    udi_ubit8_t queue_stopped;		/* 1 if the tx queue is stopped */
    _OSDEP_SIMPLE_MUTEX_T	lock;

} osnsr_info_t;

typedef struct device nsrmap_osdep_tx_t;

#endif	/* NETMAP_LINUX_H */
