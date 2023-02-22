/*
 * File: meta/network/udi_nicP.h
 *
 * UDI Network Interface Metalanguage Library private header
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

/* cb_group definitions */
#define	UDI_NIC_CTRL_CB_GROUP		1
#define	UDI_NIC_TX_CB_GROUP		2
#define	UDI_NIC_RX_CB_GROUP		3

/*
 * ---------------------------------------------------------------------------
 * Back-end stubs.
 * ---------------------------------------------------------------------------
 */

/* Function prototypes for unmarshalling ops. */
static udi_mei_direct_stub_t	udi_nd_bind_req_direct;
static udi_mei_backend_stub_t	udi_nd_bind_req_backend;
static udi_mei_direct_stub_t	udi_nsr_bind_ack_direct;
static udi_mei_backend_stub_t	udi_nsr_bind_ack_backend;
static udi_mei_direct_stub_t	udi_nd_unbind_req_direct;
static udi_mei_backend_stub_t	udi_nd_unbind_req_backend;
static udi_mei_direct_stub_t	udi_nsr_unbind_ack_direct;
static udi_mei_backend_stub_t	udi_nsr_unbind_ack_backend;
static udi_mei_direct_stub_t	udi_nd_enable_req_direct;
static udi_mei_backend_stub_t	udi_nd_enable_req_backend;
static udi_mei_direct_stub_t	udi_nsr_enable_ack_direct;
static udi_mei_backend_stub_t	udi_nsr_enable_ack_backend;
static udi_mei_direct_stub_t	udi_nd_disable_req_direct;
static udi_mei_backend_stub_t	udi_nd_disable_req_backend;
static udi_mei_direct_stub_t	udi_nd_ctrl_req_direct;
static udi_mei_backend_stub_t	udi_nd_ctrl_req_backend;
static udi_mei_direct_stub_t	udi_nsr_ctrl_ack_direct;
static udi_mei_backend_stub_t	udi_nsr_ctrl_ack_backend;
static udi_mei_direct_stub_t	udi_nsr_status_ind_direct;
static udi_mei_backend_stub_t	udi_nsr_status_ind_backend;
static udi_mei_direct_stub_t	udi_nd_info_req_direct;
static udi_mei_backend_stub_t	udi_nd_info_req_backend;
static udi_mei_direct_stub_t	udi_nsr_info_ack_direct;
static udi_mei_backend_stub_t	udi_nsr_info_ack_backend;
static udi_mei_direct_stub_t	udi_nsr_tx_rdy_direct;
static udi_mei_backend_stub_t	udi_nsr_tx_rdy_backend;
static udi_mei_direct_stub_t	udi_nd_tx_req_direct;
static udi_mei_backend_stub_t	udi_nd_tx_req_backend;
static udi_mei_direct_stub_t	udi_nd_exp_tx_req_direct;
static udi_mei_backend_stub_t	udi_nd_exp_tx_req_backend;
static udi_mei_direct_stub_t	udi_nsr_rx_ind_direct;
static udi_mei_backend_stub_t	udi_nsr_rx_ind_backend;
static udi_mei_direct_stub_t	udi_nsr_exp_rx_ind_direct;
static udi_mei_backend_stub_t	udi_nsr_exp_rx_ind_backend;
static udi_mei_direct_stub_t	udi_nd_rx_rdy_direct;
static udi_mei_backend_stub_t	udi_nd_rx_rdy_backend;

/*
 * ---------------------------------------------------------------------------
 * Op Templates and associated op_idx's.
 * ---------------------------------------------------------------------------
 */

/* ND Control op_idx's */
#define	UDI_NIC_BIND_REQ	1
#define	UDI_NIC_UNBIND_REQ	2
#define	UDI_NIC_ENABLE_REQ	3
#define	UDI_NIC_DISABLE_REQ	4
#define	UDI_NIC_CTRL_REQ	5
#define	UDI_NIC_INFO_REQ	6

