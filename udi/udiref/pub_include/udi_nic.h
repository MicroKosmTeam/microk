/*
 * File: udi_nic.h
 *
 * Public definitions for the UDI Network Interface Metalanguage.
 *
 * All UDI NIC and NSR drivers must #include this file.
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
 

#ifndef _UDI_NIC_H
#define	_UDI_NIC_H
/*
 * Validate UDI_NIC_VERSION.
 */
#if !defined(UDI_NIC_VERSION)
#error "UDI_NIC_VERSION must be defined."
#elif (UDI_NIC_VERSION != 0x101)
#error "Unsupported UDI_NIC_VERSION."
#endif

/*
 * Validate #include order.
 */
#ifndef _UDI_H
#error "udi.h must be #included before udi_nic.h."
#endif


/*----------------------------------------------------------------------------*/
/* Constant definitions							      */
/*----------------------------------------------------------------------------*/
#define	UDI_NIC_MAC_ADDRESS_SIZE	20


/*----------------------------------------------------------------------------*/
/* Control Block definitions.						      */
/*----------------------------------------------------------------------------*/
typedef struct {
	udi_cb_t	gcb;
} udi_nic_cb_t;

#define	UDI_NIC_STD_CB_NUM		1

typedef struct {
	udi_cb_t	gcb;
	udi_ubit8_t	media_type;	/* See below */
	udi_ubit32_t	min_pdu_size;
	udi_ubit32_t	max_pdu_size;
	udi_ubit32_t	rx_hw_threshold;
	udi_ubit32_t	capabilities;	/* See below */
	udi_ubit8_t	max_perfect_multicast;
	udi_ubit8_t	max_total_multicast;
	udi_ubit8_t	mac_addr_len;
	udi_ubit8_t	mac_addr[UDI_NIC_MAC_ADDRESS_SIZE];
} udi_nic_bind_cb_t;

#define	UDI_NIC_BIND_CB_NUM		2

/* media_type values */
#define	UDI_NIC_ETHER			0
#define	UDI_NIC_TOKEN			1
#define	UDI_NIC_FASTETHER		2
#define	UDI_NIC_GIGETHER		3
#define	UDI_NIC_VGANYLAN		4
#define	UDI_NIC_FDDI			5
#define	UDI_NIC_ATM			6
#define	UDI_NIC_FC			7
#define	UDI_NIC_MISCMEDIA		0xff

/* capabilities supported by the ND */
#define	UDI_NIC_CAP_TX_IP_CKSUM		(1U<<0)
#define	UDI_NIC_CAP_TX_TCP_CKSUM	(1U<<1)
#define	UDI_NIC_CAP_TX_UDP_CKSUM	(1U<<2)
#define	UDI_NIC_CAP_MCAST_LOOPBK	(1U<<3)
#define	UDI_NIC_CAP_BCAST_LOOPBK	(1U<<4)

/* capability requests from NSR */
#define	UDI_NIC_CAP_USE_TX_CKSUM	(1U<<30)
#define	UDI_NIC_CAP_USE_RX_CKSUM	(1U<<31)

typedef struct {
	udi_cb_t	gcb;
	udi_ubit8_t	command;	/* See below */
	udi_ubit32_t	indicator;
	udi_buf_t	*data_buf;
} udi_nic_ctrl_cb_t;

#define	UDI_NIC_CTRL_CB_NUM		3

/* command values */
#define	UDI_NIC_ADD_MULTI		0x1
#define	UDI_NIC_DEL_MULTI		0x2
#define	UDI_NIC_ALLMULTI_ON		0x3
#define	UDI_NIC_ALLMULTI_OFF		0x4
#define	UDI_NIC_GET_CURR_MAC		0x5
#define	UDI_NIC_SET_CURR_MAC		0x6
#define	UDI_NIC_GET_FACT_MAC		0x7
#define	UDI_NIC_PROMISC_ON		0x8
#define	UDI_NIC_PROMISC_OFF		0x9
#define	UDI_NIC_HW_RESET		0xA
#define	UDI_NIC_BAD_RXPKT		0xB

typedef struct {
	udi_cb_t	gcb;
	udi_ubit8_t	event;		/* See below */
} udi_nic_status_cb_t;

#define	UDI_NIC_STATUS_CB_NUM		4

/* event values */
#define	UDI_NIC_LINK_DOWN		0x0
#define	UDI_NIC_LINK_UP			0x1
#define	UDI_NIC_LINK_RESET		0x2

