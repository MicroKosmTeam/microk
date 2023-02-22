/*
 * File: mapper/posix/net/netmap_posix.h
 *
 * POSIX netmapper private header
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


#ifndef NETMAP_OSDEP_H
#define	NETMAP_OSDEP_H

#define UDI_POSIX_ENV

typedef struct netmap_osdep {
	void	*rdata_gio;
	union {
		udi_ubit8_t	array[20];
		void		*ptr;
		/* Space for statistics counters dependent on the type of the
		 * underlying media. Normally this would be whatever the OS
		 * supports (e.g. TokenRing, FDDI, Ether etc.)
		 */
	} u;
	udi_ubit32_t	status;			/* OS-dependent status */
	udi_ubit32_t	ioc_cmd;		/* ioctl command */
	void		*ioc_datap;		/* ioctl data buffer reference*/
	void		*ioc_cmdp;		/* ioctl command reference */
	udi_boolean_t	ioc_copyout;		/* Data to copyout to user */
	udi_status_t	ioc_retval;		/* Return status for ioctl */
	_OSDEP_EVENT_T	sv1;			/* Sync Variable	*/
	_OSDEP_EVENT_T	sv2;
	_OSDEP_EVENT_T	op_close_sv;
	_OSDEP_MUTEX_T	op_mutex;
	_OSDEP_EVENT_T	ioc_sv;			/* Ioctl sync. variable */
	udi_cb_t	*ioc_cb;		/* udi_nic_ctrl_cb for ioctl */
	struct _osdep_mca {			/* Active multicast list */
		udi_ubit32_t	n_entries;
		void		*mca_table;
	} MCA_list;
} netmap_osdep_t;

typedef struct netmap_Rx_osdep {
	udi_ubit32_t	status;			/* OS-dependent status  */
	_OSDEP_EVENT_T	sv2;			/* Read sync variable	*/
} netmap_Rx_osdep_t;

/* OS-dependent status values */
#define	OSNsrmapRx_Waiting	0x00000001	/* App waiting for Rx */
#define	OSNsrmapTx_Waiting	0x00000002	/* App waiting for Tx */

#define	NETMAP_OSDEP_SIZE	sizeof( netmap_osdep_t )
#define	NETMAP_RX_OSDEP_SIZE	sizeof( netmap_Rx_osdep_t )

/*
 * nsrmap_ioctl: definitions
 */

/*
 * Multicast Address structure definition, used for SETMCA, DELMCA, SETALLMCA, 
 * DELALLMCA 
 */
typedef struct {
	udi_ubit32_t	ioc_cmd;	/* Command 			*/
	udi_ubit32_t	ioc_len;	/* Length of data (bytes)	*/
	void		*ioc_data;	/* Data buffer 			*/
	udi_ubit32_t	ioc_num;	/* Number of entries		*/
} nsr_mca_struct_t;

typedef struct {
	udi_ubit32_t	ioc_cmd;	/* Command			*/
	udi_ubit32_t	ioc_len;	/* Length of data buffer	*/
	void		*ioc_data;	/* Data buffer (user supplied)	*/
} nsr_macaddr_t;

#define	POSIX_ND_SETMCA		0x00000001	/* Set MCA addresses	      */
#define	POSIX_ND_DELMCA		0x00000002	/* Clear specified MCA addrs  */
#define	POSIX_ND_GETADDR	0x00000004	/* Get current MAC address    */
#define	POSIX_ND_SETADDR	0x00000008	/* Set current MAC address    */
#define	POSIX_ND_GETRADDR	0x00000010	/* Get factory MAC address    */
#define	POSIX_ND_SETALLMCA	0x00000020	/* Enable all MCA addresses   */
#define	POSIX_ND_DELALLMCA	0x00000040	/* Disable all MCA addresses  */
#define	POSIX_ND_PROMISC_ON	0x00000080	/* Enable promiscuous mode    */
#define	POSIX_ND_PROMISC_OFF	0x00000100	/* Disable promiscuous mode   */
#define	POSIX_ND_HWRESET	0x00000200	/* Reset hardware	      */
#define	POSIX_ND_BAD_RXPKT	0x00000400	/* Set bad Rx pkt behaviour   */
#define	POSIX_ND_RETMCA		0x00000800	/* Return active MCA addresses*/

/*
 * Helper macros
 */
#define	NETMAP_COPYIN( src, dest, len ) \
	udi_memcpy( dest, src, len )

#define	NETMAP_COPYOUT( src, dest, len ) \
	udi_memcpy( dest, src, len )
#endif	/* NETMAP_OSDEP_H */