/* ND Transmit op_idx's */
#define	UDI_NIC_TX_REQ		1
#define	UDI_NIC_EXP_TX_REQ	2

/* ND Receive op_idx's */
#define	UDI_NIC_RX_RDY		1

/*
 * CB layouts
 */

static udi_layout_t	udi_nic_bind_visible_layout[] = {
	UDI_DL_UBIT8_T,		/* media_type		*/
	UDI_DL_UBIT32_T,	/* min_pdu_size		*/
	UDI_DL_UBIT32_T,	/* max_pdu_size		*/
/* UDI_NIC_VERSION 0x101 	vvv*/
	UDI_DL_UBIT32_T,	/* capabilities		*/
	UDI_DL_UBIT8_T,		/* max_perfect_multicast*/
	UDI_DL_UBIT8_T,		/* max_total_multicast	*/
/* UDI_NIC_VERSION 0x101	^^^ */
	UDI_DL_UBIT32_T,	/* rx_hw_threshold	*/
	UDI_DL_UBIT8_T,		/* mac_addr_len		*/
	UDI_DL_ARRAY, UDI_NIC_MAC_ADDRESS_SIZE, UDI_DL_UBIT8_T, UDI_DL_END,
	UDI_DL_END
};

static udi_layout_t	udi_nic_bind_req_marshal_layout[] = {
	UDI_DL_INDEX_T,		/* tx_chan_index	*/
	UDI_DL_INDEX_T,		/* rx_chan_index	*/
	UDI_DL_END
};

static udi_layout_t	udi_nic_ctrl_visible_layout[] = {
	UDI_DL_UBIT8_T,		/* command		*/
	UDI_DL_UBIT32_T,	/* indicator		*/
	UDI_DL_BUF, 0, 0, 0,	/* data_buf (always preserved) */
	UDI_DL_END
};

static udi_layout_t	udi_nic_info_visible_layout[] = {
	UDI_DL_BOOLEAN_T,	/* interface_is_active 	*/
	UDI_DL_BOOLEAN_T,	/* link_is_active 	*/
	UDI_DL_BOOLEAN_T,	/* is_full_duplex 	*/
	UDI_DL_UBIT32_T,	/* link_mbps		*/
	UDI_DL_UBIT32_T,	/* link_bps		*/
	UDI_DL_UBIT32_T,	/* tx_packets		*/
	UDI_DL_UBIT32_T,	/* rx_packets		*/
	UDI_DL_UBIT32_T,	/* tx_errors		*/
	UDI_DL_UBIT32_T,	/* rx_errors		*/
	UDI_DL_UBIT32_T,	/* tx_discards		*/
	UDI_DL_UBIT32_T,	/* rx_discards		*/
	UDI_DL_UBIT32_T,	/* tx_underrun		*/
	UDI_DL_UBIT32_T,	/* rx_overrun		*/
	UDI_DL_UBIT32_T,	/* collisions		*/
	UDI_DL_END
};

static udi_layout_t	udi_nic_tx_req_visible_layout[] = {
	UDI_DL_CB,		/* chain		*/
	UDI_DL_BUF, 0, 0, 0,	/* tx_buf (always preserved) */
	UDI_DL_BOOLEAN_T,	/* completion_urgent	*/
	UDI_DL_END
};

static udi_layout_t	udi_nic_tx_rdy_visible_layout[] = {
	UDI_DL_CB,		/* chain		*/
	UDI_DL_BUF, 0, 0, 1,	/* tx_buf (never preserved) */
	UDI_DL_BOOLEAN_T,	/* completion_urgent	*/
	UDI_DL_END
};

static udi_layout_t	udi_nic_rx_ind_visible_layout[] = {
	UDI_DL_CB,		/* chain 		*/
	UDI_DL_BUF, 0, 0, 0,	/* rx_buf (always preserved) */
	UDI_DL_UBIT8_T,		/* rx_status		*/
	UDI_DL_UBIT8_T,		/* addr_match		*/
	UDI_DL_END
};