typedef struct {
	udi_cb_t	gcb;
	udi_boolean_t	interface_is_active;
	udi_boolean_t	link_is_active;
	udi_boolean_t	is_full_duplex;
	udi_ubit32_t	link_mbps;
	udi_ubit32_t	link_bps;
	udi_ubit32_t	tx_packets;
	udi_ubit32_t	rx_packets;
	udi_ubit32_t	tx_errors;
	udi_ubit32_t	rx_errors;
	udi_ubit32_t	tx_discards;
	udi_ubit32_t	rx_discards;
	udi_ubit32_t	tx_underrun;
	udi_ubit32_t	rx_overrun;
	udi_ubit32_t	collisions;
} udi_nic_info_cb_t;

#define	UDI_NIC_INFO_CB_NUM		5

typedef struct udi_nic_tx_cb_struct	udi_nic_tx_cb_t;
struct udi_nic_tx_cb_struct {
	udi_cb_t	gcb;
	udi_nic_tx_cb_t	*chain;
	udi_buf_t	*tx_buf;
	udi_boolean_t	completion_urgent;
};

#define	UDI_NIC_TX_CB_NUM		6


typedef struct udi_nic_rx_cb_struct	udi_nic_rx_cb_t;
struct udi_nic_rx_cb_struct {
	udi_cb_t	gcb;
	udi_nic_rx_cb_t	*chain;
	udi_buf_t	*rx_buf;
	udi_ubit8_t	rx_status;
	udi_ubit8_t	addr_match;
	udi_ubit8_t	rx_valid;
};

#define	UDI_NIC_RX_CB_NUM		7

/* values for rx_status */
#define	UDI_NIC_RX_BADCKSUM		(1U<<0)
#define	UDI_NIC_RX_UNDERRUN		(1U<<1)
#define	UDI_NIC_RX_OVERRUN		(1U<<2)
#define	UDI_NIC_RX_DRIBBLE		(1U<<3)
#define	UDI_NIC_RX_FRAME_ERR		(1U<<4)
#define	UDI_NIC_RX_MAC_ERR		(1U<<5)
#define	UDI_NIC_RX_OTHER_ERR		(1U<<7)

/* values for addr_match */
#define	UDI_NIC_RX_UNKNOWN		0x0
#define	UDI_NIC_RX_EXACT		0x1
#define	UDI_NIC_RX_HASH			0x2
#define	UDI_NIC_RX_BROADCAST		0x3

/* values for rx_valid */
#define UDI_NIC_RX_GOOD_IP_CKSUM  	(1U<<0)
#define UDI_NIC_RX_GOOD_TCP_CKSUM 	(1U<<1)
#define UDI_NIC_RX_GOOD_UDP_CKSUM 	(1U<<2)


/*----------------------------------------------------------------------------*/
/* Prototypes for "callee"-side interface operation entry points.	      */
/*----------------------------------------------------------------------------*/
typedef void (udi_nd_bind_req_op_t)(
		udi_nic_bind_cb_t	*cb,
		udi_index_t		tx_chan_index,
		udi_index_t		rx_chan_index);
typedef void (udi_nsr_bind_ack_op_t)(
		udi_nic_bind_cb_t	*cb,
		udi_status_t		status);
typedef void (udi_nd_unbind_req_op_t)(
		udi_nic_cb_t		*cb);
typedef void (udi_nsr_unbind_ack_op_t)(
		udi_nic_cb_t		*cb,
		udi_status_t		status);
typedef void (udi_nd_enable_req_op_t)(
		udi_nic_cb_t		*cb);
typedef void (udi_nsr_enable_ack_op_t)(
		udi_nic_cb_t		*cb,
		udi_status_t		status);
typedef void (udi_nd_disable_req_op_t)(
		udi_nic_cb_t		*cb);
typedef void (udi_nd_ctrl_req_op_t)(
		udi_nic_ctrl_cb_t	*cb);
typedef void (udi_nsr_ctrl_ack_op_t)(
		udi_nic_ctrl_cb_t	*cb,
		udi_status_t		status);
typedef void (udi_nsr_status_ind_op_t)(
		udi_nic_status_cb_t	*cb);
typedef void (udi_nd_info_req_op_t)(
		udi_nic_info_cb_t	*cb,
		udi_boolean_t		reset_statistics);
typedef void (udi_nsr_info_ack_op_t)(
		udi_nic_info_cb_t	*cb);
typedef void (udi_nsr_tx_rdy_op_t)(
		udi_nic_tx_cb_t		*cb);
typedef void (udi_nd_tx_req_op_t)(
		udi_nic_tx_cb_t		*cb);
typedef void (udi_nd_exp_tx_req_op_t)(
		udi_nic_tx_cb_t		*cb);
