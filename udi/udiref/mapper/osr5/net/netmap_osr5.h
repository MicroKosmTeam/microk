/*
 * File: osr5/net/netmap_osr5.h
 *
 * OpenServer-specific NIC mapper private decls.
 *
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

#ifndef _NSRMAP_OSR5_H
#define	_NSRMAP_OSR5_H

#define	NsrmapS_RxWaiting	0x00000010	/* Waiting for Rx notification*/
#define	NsrmapS_TxWaiting	0x00000020	/* Waiting for Tx CB	      */

#define	NETMAP_OSDEP_H

typedef struct {
	_OSDEP_SIMPLE_MUTEX_T enable_mutex;
	volatile queue_t *enableq;
} netmap_osdep_wrenable_t;

typedef struct netmap_osdep {
	union {
		mac_stats_tr_t		t;	/* Token-Ring stats	*/
		mac_stats_eth_t		e;	/* Ethernet stats	*/
		mac_stats_fddi_t	f;	/* FDDI stats		*/
	} s_u;
	udi_ubit32_t		link_bps;	/* Link-speed (bps)	*/
	udi_ubit32_t		link_mbps;	/* Link-speed (mbps)	*/
	struct pci_devinfo	devinfo;
	mblk_t		*scratch_msg;		/* Original data message      */
	mblk_t		*scratch_ptr;		/* Current mblk within message*/
	udi_size_t	scratch_offset;		/* Byte offset within message */
	void		*mac_cookie;		/* Cookie set in MAC_BIND_REQ */
	udi_ubit32_t	ioc_cmd;		/* ioctl command	      */
	void		*ioc_datap;		/* ioctl data buffer reference*/
	void		*ioc_cmdp;		/* ioctl command reference    */
	udi_boolean_t	ioc_copyout;		/* Data to copyout to user    */
	udi_status_t	ioc_retval;		/* Return status for ioctl    */
	queue_t		*upQ;			/* Upstream queue	      */
	netmap_osdep_wrenable_t we;	/* Downstream flow control    */
	_OSDEP_EVENT_T	sv1;			/* Sync Variables	      */
	_OSDEP_EVENT_T	sv2;
	_OSDEP_EVENT_T	op_close_sv;
	_OSDEP_EVENT_T	ioc_sv;			/* Ioctl sync. variable	      */
	_OSDEP_MUTEX_T	op_mutex;
	udi_cb_t	*ioc_cb;		/* udi_nic_ctrl_cb_t for ioctl*/
	udi_ubit32_t	multicast_state;	/* track multicast ND state   */
	_OSDEP_SIMPLE_MUTEX_T mca_mutex;	/* protect the multicast table*/
	udi_boolean_t	nsr_should_filter_mcas; /* call mdi_valid_mca */
	struct _osdep_mca {			/* Active multicast table     */
		udi_ubit32_t	n_entries;
		void		*mca_table;
	} MCA_list;
	struct nsrmap_device	*ndevicep;
	udi_boolean_t	bound;			/* MDI_BIND_REQ processed */
} netmap_osdep_t;

#define	NETMAP_OSDEP_SIZE	sizeof( netmap_osdep_t )
#define NETMAP_STAT_SIZE	offsetof( netmap_osdep_t, link_bps ) - offsetof( netmap_osdep_t, s_u.t)

typedef struct netmap_osdep_tx {
	netmap_osdep_wrenable_t *wep;
} netmap_osdep_tx_t;

#define NETMAP_OSDEP_TX_SIZE		(sizeof(netmap_osdep_tx_t))

#include <sys/async.h>

/* NSR specific ioctls	*/
#define NSR_UDI_IOC(x)			(('n' << 8) | (x))
#define	NSR_UDI_NIC_BAD_RXPKT		(udi_ubit32_t) NSR_UDI_IOC(0)
#define	NSR_UDI_NIC_BAD_RXPKT_INTERN	(udi_ubit32_t) NSR_UDI_IOC(1)
#define	NSR_UDI_NIC_HW_RESET		(udi_ubit32_t) NSR_UDI_IOC(2)
#define	NSR_UDI_NIC_PROMISC_OFF		(udi_ubit32_t) NSR_UDI_IOC(3)
#define	NSR_UDI_NIC_PROMISC_ON		(udi_ubit32_t) NSR_UDI_IOC(4)

/* MAC address command format */
typedef struct {
	udi_ubit32_t	ioc_cmd;	/* Command 			*/
	udi_ubit32_t	ioc_len;	/* Length of data buffer	*/
	void		*ioc_data;	/* Data buffer (in mblk_t)	*/
} nsr_macaddr_t;

/* Multicast address */
#define UDI_MCA(ea, media_type) (((caddr_t)(ea))[0] & 1)

/* Broadcast address */
#define UDI_BCAST(ea) ((((caddr_t)(ea))[0] & 0xff) == 0xff && \
                          mdi_addrs_equal(netmap_broad, ea))

#endif	/* _NSRMAP_OSR5_H */