static udi_layout_t	udi_nic_rx_rdy_visible_layout[] = {
	UDI_DL_CB,		/* chain 		*/
	UDI_DL_BUF, 0, 0, 1,	/* rx_buf (never preserved) */
	UDI_DL_UBIT8_T,		/* rx_status		*/
	UDI_DL_UBIT8_T,		/* addr_match		*/
	UDI_DL_END
};

/* Marshalling layouts */
static udi_layout_t	udi_nd_info_req_marshal_layout[] = {
	UDI_DL_BOOLEAN_T,	/* reset_statistics 	*/
	UDI_DL_END
};

static udi_layout_t	udi_nic_empty_layout[] = {
	UDI_DL_END
};

/* ND ops templates */
static udi_mei_op_template_t	udi_nic_nd_ctrl_op_templates[] = {
	{ "udi_nd_bind_req", UDI_MEI_OPCAT_REQ,
		0, 	/* flags */
		UDI_NIC_BIND_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_nd_bind_req_direct, udi_nd_bind_req_backend,	
		udi_nic_bind_visible_layout,
		udi_nic_bind_req_marshal_layout },
	{ "udi_nd_unbind_req", UDI_MEI_OPCAT_REQ,
		0,	/* flags */
		UDI_NIC_STD_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_nd_unbind_req_direct, udi_nd_unbind_req_backend,	
		udi_nic_empty_layout, udi_nic_empty_layout },
	{ "udi_nd_enable_req", UDI_MEI_OPCAT_REQ,
		0,	/* flags */
		UDI_NIC_STD_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_nd_enable_req_direct, udi_nd_enable_req_backend,	
		udi_nic_empty_layout, udi_nic_empty_layout },
	{ "udi_nd_disable_req", UDI_MEI_OPCAT_REQ,
		0,	/* flags */
		UDI_NIC_STD_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_nd_disable_req_direct, udi_nd_disable_req_backend,	
		udi_nic_empty_layout, udi_nic_empty_layout },
	{ "udi_nd_ctrl_req", UDI_MEI_OPCAT_REQ,
		0,	/* flags */
		UDI_NIC_CTRL_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_nd_ctrl_req_direct, udi_nd_ctrl_req_backend,	
		udi_nic_ctrl_visible_layout, udi_nic_empty_layout },
	{ "udi_nd_info_req", UDI_MEI_OPCAT_REQ,
		0,	/* flags */
		UDI_NIC_INFO_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_nd_info_req_direct, udi_nd_info_req_backend,	
		udi_nic_info_visible_layout, udi_nd_info_req_marshal_layout },
	{ NULL }
};

static udi_mei_op_template_t	udi_nic_nd_tx_op_templates[] = {
	{ "udi_nd_tx_req", UDI_MEI_OPCAT_REQ,
		0,	/* flags */
		UDI_NIC_TX_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_nd_tx_req_direct, udi_nd_tx_req_backend,	
		udi_nic_tx_req_visible_layout, udi_nic_empty_layout },
	{ "udi_nd_exp_tx_req", UDI_MEI_OPCAT_REQ,
		0,	/* flags */
		UDI_NIC_TX_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_nd_exp_tx_req_direct, udi_nd_exp_tx_req_backend,	
		udi_nic_tx_req_visible_layout, udi_nic_empty_layout },
	{ NULL }
};

static udi_mei_op_template_t	udi_nic_nd_rx_op_templates[] = {
	{ "udi_nd_rx_rdy", UDI_MEI_OPCAT_RES,
		0,	/* flags */
		UDI_NIC_RX_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_nd_rx_rdy_direct, udi_nd_rx_rdy_backend,	
		udi_nic_rx_rdy_visible_layout, udi_nic_empty_layout },
	{ NULL }
};

/* NSR Control op_idx's */
#define	UDI_NIC_BIND_ACK	1
#define	UDI_NIC_UNBIND_ACK	2
#define	UDI_NIC_ENABLE_ACK	3
#define	UDI_NIC_CTRL_ACK	4
#define	UDI_NIC_INFO_ACK	5
#define	UDI_NIC_STATUS_IND	6