typedef void (udi_nsr_rx_ind_op_t)(
		udi_nic_rx_cb_t		*cb);
typedef void (udi_nsr_exp_rx_ind_op_t)(
		udi_nic_rx_cb_t		*cb);
typedef void (udi_nd_rx_rdy_op_t)(
		udi_nic_rx_cb_t		*cb);


/*----------------------------------------------------------------------------*/
/* Interface Operations vectors.					      */
/*----------------------------------------------------------------------------*/
typedef const struct {
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_nd_bind_req_op_t		*nd_bind_req_op;
	udi_nd_unbind_req_op_t		*nd_unbind_req_op;
	udi_nd_enable_req_op_t		*nd_enable_req_op;
	udi_nd_disable_req_op_t		*nd_disable_req_op;
	udi_nd_ctrl_req_op_t		*nd_ctrl_req_op;
	udi_nd_info_req_op_t		*nd_info_req_op;
} udi_nd_ctrl_ops_t;

#define	UDI_ND_CTRL_OPS_NUM		1

typedef const struct {
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_nd_tx_req_op_t		*nd_tx_req_op;
	udi_nd_exp_tx_req_op_t		*nd_exp_tx_req_op;
} udi_nd_tx_ops_t;

#define	UDI_ND_TX_OPS_NUM		2

typedef const struct {
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_nd_rx_rdy_op_t		*nd_rx_rdy_op;
} udi_nd_rx_ops_t;

#define	UDI_ND_RX_OPS_NUM		3

typedef const struct {
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_nsr_bind_ack_op_t		*nsr_bind_ack_op;
	udi_nsr_unbind_ack_op_t		*nsr_unbind_ack_op;
	udi_nsr_enable_ack_op_t		*nsr_enable_ack_op;
	udi_nsr_ctrl_ack_op_t		*nsr_ctrl_ack_op;
	udi_nsr_info_ack_op_t		*nsr_info_ack_op;
	udi_nsr_status_ind_op_t		*nsr_status_ind_op;
} udi_nsr_ctrl_ops_t;

#define	UDI_NSR_CTRL_OPS_NUM		4

typedef const struct {
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_nsr_tx_rdy_op_t		*nsr_tx_rdy_op;
} udi_nsr_tx_ops_t;

#define	UDI_NSR_TX_OPS_NUM		5

typedef const struct {
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_nsr_rx_ind_op_t		*nsr_rx_ind_op;
	udi_nsr_exp_rx_ind_op_t		*nsr_exp_rx_ind_op;
} udi_nsr_rx_ops_t;

#define	UDI_NSR_RX_OPS_NUM		6

/*----------------------------------------------------------------------------*/
/* Interface Operations  ("caller"-side)				      */
/*----------------------------------------------------------------------------*/

/*
 * Control entry points
 */
udi_nd_bind_req_op_t		udi_nd_bind_req;
udi_nd_unbind_req_op_t		udi_nd_unbind_req;
udi_nd_enable_req_op_t		udi_nd_enable_req;
udi_nd_disable_req_op_t		udi_nd_disable_req;
udi_nd_ctrl_req_op_t		udi_nd_ctrl_req;
udi_nd_info_req_op_t		udi_nd_info_req;

udi_nsr_bind_ack_op_t		udi_nsr_bind_ack;
udi_nsr_unbind_ack_op_t		udi_nsr_unbind_ack;
udi_nsr_enable_ack_op_t		udi_nsr_enable_ack;
udi_nsr_ctrl_ack_op_t		udi_nsr_ctrl_ack;
udi_nsr_info_ack_op_t		udi_nsr_info_ack;
udi_nsr_status_ind_op_t		udi_nsr_status_ind;

/*
 * Transmit entry points
 */
udi_nd_tx_req_op_t		udi_nd_tx_req;
udi_nd_exp_tx_req_op_t		udi_nd_exp_tx_req;

udi_nsr_tx_rdy_op_t		udi_nsr_tx_rdy;

/*
 * Receive entry points
 */
udi_nd_rx_rdy_op_t		udi_nd_rx_rdy;

udi_nsr_rx_ind_op_t		udi_nsr_rx_ind;
udi_nsr_exp_rx_ind_op_t		udi_nsr_exp_rx_ind;

/*----------------------------------------------------------------------------*/
/* State definitions							      */
/*----------------------------------------------------------------------------*/

typedef enum { UNBOUND, 
	       BINDING, 
	       BOUND, 
	       ENABLED, 
	       ACTIVE, 
	       UNBINDING 
} _udi_nic_state_t;

#endif	/* _UDI_NIC_H */