/* NSR Transmit op_idx's */
#define	UDI_NIC_TX_RDY		1

/* NSR Receive op_idx's */
#define	UDI_NIC_RX_IND		1
#define	UDI_NIC_EXP_RX_IND	2

/* NSR CB layouts */

static udi_layout_t	udi_nic_status_visible_layout[] = {
	UDI_DL_UBIT8_T,		/* event		*/
	UDI_DL_END
};

/* NSR marshalling layouts */
static udi_layout_t	udi_nsr_bind_ack_marshal_layout[] = {
	UDI_DL_STATUS_T,	/* status		*/
	UDI_DL_END
};

/* NSR ops templates */
static udi_mei_op_template_t	udi_nic_nsr_ctrl_op_templates[] = {
	{ "udi_nsr_bind_ack", UDI_MEI_OPCAT_ACK,
		0,	/* flags */
		UDI_NIC_BIND_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_nsr_bind_ack_direct, udi_nsr_bind_ack_backend, 
		udi_nic_bind_visible_layout,
		udi_nsr_bind_ack_marshal_layout },
	{ "udi_nsr_unbind_ack", UDI_MEI_OPCAT_ACK,
		0,	/* flags */
		UDI_NIC_STD_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_nsr_unbind_ack_direct, udi_nsr_unbind_ack_backend, 
		udi_nic_bind_visible_layout,
		udi_nsr_bind_ack_marshal_layout },
	{ "udi_nsr_enable_ack", UDI_MEI_OPCAT_ACK,
		0,	/* flags */
		UDI_NIC_STD_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_nsr_enable_ack_direct, udi_nsr_enable_ack_backend, 
		udi_nic_empty_layout, udi_nsr_bind_ack_marshal_layout },
	{ "udi_nsr_ctrl_ack", UDI_MEI_OPCAT_ACK,
		0,	/* flags */
		UDI_NIC_CTRL_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_nsr_ctrl_ack_direct, udi_nsr_ctrl_ack_backend, 
		udi_nic_ctrl_visible_layout, udi_nsr_bind_ack_marshal_layout },
	{ "udi_nsr_info_ack", UDI_MEI_OPCAT_ACK,
		0,	/* flags */
		UDI_NIC_INFO_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_nsr_info_ack_direct, udi_nsr_info_ack_backend, 
		udi_nic_info_visible_layout, udi_nic_empty_layout },
	{ "udi_nsr_status_ind", UDI_MEI_OPCAT_IND,
		0,	/* flags */
		UDI_NIC_STATUS_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_nsr_status_ind_direct, udi_nsr_status_ind_backend, 
		udi_nic_status_visible_layout,
		udi_nic_empty_layout },
	{
		NULL
	}
};

static udi_mei_op_template_t	udi_nic_nsr_tx_op_templates[] = {
	{ "udi_nsr_tx_rdy", UDI_MEI_OPCAT_RES,
		0,	/* flags */
		UDI_NIC_TX_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_nsr_tx_rdy_direct, udi_nsr_tx_rdy_backend, 
		udi_nic_tx_rdy_visible_layout, udi_nic_empty_layout },
	{
		NULL
	}
};

static udi_mei_op_template_t	udi_nic_nsr_rx_op_templates[] = {
	{ "udi_nsr_rx_ind", UDI_MEI_OPCAT_IND,
		0,	/* flags */
		UDI_NIC_RX_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_nsr_rx_ind_direct, udi_nsr_rx_ind_backend, 
		udi_nic_rx_ind_visible_layout,
		udi_nic_empty_layout },
	{ "udi_nsr_exp_rx_ind", UDI_MEI_OPCAT_IND,
		0,	/* flags */
		UDI_NIC_RX_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_nsr_exp_rx_ind_direct, udi_nsr_exp_rx_ind_backend, 
		udi_nic_rx_ind_visible_layout, udi_nic_empty_layout },
	{
		NULL
	}
};

