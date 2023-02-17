
/*
 * File: mapper/common/net/netmap.c
 * 
 * This is the NSR mapper. It provides the NSR end of
 * communication for a ND.
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
 * ===========================================================================
 *
 *	This file is #included by the OS-specific mapper code 
 *	The following interfaces are 'exported' to the OS-specific version:
 *
 * void NSR_enable(udi_cb_t *gcb, void (*cbfn)(udi_cb_t *, udi_status_t))
 * void NSR_disable(udi_cb_t *gcb, void (*cbfn)(udi_cb_t *, udi_status_t))
 * void NSR_return_packet(udi_nic_rx_cb_t *cb)
 * void NSR_ctrl_req(udi_nic_ctrl_cb_t *cb)
 * nsrmap_Tx_region_data_t *NSR_txdata(nsrmap_region_data_t *rdata)
 *
 *	The following need to be provided by the OS-specific portion:
 *
 * void osnsr_init(nsrmap_region_data_t *)
 * void osnsr_bind_done(udi_channel_event_cb_t *, udi_status_t)
 * void osnsr_unbind_done(udi_nic_cb_t *)
 * void osnsr_status_ind(udi_nic_status_cb_t *)
 * void osnsr_ctrl_ack(udi_nic_ctrl_cb_t *, udi_status_t)
 * void osnsr_rx_packet(udi_nic_rx_cb_t *, udi_boolean_t)
 * void osnsr_tx_rdy(nsrmap_Tx_region_data_t *)
 * void osnsr_final_cleanup(udi_mgmt_cb_t *)
 * udi_ubit32_t osnsr_max_xfer_size(udi_ubit8_t)
 * udi_ubit32_t osnsr_min_xfer_size(udi_ubit8_t)
 * udi_ubit32_t osnsr_mac_addr_size(udi_ubit8_t)
 *
 *
 * This mapper is split into 2 regions. The primary controls all link 
 * establishment, control requests to/from the ND and Rx operations.
 * The secondary region controls Tx operations. This allows full-use
 * to be made of any MP parallelism available from the environment.
 * NOTE: It is assumed that the number of control requests made to the ND will
 *	 be a small proportion of the overall number of Rx and Tx requests.
 * ===========================================================================
 */

#ifndef STATIC
#define	STATIC	static
#endif

/* ========================================================================== */

/* 			udiprops provided definitions			      */

/* ========================================================================== */

/*
 * Ops_idx values
 */
#ifndef NSRMAP_CTRL_IDX
#error	"NSRMAP_CTRL_IDX not defined"
#define	NSRMAP_CTRL_IDX		1	/* ops_idx for control ops */
#endif /* NSRMAP_CTRL_IDX */
#ifndef NSRMAP_TX_IDX
#error	"NSRMAP_TX_IDX not defined"
#define	NSRMAP_TX_IDX		2	/* ops_idx for transmit ops */
#endif /* NSRMAP_TX_IDX */
#ifndef NSRMAP_RX_IDX
#error "NSRMAP_RX_IDX not defined"
#define	NSRMAP_RX_IDX		3	/* ops_idx for receive ops */
#endif /* NSRMAP_RX_IDX */
#ifndef NSRMAP_GIO_TX_CLIENT_IDX
#error "NSRMAP_GIO_TX_CLIENT_IDX not defined"
#define	NSRMAP_GIO_TX_CLIENT_IDX 4	/* GIO client ops_idx (primary region) */
#endif /* NSRMAP_GIO_TX_CLIENT_IDX */
#ifndef NSRMAP_GIO_TX_PROV_IDX
#error "NSRMAP_GIO_TX_PROV_IDX not defined"
#define NSRMAP_GIO_TX_PROV_IDX	5	/* GIO provider ops_idx (Tx region) */
#endif /* NSRMAP_GIO_TX_PROV_IDX */

/*
 * Regions
 */
#ifndef NSRMAP_TX_REG_IDX
#error "NSRMAP_TX_REG_IDX not defined"
#define	NSRMAP_TX_REG_IDX	1	/* Tx region index      */
#endif /* NSRMAP_TX_REG_IDX */

/*
 * Metas
 */
#ifndef NSRMAP_NIC_META
#error	"NSRMAP_NIC_META not defined"
#define	NSRMAP_NIC_META		1
#endif /* NSRMAP_NIC_META */
#ifndef NSRMAP_GIO_META
#error	"NSRMAP_GIO_META not defined"
#define	NSRMAP_GIO_META		2
#endif /* NSRMAP_GIO_META */

/*
 * Default settings for deferred op handling
 */
#ifndef	NSR_DEFER_SECS
#define	NSR_DEFER_SECS	5		/* 5 second delay       */
#endif /* NSR_DEFER_SECS */
#ifndef	NSR_DEFER_NSECS
#define	NSR_DEFER_NSECS	0		/* 0 nanoseconds        */
#endif /* NSR_DEFER_NSECS */

/*
 * Default LINK-UP detection timeout
 */
#ifndef NSR_LINKUP_SECS
#define	NSR_LINKUP_SECS	2		/* 2 second delay       */
#endif /* NSR_LINKUP_SECS */
#ifndef NSR_LINKUP_NSECS
#define	NSR_LINKUP_NSECS 0		/* 0 nanoseconds        */
#endif /* NSR_LINKUP_NSECS */

/*
 * Function declarations
 */

static char *state_names[] = {
	"UNBOUND",
	"BINDING",
	"BOUND",
	"ENABLED",
	"ACTIVE",
	"UNBINDING",
	"ILLEGAL"
};

/* Management Meta */
STATIC udi_devmgmt_req_op_t nsrmap_devmgmt_req;
STATIC udi_usage_ind_op_t nsrmap_usage_ind;
STATIC udi_mem_alloc_call_t nsrmap_gio_cmd_alloc;
STATIC udi_buf_path_alloc_call_t nsr_buf_path_alloc_cbfn;
STATIC udi_final_cleanup_req_op_t nsrmap_final_cleanup_req;

/* Network Control Meta */
STATIC udi_channel_event_ind_op_t nsrmap_channel_event;
STATIC udi_nsr_bind_ack_op_t nsrmap_bind_ack;
STATIC udi_nsr_unbind_ack_op_t nsrmap_unbind_ack;
STATIC udi_nsr_enable_ack_op_t nsrmap_enable_ack;
STATIC udi_nsr_ctrl_ack_op_t nsrmap_ctrl_ack;
STATIC udi_nsr_info_ack_op_t nsrmap_info_ack;
STATIC udi_nsr_status_ind_op_t nsrmap_status_ind;

/* NSR Transmit Meta */
STATIC udi_channel_event_ind_op_t nsrmap_tx_channel_event;
STATIC udi_nsr_tx_rdy_op_t nsrmap_tx_rdy;

/* NSR Receive Meta */
STATIC udi_channel_event_ind_op_t nsrmap_rx_channel_event;
STATIC void nsrmap_rx_ind_common(udi_nic_rx_cb_t *,
				 udi_boolean_t);
STATIC udi_nsr_rx_ind_op_t nsrmap_rx_ind;
STATIC udi_nsr_exp_rx_ind_op_t nsrmap_exp_rx_ind;

/* GIO Provider Meta (Tx region) */
STATIC udi_channel_event_ind_op_t nsrmap_gio_Tx_prov_channel_event;
STATIC udi_gio_bind_req_op_t nsrmap_gio_Tx_bind_req;
STATIC udi_gio_unbind_req_op_t nsrmap_gio_Tx_unbind_req;
STATIC udi_gio_xfer_req_op_t nsrmap_gio_Tx_xfer_req;
STATIC udi_gio_event_res_op_t nsrmap_gio_Tx_event_res;

/* GIO Client Meta (NSR primary region) Transmit */
STATIC udi_channel_event_ind_op_t nsrmap_gio_Tx_client_channel_event;
STATIC udi_gio_bind_ack_op_t nsrmap_gio_bind_ack;
STATIC udi_gio_unbind_ack_op_t nsrmap_gio_unbind_ack;
STATIC udi_gio_xfer_ack_op_t nsrmap_gio_xfer_ack;
STATIC udi_gio_xfer_nak_op_t nsrmap_gio_xfer_nak;
STATIC udi_gio_event_ind_op_t nsrmap_gio_event_ind;

/*
 * Support routines
 */
STATIC udi_channel_spawn_call_t nsrmap_tx_spawn_done;
STATIC udi_channel_spawn_call_t nsrmap_rx_spawn_done;
STATIC udi_cb_alloc_call_t nsrmap_bind_parent_cbfn;
STATIC udi_cb_alloc_call_t nsrmap_linkup_cballoc;
STATIC udi_cb_alloc_call_t nsrmap_rx_alloc_cbfn;
STATIC udi_cb_alloc_call_t NSR_enable_alloc;
STATIC void nsrmap_commit_cbs(nsrmap_region_data_t *);
STATIC udi_cb_alloc_call_t NSR_disable_alloc;
STATIC udi_mem_alloc_call_t nsrmap_got_txmem;
STATIC udi_mem_alloc_call_t nsrmap_got_rxmem;
STATIC udi_cb_alloc_batch_call_t nsrmap_got_cb_batch;

STATIC udi_cb_alloc_call_t nsr_gio_chan_cbfn;
STATIC udi_cb_alloc_call_t nsr_gio_bind_cbfn;
STATIC udi_buf_write_call_t nsr_gio_buf_cbfn;

#if 0
STATIC udi_buf_write_call_t nsr_gio_tx_cmd_cbfn;
#endif

STATIC void nsr_change_state(nsrmap_region_data_t *,
			     _udi_nic_state_t);
STATIC void nsr_set_tracemask(nsrmap_region_data_t *,
			      udi_ubit32_t,
			      udi_ubit32_t);

#if 0
STATIC void nsr_alloc_Rx_resources(nsrmap_region_data_t *,
				   udi_ubit32_t,
				   udi_ubit32_t);
#endif
STATIC void nsr_alloc_Tx_resources(nsrmap_region_data_t *,
				   udi_ubit32_t,
				   udi_ubit32_t);
STATIC void nsr_release_resources(nsrmap_region_data_t *);
STATIC void nsr_commit_Rx_cbs(nsrmap_region_data_t *);
STATIC void nsr_commit_Tx_cbs(nsrmap_region_data_t *);
STATIC void nsr_commit_tx_cbs(nsrmap_Tx_region_data_t *);
STATIC void nsr_send_region(nsrmap_region_data_t *,
			    udi_ubit32_t,
			    void (*)(udi_cb_t *),
			    nsrmap_gio_t *);
STATIC udi_channel_anchor_call_t nsrmap_channel_anchored;
STATIC udi_cb_alloc_call_t nsrmap_reg_cb_alloc;
STATIC udi_buf_path_alloc_call_t nsrmap_Tx_buf_path_cbfn;
STATIC void nsr_recycle_Rx_q(nsrmap_region_data_t *);

STATIC void nsrmap_unbind_req(udi_cb_t *);
STATIC udi_mem_alloc_call_t nsrmap_defer_unbind_op;
STATIC udi_timer_expired_call_t nsrmap_defer_timeout;
STATIC udi_cb_alloc_call_t nsrmap_defer_cballoc;
STATIC void nsrmap_start_defer_chain(udi_cb_t *);

STATIC udi_mem_alloc_call_t nsrmap_defer_txrdy_op;
STATIC void nsrmap_start_defer_Tx_chain(udi_cb_t *);
STATIC udi_cb_alloc_call_t nsrmap_defer_Tx_cballoc;
STATIC udi_timer_expired_call_t nsrmap_defer_Tx_timeout;

STATIC udi_timer_expired_call_t nsrmap_fake_linkup;

/*
 * Management Metalanguage/NSR Entrypoints
 */
static udi_mgmt_ops_t nsrmap_mgmt_ops = {
	nsrmap_usage_ind,		/* usage_ind_op */
	udi_enumerate_no_children,	/* enumerate_req_op      */
	nsrmap_devmgmt_req,		/* devmgmt_req_op        */
	nsrmap_final_cleanup_req	/* final_cleanup_req_op  */
};

/*
 * Network Meta/NSR Control operation entrypoints
 */
static udi_nsr_ctrl_ops_t nsrmap_ctrl_ops = {
	nsrmap_channel_event,
	nsrmap_bind_ack,
	nsrmap_unbind_ack,
	nsrmap_enable_ack,
	nsrmap_ctrl_ack,
	nsrmap_info_ack,
	nsrmap_status_ind
};

/*
 * Network Meta/NSR Transmit operation entrypoints
 */
static udi_nsr_tx_ops_t nsrmap_tx_ops = {
	nsrmap_tx_channel_event,
	nsrmap_tx_rdy
};

/*
 * Network Meta/NSR Receive operation entrypoints
 */
static udi_nsr_rx_ops_t nsrmap_rx_ops = {
	nsrmap_rx_channel_event,
	nsrmap_rx_ind,
	nsrmap_exp_rx_ind
};

static udi_gio_provider_ops_t nsrmap_gio_Tx_prov_ops = {
	nsrmap_gio_Tx_prov_channel_event,
	nsrmap_gio_Tx_bind_req,
	nsrmap_gio_Tx_unbind_req,
	nsrmap_gio_Tx_xfer_req,
	nsrmap_gio_Tx_event_res
};

static udi_gio_client_ops_t nsrmap_gio_Tx_client_ops = {
	nsrmap_gio_Tx_client_channel_event,
	nsrmap_gio_bind_ack,
	nsrmap_gio_unbind_ack,
	nsrmap_gio_xfer_ack,
	nsrmap_gio_xfer_nak,
	nsrmap_gio_event_ind
};

/*
 * default set of op_flags
 */
static udi_ubit8_t nsrmap_default_op_flags[] = {
	0, 0, 0, 0, 0, 0
};

static udi_primary_init_t nsr_primary_init = {
	&nsrmap_mgmt_ops,
	nsrmap_default_op_flags,	/* op_flags */
	0,				/* mgmt scratch */
	0,				/* enum list length */
	sizeof (nsrmap_region_data_t),	/* rdata size */
	0,				/* child data size */
	2,				/* buf path [should this be 2 ??] */
};

static udi_secondary_init_t nsr_secondary_init_list[] = {
	{
	 NSRMAP_TX_REG_IDX,		/* Tx region index */
	 sizeof (nsrmap_Tx_region_data_t)
	 }
	,
	{
	 0				/* Terminator */
	 }
};

static udi_ops_init_t nsr_ops_init_list[] = {
	{
	 NSRMAP_CTRL_IDX,
	 NSRMAP_NIC_META,
	 UDI_NSR_CTRL_OPS_NUM,
	 0,				/* chan_context size */
	 (udi_ops_vector_t *)&nsrmap_ctrl_ops,
	 nsrmap_default_op_flags	/* op_flags */
	 },

	{
	 NSRMAP_TX_IDX,
	 NSRMAP_NIC_META,
	 UDI_NSR_TX_OPS_NUM,
	 0,				/* chan_context size */
	 (udi_ops_vector_t *)&nsrmap_tx_ops,
	 nsrmap_default_op_flags	/* op_flags */
	 },

	{
	 NSRMAP_RX_IDX,
	 NSRMAP_NIC_META,
	 UDI_NSR_RX_OPS_NUM,
	 0,				/* chan_context size */
	 (udi_ops_vector_t *)&nsrmap_rx_ops,
	 nsrmap_default_op_flags	/* op_flags */
	 },

	{
	 NSRMAP_GIO_TX_PROV_IDX,
	 NSRMAP_GIO_META,
	 UDI_GIO_PROVIDER_OPS_NUM,
	 0,				/* chan_context size */
	 (udi_ops_vector_t *)&nsrmap_gio_Tx_prov_ops,
	 nsrmap_default_op_flags	/* op_flags */
	 },

	{
	 NSRMAP_GIO_TX_CLIENT_IDX,
	 NSRMAP_GIO_META,
	 UDI_GIO_CLIENT_OPS_NUM,
	 0,				/* chan_context size */
	 (udi_ops_vector_t *)&nsrmap_gio_Tx_client_ops,
	 nsrmap_default_op_flags	/* op_flags */
	 },

	{
	 0				/* Terminator */
	 }
};

/*
 * -----------------------------------------------------------------------------
 * Meta CB initialisation structures:
 * -----------------------------------------------------------------------------
 */

/*
 * layout specifier for passing a udi_channel_t across regions. Used with
 * udi_gio_xfer_req between primary and secondary (transmit) region
 */
static udi_layout_t nsr_gio_chan_layout[] = { UDI_DL_CHANNEL_T, UDI_DL_END };

static udi_cb_init_t nsr_cb_init_list[] = {
	{
	 NSRMAP_NET_CB_IDX,
	 NSRMAP_NIC_META,
	 UDI_NIC_STD_CB_NUM,
	 0, 0, NULL			/* scratch + inline */
	 },
	{
	 NSRMAP_BIND_CB_IDX,
	 NSRMAP_NIC_META,
	 UDI_NIC_BIND_CB_NUM,
	 0, 0, NULL			/* scratch + inline */
	 },
	{
	 NSRMAP_CTRL_CB_IDX,
	 NSRMAP_NIC_META,
	 UDI_NIC_CTRL_CB_NUM,
	 0, 0, NULL			/* scratch + inline */
	 },
	{
	 NSRMAP_STATUS_CB_IDX,
	 NSRMAP_NIC_META,
	 UDI_NIC_STATUS_CB_NUM,
	 0, 0, NULL			/* scratch + inline */
	 },
	{
	 NSRMAP_INFO_CB_IDX,
	 NSRMAP_NIC_META,
	 UDI_NIC_INFO_CB_NUM,
	 0, 0, NULL			/* scratch + inline */
	 },
	{
	 NSRMAP_TX_CB_IDX,
	 NSRMAP_NIC_META,
	 UDI_NIC_TX_CB_NUM,
	 sizeof (nsrmap_scratch_data_t), 0, NULL	/* scratch + inline */
	 }
	,
	{
	 NSRMAP_RX_CB_IDX,
	 NSRMAP_NIC_META,
	 UDI_NIC_RX_CB_NUM,
	 sizeof (nsr_cb_t *), 0, NULL	/* scratch + inline */
	 }
	,

	/*
	 * GIO Control Block specifications
	 */
	{
	 NSRMAP_GIO_BIND_CB_IDX,
	 NSRMAP_GIO_META,
	 UDI_GIO_BIND_CB_NUM,
	 0, 0, NULL			/* scratch + inline */
	 }
	,
	{
	 NSRMAP_GIO_XFER_CB_IDX,
	 NSRMAP_GIO_META,
	 UDI_GIO_XFER_CB_NUM,
	 0,				/* scratch */
	 sizeof (_nsrmap_chan_param_t),	/* inline size */
	 nsr_gio_chan_layout		/* inline layout */
	 }
	,
	{
	 NSRMAP_GIO_EVENT_CB_IDX,
	 NSRMAP_GIO_META,
	 UDI_GIO_EVENT_CB_NUM,
	 0, 0, NULL			/* scratch + inline */
	 }
	,
	{
	 0				/* Terminator */
	 }
};

#ifndef HAVE_GCB_LIST
static udi_gcb_init_t nsr_gcb_init_list[] = { {0, 0} };
#endif
static udi_cb_select_t nsr_cb_select_list[] = { {0, 0} };

udi_init_t udi_init_info = {
	&nsr_primary_init,
	nsr_secondary_init_list,
	nsr_ops_init_list,
	nsr_cb_init_list,
	nsr_gcb_init_list,
	nsr_cb_select_list
};

/*
 * -----------------------------------------------------------------------------
 * Tracing / Logging support routines
 * -----------------------------------------------------------------------------
 */
#ifdef NSR_TRACE
STATIC char nsr_trace_buffer[80];
#endif


STATIC void
nsrmap_usage_ind(udi_usage_cb_t *cb,
		 udi_ubit8_t resource_level)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;

	DBG_MGMT1(("nsrmap_usage_ind: context = %p\n", rdata));
	if (rdata->rdatap != (void *)rdata) {
		/*
		 * Initialise our region-local data. The area will have been
		 * zero'd by the region_init code. We just need to initialise
		 * queues and any synchronization variables we need
		 */

		NSR_UDI_QUEUE_INIT(&rdata->rxcbq);
		NSR_UDI_QUEUE_INIT(&rdata->rxonq);
		NSR_UDI_QUEUE_INIT(&rdata->deferq);

		/*
		 * Set initial state 
		 */
		rdata->State = UNBOUND;

		/*
		 * Initialise OS-dependent structure members 
		 */
		osnsr_init(rdata);

		rdata->rdatap = (void *)rdata;

		/*
		 * Allocate buf_path for inter-region buffer allocation 
		 */
		udi_buf_path_alloc(nsr_buf_path_alloc_cbfn, UDI_GCB(cb));
		return;
	}

	/*
	 * Set the trace-mask for the given Meta 
	 */
	rdata->ml_tracemask[cb->meta_idx] = cb->trace_mask;

	/*
	 * Pass the new tracemask to our secondary regions
	 */
	nsr_set_tracemask(rdata, cb->meta_idx, cb->trace_mask);

	udi_usage_res(cb);
}

/*
 * nsr_buf_path_alloc_cbfn:
 * -----------------------
 * Callback for buf_path allocation which is to be used for inter-region buffer
 * allocations. Initiated by the usage_ind handler.
 */
STATIC void
nsr_buf_path_alloc_cbfn(udi_cb_t *gcb,
			udi_buf_path_t new_buf_path)
{

	nsrmap_region_data_t *rdata = gcb->context;

	DBG_MGMT2(("nsr_buf_path_alloc_cbfn(%p, %p)\n", gcb, new_buf_path));
	rdata->buf_path = new_buf_path;

	/*
	 * Allocate inter-region command data buffer -MOVABLE MEMORY- 
	 */
	udi_mem_alloc(nsrmap_gio_cmd_alloc, gcb,
		      sizeof (nsrmap_gio_t), UDI_MEM_MOVABLE);
}

/*
 * nsrmap_gio_cmd_alloc:
 * --------------------
 * Callback for rdata->gio_cmd allocation from usage_ind(). We have now got
 * some movable memory which can be used on our inter-region operations.
 * Just complete the usage_ind processing as necessary.
 */
STATIC void
nsrmap_gio_cmd_alloc(udi_cb_t *gcb,
		     void *new_mem)
{
	nsrmap_region_data_t *rdata = gcb->context;
	udi_usage_cb_t *cb = UDI_MCB(gcb, udi_usage_cb_t);

	DBG_MGMT2(("nsrmap_gio_cmd_alloc: gio_cmd @ %p\n", new_mem));

	rdata->gio_cmdp = new_mem;
	rdata->ml_tracemask[cb->meta_idx] = cb->trace_mask;

	nsr_set_tracemask(rdata, cb->meta_idx, cb->trace_mask);

	udi_usage_res(cb);
}

STATIC void
nsrmap_devmgmt_req(udi_mgmt_cb_t *cb,
		   udi_ubit8_t mgmt_op,
		   udi_ubit8_t parent_id)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;
	udi_ubit8_t flags = 0;

	DBG_MGMT1(("nsrmap_devmgmt_req( %p, %2x %2x )\n", cb, mgmt_op,
		   parent_id));
	switch (mgmt_op) {
	case UDI_DMGMT_PREPARE_TO_SUSPEND:
	case UDI_DMGMT_SUSPEND:
		flags = UDI_DMGMT_NONTRANSPARENT;
		break;
	case UDI_DMGMT_UNBIND:
		DBG_MGMT3(("UDI_DMGMT_UNBIND\n"));
		/*
		 * Unbind from parent 
		 */

		rdata->scratch_cb = UDI_GCB(cb);
		if (BTST(rdata->NSR_Rx_state, NsrmapS_AllocPending)) {
			/*
			 * Need to defer the unbind until all allocations have
			 * been completed
			 */
			DBG_MGMT2(("Deferring UNBIND\n"));
			udi_mem_alloc(nsrmap_defer_unbind_op, UDI_GCB(cb),
				      sizeof (_nsr_defer_op_t), 0);
		} else {

			/*
			 * Propagate state change to secondary regions 
			 */
			nsr_change_state(rdata, UNBINDING);

			udi_nd_unbind_req(UDI_MCB
					  (rdata->bind_cb, udi_nic_cb_t));

			/*
			 * Completion handled in udi_nic_unbind_ack... 
			 */
		}
		return;
	default:
		break;
	}

	udi_devmgmt_ack(cb, flags, UDI_OK);

}

/*
 * nsrmap_defer_unbind_op:
 * ----------------------
 * Enqueue an operation which will be scheduled later on. Simply allocate space
 * for an _nsr_defer_op_t and fill it in.
 */
STATIC void
nsrmap_defer_unbind_op(udi_cb_t *gcb,
		       void *new_mem)
{
	nsrmap_region_data_t *rdata = gcb->context;
	_nsr_defer_op_t *op = (_nsr_defer_op_t *) new_mem;

	if (!BTST(rdata->NSR_Rx_state, NsrmapS_AllocPending)) {
		/*
		 * We've already completed the allocation chain so there will
		 * be no-one to run us. We simply call the nsrmap_unbind_req
		 * instead
		 */
		DBG_MGMT3(("nsrmap_defer_unbind_op: Allocation already done\n"));
		udi_mem_free(new_mem);
		nsrmap_unbind_req(gcb);
	} else {
		op->func = nsrmap_unbind_req;
		op->cb = gcb;

		DBG_MGMT3(("nsrmap_defer_unbind_op: Deferring unbind_req\n"));
		NSR_UDI_ENQUEUE_TAIL(&rdata->deferq, &op->Q);

		/*
		 * Start a deferred callback chain to process any deferq
		 * entries. This will be self-perpetuating until we transition
		 * to the UNBOUND state.
		 */
		nsrmap_start_defer_chain(gcb);
	}
}

/*
 * nsrmap_start_defer_chain:
 * ------------------------
 * Start a timeout chain to check on the NsrmapS_AllocPending state of the
 * NSR. When the allocation completes this is cleared and we can call the
 * queued operation. The callback is responsible for rescheduling itself while
 * the NSR is not in the UNBOUND state.
 */
STATIC void
nsrmap_start_defer_chain(udi_cb_t *gcb)
{
	nsrmap_region_data_t *rdata = gcb->context;

	if (BTST(rdata->NSR_state, NsrmapS_TimerActive)) {
		/*
		 * Nothing to do, the chain is already running 
		 */
		return;
	} else {
		/*
		 * Allocate a timer_cb for the udi_timer_start() 
		 */
		udi_cb_alloc(nsrmap_defer_cballoc, gcb, NSRMAP_NET_CB_IDX,
			     gcb->channel);
	}
}

/*
 * nsrmap_defer_cballoc:
 * --------------------
 * Callback for the timer_cb which is allocated for use by nsrmap_defer_timeout
 */
STATIC void
nsrmap_defer_cballoc(udi_cb_t *gcb,
		     udi_cb_t *new_cb)
{
	nsrmap_region_data_t *rdata = gcb->context;
	udi_time_t tval;

	rdata->timer_cb = new_cb;
	BSET(rdata->NSR_state, NsrmapS_TimerActive);

	tval.seconds = NSR_DEFER_SECS;
	tval.nanoseconds = NSR_DEFER_NSECS;

	udi_timer_start(nsrmap_defer_timeout, new_cb, tval);
}

/*
 * nsrmap_defer_timeout:
 * --------------------
 * Timeout routine for handling deferred operations. Any queued operations can
 * be scheduled as soon as the AllocPending state is cleared. If we are still
 * blocked by an allocation thread [NsrmapS_AllocPending set] we queue another
 * timeout for NSR_DEFER_SECS:NSR_DEFER_NSECS time.
 * Also, if there is more than one deferred operation queued, we keep the
 * timeout chain running until the queue is emptied.
 * Once we've stopped the chain we free all of the resources.
 */
STATIC void
nsrmap_defer_timeout(udi_cb_t *gcb)
{
	nsrmap_region_data_t *rdata = gcb->context;
	udi_time_t tval;
	_nsr_defer_op_t *op;
	void (*func) (udi_cb_t *);
	udi_cb_t *cb;
	udi_boolean_t restart = TRUE;

	if (!BTST(rdata->NSR_Rx_state, NsrmapS_AllocPending)) {
		op = (_nsr_defer_op_t *) NSR_UDI_DEQUEUE_HEAD(&rdata->deferq);
		if (op) {
			func = op->func;
			cb = op->cb;
			udi_mem_free(op);
			if (NSR_UDI_QUEUE_EMPTY(&rdata->deferq)) {
				restart = FALSE;
			}

			/*
			 * Call deferred op 
			 */
			(*func) (cb);
		} else {
			restart = FALSE;
		}
	}
	if (restart) {
		tval.seconds = NSR_DEFER_SECS;
		tval.nanoseconds = NSR_DEFER_NSECS;

		udi_timer_start(nsrmap_defer_timeout, gcb, tval);
	} else {
		/*
		 * Clear up timer resources
		 */
		BCLR(rdata->NSR_state, NsrmapS_TimerActive);
		udi_cb_free(gcb);
		rdata->timer_cb = NULL;
	}
}

/*
 * nsrmap_unbind_req:
 * -----------------
 * Deferred execution of a DMGMT_UNBIND request. Issues the udi_nd_unbind_req
 * to the ND to complete the devmgmt_req processing.
 */
STATIC void
nsrmap_unbind_req(udi_cb_t *gcb)
{
	nsrmap_region_data_t *rdata = gcb->context;

	DBG_MGMT2(("nsrmap_unbind_req( %p )\nRESUMING UNBIND\n", gcb));

	rdata->scratch_cb = gcb;	/* Originating mgmt_cb_t */

	/*
	 * Propagate state change to secondary regions 
	 */
	nsr_change_state(rdata, UNBINDING);

	udi_nd_unbind_req(UDI_MCB(rdata->bind_cb, udi_nic_cb_t));
}

/*
 * nsrmap_final_cleanup_req:
 * ------------------------
 * Release any allocated memory and locks/events from the region local data
 * Callout to the OS specific cleanup code to clear any OS-specific data.
 */
STATIC void
nsrmap_final_cleanup_req(udi_mgmt_cb_t *cb)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;

	/*
	 * Release any mutex's associated with our internal queues
	 */
	NSR_UDI_QUEUE_DEINIT(&rdata->rxcbq);
	NSR_UDI_QUEUE_DEINIT(&rdata->rxonq);
	NSR_UDI_QUEUE_DEINIT(&rdata->deferq);

	osnsr_final_cleanup(cb);

	/*
	 * Release our udi_mem_alloc'd data
	 */
	udi_mem_free(rdata->gio_cmdp);

	/*
	 * Release the inter-region udi_buf_path
	 */
	udi_buf_path_free(rdata->buf_path);

	rdata->fcr_cb = cb;

	/*
	 * If the internal GIO channel has already been torn down, 
	 * acknowledge the FCR now.   Otherwise that code, upon channel
	 * close, will issue the FCR for us.
	 */
	if (rdata->tx_event_cb == NULL) {
		udi_final_cleanup_ack(cb);
	}
}

/* ========================================================================== */

/* 				Network Control Meta 			      */

/* ========================================================================== */
STATIC void
nsrmap_channel_event(udi_channel_event_cb_t *cb)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;
	udi_boolean_t complete = TRUE;
	udi_channel_t l_chan = UDI_NULL_CHANNEL;
	udi_boolean_t close_channel = FALSE;

	if (rdata->ml_tracemask[1] & UDI_TREVENT_LOCAL_PROC_ENTRY) {
		udi_trace_write(&rdata->init_context, UDI_TREVENT_LOCAL_PROC_ENTRY, 0,	/* mgmt meta */
				100, cb, rdata);
	}

	DBG_MGMT1(("nsrmap_channel_event(%p) context = %p\n", cb, rdata));

	switch (cb->event) {
	case UDI_CHANNEL_CLOSED:
		DBG_MGMT3(("UDI_CHANNEL_CLOSED\n"));
		/*
		 * TODO: Handle abrupt unbind & release all allocated data 
		 */
		l_chan = UDI_GCB(cb)->channel;
		close_channel = TRUE;
		break;
	case UDI_CHANNEL_BOUND:
		DBG_MGMT3(("UDI_CHANNEL_BOUND\n"));
		/*
		 * Handle bind to parent functionality 
		 */

		/*
		 * Obtain the parent buf_path from the channel
		 * QUESTION: Is this a list of <buf_path> elements ? If so, how
		 *           do we transfer the information to the 2ary region
		 *           (i.e. one is for Rx and one for Tx, but which ??)
		 */
		DBG_MGMT3(("path_handles[0]= %p\n",
			   cb->params.parent_bound.path_handles[0]));
		DBG_MGMT3(("path_handles[1]= %p\n",
			   cb->params.parent_bound.path_handles[1]));

		rdata->parent_buf_path = *cb->params.parent_bound.path_handles;
		DBG_MGMT3(("parent_buf_path = %p\n", rdata->parent_buf_path));

		/*
		 * Allocate a bind cb which we'll hang on to until the
		 * corresponding unbind occurs
		 */
		rdata->State = BINDING;

		udi_cb_alloc(nsrmap_bind_parent_cbfn,
			     UDI_GCB(cb), NSRMAP_BIND_CB_IDX,
			     UDI_GCB(cb)->channel);
		complete = FALSE;
		break;
	default:
		DBG_MGMT3(("Unexpected event %d\n", cb->event));
		break;
	}

	/*
	 * Only issue a udi_channel_event_complete if no other routine is 
	 * expected to do so. 
	 * UDI_CHANNEL_BOUND will be completed when the parent bind
	 * process finishes
	 */
	if (complete) {
		udi_channel_event_complete(cb, UDI_OK);
	}

	/*
	 * Close channel if necessary
	 */
	if (close_channel) {
		udi_channel_close(l_chan);
	}

	if (rdata->ml_tracemask[1] & UDI_TREVENT_LOCAL_PROC_EXIT) {
		udi_trace_write(&rdata->init_context, UDI_TREVENT_LOCAL_PROC_EXIT, 0,	/* mgmt meta */
				109, cb, UDI_OK);
	}
}

/*
 * nsrmap_bind_parent_cbfn:
 * -----------------------
 * Callback for allocating a control block for ND bind.
 * Initializes the control block and calls udi_nd_bind_req()
 *
 * Step 1a in the CHANNEL_BOUND event handling.
 *
 * Calling Context:
 *	NSR Primary region
 */
STATIC void
nsrmap_bind_parent_cbfn(udi_cb_t *gcb,
			udi_cb_t *new_cb)
{
	nsrmap_region_data_t *rdata = gcb->context;

	DBG_MGMT1(("nsrmap_bind_parent_cbfn( gcb = %p, new_cb = %p )\n",
		   gcb, new_cb));
	DBG_MGMT1(("\t...gcb->context = %p, new_cb->context = %p\n",
		   gcb->context, new_cb->context));

	rdata->bind_cb = new_cb;	/* Save for udi_nd_bind_req */

	rdata->bind_info.bind_channel = gcb->channel;	/*Management chan */
	rdata->bind_info.bind_cb = gcb;

	/*
	 * Allocate a CB which we use for completing NSR_enable when a
	 * LINK-UP event doesn't occur in a reasonable time
	 */
	udi_cb_alloc(nsrmap_linkup_cballoc, new_cb, NSRMAP_BIND_CB_IDX,
		     new_cb->channel);
}

/*
 * nsrmap_linkup_cballoc:
 * ---------------------
 * Callback for allocating a control block for use in detecting a deferred
 * LINK-UP indication. This typically happens if the NIC is opened when the
 * cable is not attached to an active network.
 *
 * Step 1b in the CHANNEL_BOUND event handling
 *
 * Calling Context:
 *	NSR Primary region
 */
STATIC void
nsrmap_linkup_cballoc(udi_cb_t *gcb,
		      udi_cb_t *new_cb)
{
	nsrmap_region_data_t *rdata = gcb->context;

	DBG_MGMT2(("nsrmap_linkup_cballoc( gcb = %p, new_cb = %p)\n",
		   gcb, new_cb));

	rdata->link_timer_cb = new_cb;

	/*
	 * Spawn Tx and Rx channels from anchored control channel
	 * Need to wait for spawning to be complete before we can 
	 * continue.
	 */

	DBG_MGMT2(("\t>Spawning RX channel[%d], cb = %p, channel = %p\n",
		   NSR_RX_CHAN, gcb, rdata->bind_info.bind_channel));

	udi_channel_spawn(nsrmap_rx_spawn_done, rdata->bind_cb, rdata->bind_info.bind_channel, NSR_RX_CHAN, NSRMAP_RX_IDX,	/* Anchored */
			  rdata);
}

/*
 * nsrmap_tx_spawn_done:
 * --------------------
 * Callback routine for Tx channel. We've now spawned our end of the channel.
 * It will become usable once we receive a response from the ND, indicating
 * that its end has been spawned. This channel should be used for all Tx ops.
 * We need to pass this unanchored channel to the Tx region via the
 * tx_xfer_cb, the region is responsible for anchoring the channel. Once this
 * communication has started we continue with spawning the Rx channel
 * NOTE: we use the tr_params inline area for passing the udi_channel_t.
 *
 * Step 4 in the CHANNEL_BOUND event handling.
 *
 * Calling Context:
 *	NSR Primary region
 */
STATIC void
nsrmap_tx_spawn_done(udi_cb_t *gcb,
		     udi_channel_t new_channel)
{
	nsrmap_region_data_t *rdata = gcb->context;
	nsrmap_gio_t *gio_cmdp = rdata->gio_cmdp;
	_nsrmap_chan_param_t *chanp = rdata->tx_xfer_cb->tr_params;

	DBG_ND2(("nsrmap_tx_spawn_done( gcb = %p, new_channel = %p, context %p )\n", gcb, new_channel, rdata));

	rdata->bind_cb = gcb;		/* In case of reallocation */

	/*
	 * Inform Tx region of its channel to use for all transmit ops.
	 */
	gio_cmdp->cmd = NSR_GIO_SET_CHANNEL;
	chanp->channel = new_channel;

	nsr_send_region(rdata, NSRMAP_TX_REG_IDX, NULL, gio_cmdp);

	/*
	 * Completion of the region setup and binding is handled in
	 * nsrmap_gio_xfer_ack()
	 */
}

/*
 * _nsrmap_tx_cmd_cbfn:
 * -------------------
 * Callback function for transferring a command to the Tx region. The <new_buf>
 * contains the command, we issue this to the Tx region via udi_gio_xfer_req
 * and then transfer control to the next_func routine.
 *
 * Calling Context:
 *	NSR Primary region
 */
STATIC void
_nsrmap_tx_cmd_cbfn(udi_cb_t *gcb,
		    udi_buf_t *new_buf)
{
	nsrmap_region_data_t *rdata = gcb->context;
	void (*cbfn) (udi_cb_t *);

	rdata->tx_xfer_cb->data_buf = new_buf;
	rdata->tx_xfer_cb->op = UDI_GIO_OP_WRITE;

	/*
	 * Completion occurs in nsrmap_gio_xfer_ack 
	 */
	udi_gio_xfer_req(rdata->tx_xfer_cb);

	/*
	 * Call the next step in our execution sequence 
	 */
	cbfn = rdata->next_func;
	rdata->next_func = NULL;
	if (cbfn != NULL) {
		(*cbfn) (gcb);
	}
}

/*
 * nsrmap_rx_spawn_done:
 * --------------------
 * Callback routine for Rx channel. We've now spawned our end of the channel.
 * It will become usable once we receive a response from the ND, indicating
 * that its end has been spawned. This channel should be used for all Rx ops.
 *
 * We now need a udi_nic_rx_cb_t to use for subsequent Rx allocations. Once we
 * have this we can spawn the Tx channel and pass it to the Tx region.
 *
 * Step 2 in the CHANNEL_BOUND event handling.
 *
 * Calling Context:
 *	NSR Primary region
 */
STATIC void
nsrmap_rx_spawn_done(udi_cb_t *gcb,
		     udi_channel_t new_channel)
{
	nsrmap_region_data_t *rdata = gcb->context;

	DBG_ND2(("nsrmap_rx_spawn_done( gcb = %p, new_channel = %p, context %p )\n", gcb, new_channel, rdata));


	/*
	 * Reset bind_cb in case of reallocation
	 */
	rdata->bind_cb = gcb;

	BSET(rdata->NSR_Rx_state, NsrmapS_Anchored);
	/*
	 * Allocate a control block to be used for Rx allocations.
	 */
	udi_cb_alloc(nsrmap_rx_alloc_cbfn, gcb, NSRMAP_RX_CB_IDX, new_channel);
}

/*
 * nsrmap_rx_alloc_cbfn:
 * --------------------
 * Callback for Rx CB allocation. We now have an anchored Rx channel and CB
 * and can allocate the Tx channel.
 *
 * Step 3 in the CHANNEL_BOUND event handling.
 */
STATIC void
nsrmap_rx_alloc_cbfn(udi_cb_t *gcb,
		     udi_cb_t *new_cb)
{
	nsrmap_region_data_t *rdata = gcb->context;

	DBG_MGMT1(("nsrmap_rx_alloc_cbfn( gcb = %p, new_cb = %p )\n",
		   gcb, new_cb));

	rdata->rx_cb = new_cb;		/* Receive control block */

	DBG_MGMT2(("\t>Spawning TX channel[%d], cb = %p, channel = %p\n",
		   NSR_TX_CHAN, new_cb, rdata->bind_info.bind_channel));

	udi_channel_spawn(nsrmap_tx_spawn_done, rdata->bind_cb, rdata->bind_info.bind_channel, NSR_TX_CHAN, 0,	/* Unanchored */
			  rdata);
}


/*
 * nsrmap_bind_ack:
 * ---------------
 * Called when ND has completed the udi_nd_bind_req and all channels are
 * anchored. This should (maybe) enable the network card, although we might
 * wait to do this when the OS-specific mapper first tries to access it.
 *
 * Step 6 in the CHANNEL_BOUND event handling.
 */
STATIC void
nsrmap_bind_ack(udi_nic_bind_cb_t *cb,	/* (potentially) new bind_cb */

		udi_status_t status)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;

#ifdef NSRMAP_DEBUG
	udi_assert(rdata == rdata->rdatap);
#endif /* NSRMAP_DEBUG */

	if (rdata->ml_tracemask[1] & UDI_TREVENT_LOCAL_PROC_ENTRY) {
		udi_trace_write(&rdata->init_context, UDI_TREVENT_LOCAL_PROC_ENTRY, 0,	/* mgmt meta */
				110, cb, status);
	}

	DBG_ND2(("nsrmap_bind_ack( cb = %p, status = %d )\n", cb, status));
	DBG_ND3(("\t...media_type = %d, min_pdu = %d, max_pdu = %d\n",
		 cb->media_type, cb->min_pdu_size, cb->max_pdu_size));
	DBG_ND3(("\t...rx_hw_threshold = %d, mac_addr_len = %d\n",
		 cb->rx_hw_threshold, cb->mac_addr_len));

	/*
	 * Dump first 6 bytes of MAC address 
	 */
	DBG_ND3(("\t...MAC address = %2x.%2x.%2x.%2x.%2x.%2x\n",
		 cb->mac_addr[0], cb->mac_addr[1], cb->mac_addr[2],
		 cb->mac_addr[3], cb->mac_addr[4], cb->mac_addr[5]));

	/*
	 * Squirrel away the link information for future mapper use 
	 */
	udi_memcpy(&rdata->link_info, cb, sizeof (udi_nic_bind_cb_t));

	/*
	 * Calculate the maximum buffer size if max_pdu is zero. If it isn't
	 * we use this value instead.
	 * NOTE: The maximum buffer size is likely to be OS specific and
	 *       very dependent on what the architecture of the network
	 *       connection looks like. For example, a stock SVr4 stack would
	 *       have the DLPI de-muxing incorporated into the driver whereas
	 *       an SCO-style MDI implementation has the de-muxing performed
	 *       at a higher level.
	 *       To accommodate this we call out to an OS-specific routine to
	 *       determine the maximum frame size for the given card if the ND
	 *       specifies a default (0) value.
	 */

	if (rdata->link_info.max_pdu_size == 0) {
		rdata->buf_size =
			osnsr_max_xfer_size(rdata->link_info.media_type);
		rdata->link_info.max_pdu_size = rdata->buf_size;
	} else {
		rdata->buf_size = rdata->link_info.max_pdu_size;
	}

	if (rdata->link_info.min_pdu_size == 0) {
		rdata->link_info.min_pdu_size =
			osnsr_min_xfer_size(rdata->link_info.media_type);
	}

	/*
	 * Get the default MAC address size if it wasn't specified by the ND
	 */
	if (rdata->link_info.mac_addr_len == 0) {
		rdata->link_info.mac_addr_len =
			osnsr_mac_addr_size(rdata->link_info.media_type);
	}

	/*
	 * Update the rdata->bind_cb as it _may_ have been reallocated during
	 * the bind_req/bind_ack channel ops.
	 */
	rdata->bind_cb = UDI_GCB(cb);

	/*
	 * Kick the allocation of resources off in the Rx region. This needs to
	 * have rx_hw_threshold CBs with max_pdu_size buffers attached.
	 * When this allocation completes we'll finish the osnsr_bind_done
	 * code. We stash the bind status in rdata->bind_status for later
	 * access.
	 */

	rdata->bind_status = status;

	/*
	 * Update the capabilities field with the returned value from the ND.
	 * This allows the OS to take appropriate action for offloading various
	 * checksumming operations if supported.
	 */
	rdata->capabilities = cb->capabilities;

	if (status != UDI_OK) {
		/*
		 * Let the OS handle the bind failures by returning ENXIO for
		 * any device access which is attempted for this instance
		 *
		 * TODO: Log this failure
		 */
		osnsr_bind_done(UDI_MCB(rdata->bind_info.bind_cb,
					udi_channel_event_cb_t),
				status);

		return;
	}

	/*
	 * Allocate resources for Rx packets
	 */
	udi_mem_alloc(nsrmap_got_rxmem, rdata->rx_cb, sizeof (nsr_cb_t), 0);

	if (rdata->ml_tracemask[1] & UDI_TREVENT_LOCAL_PROC_EXIT) {
		udi_trace_write(&rdata->init_context,
				UDI_TREVENT_LOCAL_PROC_EXIT, 0, 119, status);
	}
}

/*
 * nsrmap_unbind_ack:
 * -----------------
 * Called on completion of udi_nd_unbind_req(). We can release the bind_cb
 * as this no longer has any useful context (the ND has 'gone away').
 * Once all the channels have been closed we let the OS-specific code know
 * that this instance of the device is no longer available, free internal
 * resources and then complete the udi_devmgmt_req() which started all this.
 */
STATIC void
nsrmap_unbind_ack(udi_nic_cb_t *cb,	/* (potentially) new bind_cb */

		  udi_status_t status)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;

#ifdef NSRMAP_DEBUG
	udi_assert(rdata == rdata->rdatap);
#endif /* NSRMAP_DEBUG */

#ifdef NSR_TRACE
	if (rdata->tracemask & UDI_TREVENT_LOCAL_PROC_ENTRY) {
		udi_snprintf(nsr_trace_buffer, sizeof (nsr_trace_buffer),
			     "nsrmap_unbind_ack(%p, %d)", cb, status);
		udi_trace_write(&rdata->init_context,
				UDI_TREVENT_LOCAL_PROC_ENTRY,
				0,
				nsr_trace_buffer,
				udi_strlen(nsr_trace_buffer),
				udi_strlen(nsr_trace_buffer) +
				NSRMAP_TRLOG_FMT_ADD);
	}
#endif /* NSR_TRACE */
	DBG_MGMT1(("nsrmap_unbind_ack( cb = %p, status = %d )\n", cb, status));


	/*
	 * Release tx and rx channels and associated control blocks 
	 */

	udi_cb_free(rdata->rx_cb);

	/*
	 * TODO: 
	 */
	osnsr_unbind_done(cb);

	/*
	 * Release the udi_nic_cb_t which was our initial rdata->bind_cb 
	 */
	udi_cb_free(UDI_GCB(cb));

	/*
	 * Clean up LINK-UP timer if it hasn't fired yet 
	 */
	if (BTST(rdata->NSR_state, NsrmapS_LinkTimerActive)) {
		udi_timer_cancel(rdata->link_timer_cb);
		BCLR(rdata->NSR_state, NsrmapS_LinkTimerActive);
	}

	/*
	 * Release the link_timer_cb 
	 */
	udi_cb_free(rdata->link_timer_cb);

#if 0
	nsr_change_state(rdata, UNBOUND);
#endif

	udi_devmgmt_ack(UDI_MCB(rdata->scratch_cb, udi_mgmt_cb_t),
			0,
			UDI_OK);
}

/*
 * nsrmap_enable_ack:
 * -----------------
 * Called on completion of a udi_nd_enable_req(). The link is now ready for
 * data-flow and all resources have been allocated.
 */
STATIC void
nsrmap_enable_ack(udi_nic_cb_t *cb,
		  udi_status_t status)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;

#ifdef NSRMAP_DEBUG
	udi_assert(rdata == rdata->rdatap);
#endif /* NSRMAP_DEBUG */

#ifdef NSR_TRACE
	if (rdata->tracemask & UDI_TREVENT_LOCAL_PROC_ENTRY) {
		udi_snprintf(nsr_trace_buffer, sizeof (nsr_trace_buffer),
			     "nsrmap_enable_ack(%p, %d)", cb, status);
		udi_trace_write(&rdata->init_context,
				UDI_TREVENT_LOCAL_PROC_ENTRY,
				0,
				nsr_trace_buffer,
				udi_strlen(nsr_trace_buffer),
				udi_strlen(nsr_trace_buffer) +
				NSRMAP_TRLOG_FMT_ADD);
	}
#endif /* NSR_TRACE */
	DBG_CTL1(("nsrmap_enable_ack( cb = %p, status = %d state %s)\n",
		  cb, status, state_names[rdata->State]));

	/*
	 * FIXME: If the enable_ack indicates an error, do we remain in the
	 *        BOUND state so that a subsequent udi_nd_enable_req can be
	 *        issued
	 */

	if (status == UDI_OK) {
		nsr_change_state(rdata, ENABLED);
	} else {
		DBG_CTL3(("udi_nd_enable_req FAILED\n"));
		if (!UDI_HANDLE_IS_NULL(rdata->cbfn, void *) &&
		    !UDI_HANDLE_IS_NULL(rdata->scratch_cb,
					udi_cb_t *)) {
			(rdata->cbfn) (rdata->scratch_cb, status);
		}
	}

	/*
	 * Release the net_cb_t control block that we allocated earlier
	 */
	udi_cb_free(UDI_GCB(cb));
}

/*
 * nsrmap_ctrl_ack:
 * ---------------
 * Called on completion of an earlier udi_nd_ctrl_req() request. Simply call
 * the OS specific hook to indicate completion. The CB is _not_ freed as this
 * can be easily used for subsequent ctrl_req/ctrl_ack operations. It will
 * be returned to the system pool on final close-down of the region
 */
STATIC void
nsrmap_ctrl_ack(udi_nic_ctrl_cb_t *cb,
		udi_status_t status)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;

#ifdef NSRMAP_DEBUG
	udi_assert(rdata == rdata->rdatap);
#endif /* NSRMAP_DEBUG */

	if (rdata->ml_tracemask[1] & UDI_TREVENT_LOCAL_PROC_ENTRY) {
		udi_trace_write(&rdata->init_context,
				UDI_TREVENT_LOCAL_PROC_ENTRY,
				0, 120, cb, status);
	}

	osnsr_ctrl_ack(cb, status);

	if (rdata->ml_tracemask[1] & UDI_TREVENT_LOCAL_PROC_EXIT) {
		udi_trace_write(&rdata->init_context,
				UDI_TREVENT_LOCAL_PROC_EXIT, 0, 129, status);
	}
}

/*
 * nsrmap_info_ack:
 * ---------------
 * Called in response to a udi_nd_info_req(). Either silently complete, or
 * transfer control to rdata->nextfunc if set.
 */
STATIC void
nsrmap_info_ack(udi_nic_info_cb_t *cb)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;

#ifdef NSRMAP_DEBUG
	udi_assert(rdata == rdata->rdatap);
#endif /* NSRMAP_DEBUG */

	if (rdata->next_func) {
		(rdata->next_func) (UDI_GCB(cb));
	} else {
		udi_cb_free(UDI_GCB(cb));
	}
}

/*
 * nsrmap_status_ind:
 * -----------------
 * Called to inform NSR about a network status change. This is one of:
 *	LINK_DOWN,
 *	LINK_UP,
 *	LINK_RESET	[same effect as LINK_DOWN]
 */
STATIC void
nsrmap_status_ind(udi_nic_status_cb_t *cb)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;

	DBG_ND1(("nsrmap_status_ind( %p ), event = %p\n", cb, cb->event));
#ifdef NSRMAP_DEBUG
	udi_assert(rdata == rdata->rdatap);
#endif /* NSRMAP_DEBUG */


	NSRMAP_ASSERT(rdata->State == ACTIVE || rdata->State == ENABLED ||
		      rdata->State == BOUND);

#ifdef NSR_TRACE
	if (rdata->tracemask & UDI_TREVENT_LOCAL_PROC_ENTRY) {
		udi_snprintf(nsr_trace_buffer, sizeof (nsr_trace_buffer),
			     "nsrmap_status_ind(%p, %d)", cb, cb->event);
		udi_trace_write(&rdata->init_context,
				UDI_TREVENT_LOCAL_PROC_ENTRY,
				0,
				nsr_trace_buffer,
				udi_strlen(nsr_trace_buffer),
				udi_strlen(nsr_trace_buffer) +
				NSRMAP_TRLOG_FMT_ADD);
	}
#endif /* NSR_TRACE */

	switch (cb->event) {
	case UDI_NIC_LINK_RESET:
		/*
		 * TODO: flag this for renegotiation 
		 */
	case UDI_NIC_LINK_DOWN:
		BCLR(rdata->NSR_state, NsrmapS_LinkUp);
		nsr_change_state(rdata, ENABLED);

		break;
	case UDI_NIC_LINK_UP:
		BSET(rdata->NSR_state, NsrmapS_LinkUp);
		if (BTST(rdata->NSR_state, NsrmapS_LinkTimerActive)) {
			udi_timer_cancel(rdata->link_timer_cb);
			BCLR(rdata->NSR_state, NsrmapS_LinkTimerActive);
		}
		nsr_change_state(rdata, ACTIVE);

		break;
	default:
		NSRMAP_ASSERT(0);
		break;
	}

	/*
	 * Call out to inform OS of status change 
	 */
	osnsr_status_ind(cb);

	udi_cb_free(UDI_GCB(cb));
}

/* ========================================================================== */

/* 				NSR Transmit Meta 			      */

/* ========================================================================== */

/*
 * nsrmap_tx_channel_event:
 * -----------------------
 * Channel event handler for Transmit channel
 * Deals with constraints propagation and channel close
 *
 * Calling Context:
 *	NSR Secondary Transmit region
 */
STATIC void
nsrmap_tx_channel_event(udi_channel_event_cb_t *cb)
{
	nsrmap_Tx_region_data_t *rdata = UDI_GCB(cb)->context;
	udi_channel_t l_chan = UDI_NULL_CHANNEL;
	udi_boolean_t close_channel = FALSE;

	DBG_MGMT1(("nsrmap_tx_channel_event(%p) context = %p\n", cb, rdata));

	switch (cb->event) {
	case UDI_CHANNEL_CLOSED:
		DBG_MGMT3(("UDI_CHANNEL_CLOSED\n"));
		l_chan = UDI_GCB(cb)->channel;
		close_channel = TRUE;
		/*
		 * Release allocated Tx buf_path 
		 */

		break;
	default:
		break;
	}

	udi_channel_event_complete(cb, UDI_OK);

	/*
	 * Close channel if necessary
	 */
	if (close_channel) {
		udi_channel_close(l_chan);
	}
}

/*
 * nsrmap_tx_rdy:
 * -------------
 * Called from ND when it has some TX CBs for our use. Have to handle two cases:
 * 1) the NSR has not yet allocated its internal structures to manage the CBs
 * 2) the structures have been allocated, this is just a response to a previous
 *    transmit operation, and is returning Tx CBs for subsequent use.
 *
 * Calling Context:
 *	Transmit region, NIC meta
 */
STATIC void
nsrmap_tx_rdy(udi_nic_tx_cb_t *cb)
{
	nsrmap_Tx_region_data_t *rdata = UDI_GCB(cb)->context;
	udi_nic_tx_cb_t *tcb, *nextcb;
	nsr_cb_t *pcb;

#ifdef NSRMAP_DEBUG
	udi_ubit32_t ii;
	udi_assert(rdata == rdata->rdatap);
#endif /* NSRMAP_DEBUG */

	if (rdata->State != ENABLED && rdata->State != ACTIVE) {
		/*
		 * Defer op 
		 */
		udi_mem_alloc(nsrmap_defer_txrdy_op, UDI_GCB(cb),
			      sizeof (_nsr_defer_op_t), 0);
		return;
	}

	NSRMAP_ASSERT(rdata->State == ENABLED || rdata->State == ACTIVE);

#ifdef NSR_TRACE
	if (rdata->tracemask & UDI_TREVENT_LOCAL_PROC_ENTRY) {
		udi_snprintf(nsr_trace_buffer, sizeof (nsr_trace_buffer),
			     "nsrmap_tx_rdy(%p)", cb);
		udi_trace_write(&rdata->init_context,
				UDI_TREVENT_LOCAL_PROC_ENTRY,
				0,
				nsr_trace_buffer,
				udi_strlen(nsr_trace_buffer),
				udi_strlen(nsr_trace_buffer) +
				NSRMAP_TRLOG_FMT_ADD);
	}
#endif /* NSR_TRACE */
	DBG_TX1(("nsrmap_tx_rdy( cb = %p )\n", cb));

#ifdef DEBUG
	for (ii = 0, tcb = cb; tcb; tcb = tcb->chain)
		ii++;

	DBG_TX3(("\t... Got %d Transmit CBs\n", ii));
#endif /* DEBUG */
	DBG_TX3(("\t...Space for %d Transmit CBs\n",
		 NSR_UDI_QUEUE_SIZE(&rdata->onq)));
	/*
	 * Save as many transmit CBs as we can fit using the onq elements.
	 * If we exhaust these we will need to allocate some more which will
	 * be time-consuming. This routine is crucial to throughput speeds as
	 * it is called in response to every Tx packet that is sent to the ND
	 *
	 * NOTE: We need to LOCK/UNLOCK around the cbq accesses as the OS
	 *       mapper may well be attempting to transmit as we are updating
	 *       the queue.
	 */
	tcb = cb;
	while (tcb) {
		/*
		 * Dequeue an nsr_cb_t from txonq 
		 */
		pcb = (nsr_cb_t *) NSR_UDI_DEQUEUE_HEAD(&rdata->onq);
		if (!pcb)
			break;
		pcb->cbp = UDI_GCB(tcb);
		nextcb = tcb->chain;
		tcb->chain = NULL;	/* Break the chain */
		NSR_UDI_ENQUEUE_TAIL(&rdata->cbq, (udi_queue_t *)pcb);
		tcb = nextcb;
	}

	rdata->ND_txcb = tcb;

	/*
	 * Queue an allocation thread to cater for any remaining Tx CBs which
	 * we didn't have space for. They are made available to the call-back
	 * by hooking them onto the rdata->ND_txcb element.
	 */
	if (tcb) {
		BCLR(rdata->NSR_state, NsrmapS_AllocDone);
		BSET(rdata->NSR_state, NsrmapS_AllocPending);
		udi_mem_alloc(nsrmap_got_txmem, rdata->my_cb, sizeof (nsr_cb_t), 0);	/* Zero-fill */
	}
	osnsr_tx_rdy(rdata);
}

/*
 * nsrmap_defer_txrdy_op:
 * ----------------------
 * Enqueue an operation which will be scheduled later on. Simply allocate space
 * for an _nsr_defer_op_t and fill it in.
 */
STATIC void
nsrmap_defer_txrdy_op(udi_cb_t *gcb,
		      void *new_mem)
{
	nsrmap_Tx_region_data_t *rdata = gcb->context;
	_nsr_defer_op_t *op = (_nsr_defer_op_t *) new_mem;

	switch (rdata->State) {
	case ENABLED:
	case ACTIVE:
		/*
		 * We've already changed to the valid state so we can simply
		 * complete the deferred nsrmap_tx_rdy op.
		 */
		DBG_TX3(("nsrmap_defer_txrdy_op: State already changed\n"));
		udi_mem_free(new_mem);
		nsrmap_tx_rdy(UDI_MCB(gcb, udi_nic_tx_cb_t));

		break;
	default:
		DBG_TX3(("nsrmap_defer_txrdy_op: Deferring nsrmap_tx_rdy\n"));
		op->func = (void (*)(udi_cb_t *))nsrmap_tx_rdy;
		op->cb = gcb;

		NSR_UDI_ENQUEUE_TAIL(&rdata->deferq, &op->Q);

		/*
		 * Start a deferred callback chain to process any deferq
		 * entries. This will be self-perpetuating until we transition
		 * to the ACTIVE or ENABLED state.
		 */
		nsrmap_start_defer_Tx_chain(gcb);
	}
}

/*
 * nsrmap_start_defer_Tx_chain:
 * ---------------------------
 * Start a timeout chain to check on the state of the NSR. When we transition
 * to ENABLED or ACTIVE we can successfully start the deferred nsrmap_tx_rdy
 * operation. The callback is responsible for rescheduling itself while
 * the NSR is not in the UNBOUND state.
 */
STATIC void
nsrmap_start_defer_Tx_chain(udi_cb_t *gcb)
{
	nsrmap_Tx_region_data_t *rdata = gcb->context;

	if (BTST(rdata->NSR_state, NsrmapS_TimerActive)) {
		/*
		 * Nothing to do, the chain is already running 
		 */
		return;
	} else {
		/*
		 * Allocate a timer_cb for the udi_timer_start() 
		 */
		udi_cb_alloc(nsrmap_defer_Tx_cballoc, gcb, NSRMAP_TX_CB_IDX,
			     gcb->channel);
	}
}

/*
 * nsrmap_defer_Tx_cballoc:
 * -----------------------
 * Callback for the timer_cb which is allocated for use by 
 * nsrmap_defer_Tx_timeout
 */
STATIC void
nsrmap_defer_Tx_cballoc(udi_cb_t *gcb,
			udi_cb_t *new_cb)
{
	nsrmap_Tx_region_data_t *rdata = gcb->context;
	udi_time_t tval;

	rdata->timer_cb = new_cb;
	BSET(rdata->NSR_state, NsrmapS_TimerActive);

	tval.seconds = NSR_DEFER_SECS;
	tval.nanoseconds = NSR_DEFER_NSECS;

	udi_timer_start(nsrmap_defer_Tx_timeout, new_cb, tval);
}

/*
 * nsrmap_defer_Tx_timeout:
 * -----------------------
 * Timeout routine for handling deferred Transmit operations. Any queued 
 * operations can be scheduled as soon as the state changes to ACTIVE or 
 * ENABLED. If we are still blocked by an incorrect state we queue another
 * timeout for NSR_DEFER_SECS:NSR_DEFER_NSECS time.
 * Also, if there is more than one deferred operation queued, we keep the
 * timeout chain running until the queue is emptied.
 * Once we've stopped the chain we free all of the resources.
 */
STATIC void
nsrmap_defer_Tx_timeout(udi_cb_t *gcb)
{
	nsrmap_Tx_region_data_t *rdata = gcb->context;
	udi_time_t tval;
	_nsr_defer_op_t *op;
	void (*func) (udi_cb_t *);
	udi_cb_t *cb;
	udi_boolean_t restart = TRUE;

	switch (rdata->State) {
	case ACTIVE:
	case ENABLED:
		op = (_nsr_defer_op_t *) NSR_UDI_DEQUEUE_HEAD(&rdata->deferq);
		if (op) {
			func = op->func;
			cb = op->cb;
			udi_mem_free(op);
			if (NSR_UDI_QUEUE_EMPTY(&rdata->deferq)) {
				restart = FALSE;
			}

			/*
			 * Call deferred op 
			 */
			(*func) (cb);
		} else {
			restart = FALSE;
		}
		break;
	default:
		DBG_TX2(("nsrmap_defer_Tx_timeout: Invalid state [%s]\n",
			 state_names[rdata->State]));
		break;
	}
	if (restart) {
		tval.seconds = NSR_DEFER_SECS;
		tval.nanoseconds = NSR_DEFER_NSECS;

		udi_timer_start(nsrmap_defer_Tx_timeout, gcb, tval);
	} else {
		/*
		 * Clear up timer resources
		 */
		BCLR(rdata->NSR_state, NsrmapS_TimerActive);
		udi_cb_free(gcb);
		rdata->timer_cb = NULL;
	}
}

/*
 * nsr_commit_tx_cbs:
 * -----------------
 * Commit the outstanding Transmit CBs from the rdata->cbq to the ND.
 * If the NsrmapS_CommitWanted flag is set we complete the pending GIO request
 */
STATIC void
nsr_commit_tx_cbs(nsrmap_Tx_region_data_t * rdata)
{
	udi_queue_t *elem;
	nsr_cb_t *pcb;
	udi_nic_tx_cb_t *txcb, *prev = NULL;

	/*
	 * Commit all unused udi_nic_tx_cb_t's to the ND 
	 */
	DBG_GIO3(("#Tx onq = %d\n", NSR_UDI_QUEUE_SIZE(&rdata->onq)));
	DBG_GIO3(("#Tx cbq = %d\n", NSR_UDI_QUEUE_SIZE(&rdata->cbq)));

	BCLR(rdata->NSR_state, NsrmapS_CommitWanted);

	while ((elem = NSR_UDI_DEQUEUE_HEAD(&rdata->cbq)) != NULL) {
		pcb = (nsr_cb_t *) elem;
		txcb = UDI_MCB(pcb->cbp, udi_nic_tx_cb_t);

		if (txcb) {
			((nsr_cb_t *) elem)->cbp = NULL;
			txcb->chain = prev;
			txcb->tx_buf = NULL;
			prev = txcb;
		}
		NSR_UDI_ENQUEUE_TAIL(&rdata->onq, elem);
	}
	if (!UDI_HANDLE_IS_NULL(prev, udi_nic_tx_cb_t *)) {
		udi_nd_tx_req(prev);
	}

	DBG_GIO3(("#Tx onq = %d\n", NSR_UDI_QUEUE_SIZE(&rdata->onq)));
	udi_gio_xfer_ack(rdata->xfer_cb);
}

/*
 * ===========================================================================
 * Externally visible interfaces:
 *	NSR_enable		- enable the physical link
 *	NSR_disable		- disable the physical link
 *	NSR_return_packet	- return udi_nic_rx_cb_t to ND
 *	NSR_ctrl_req		- issue a control request to the ND
 *	NSR_txdata		- return the Transmit region's local data handle
 *
 * NSR_enable:
 *	Called from driver open() code. The gcb will have been stashed away
 *	in the per-queue private data and is needed to get access to our
 *	region-local data. On completion of this routine the passed in
 *	callback function is executed. The calling routine should block
 *	waiting for this completion if necessary.
 */
STATIC void
NSR_enable(udi_cb_t *gcb,
	   void (*cbfn) (udi_cb_t *,
			 udi_status_t))
{
	nsrmap_region_data_t *rdata = gcb->context;

	DBG_OS1(("NSR_enable( %p, %p )\n", gcb, cbfn));

	NSRMAP_ASSERT(rdata->State == BOUND);

	rdata->scratch_cb = gcb;	/* Save for completion */
	rdata->cbfn = cbfn;		/* Callback function   */

	BSET(rdata->NSR_state, NsrmapS_Enabling);

	/*
	 * Allocate a udi_nic_cb_t for us to use
	 */
	udi_cb_alloc(NSR_enable_alloc, gcb, NSRMAP_NET_CB_IDX,
		     rdata->bind_info.bind_channel);
}

STATIC void
NSR_enable_alloc(udi_cb_t *gcb,
		 udi_cb_t *new_cb)
{
	nsrmap_region_data_t *rdata = new_cb->context;
	udi_nic_cb_t *enable_cb = UDI_MCB(new_cb,
					  udi_nic_cb_t);

	DBG_OS2(("NSR_enable_alloc( %p, %p )\n", gcb, new_cb));

	rdata->n_packets = rdata->n_discards = 0;
	udi_nd_enable_req(enable_cb);
}

/*
 * NSR_disable:
 * -----------
 * OS interface to disable the hardware link to the ND. The data paths and
 * structures are maintained in case of a subsequent NSR_enable() call. The
 * ND allocated CBs are returned (via transmit requests with no buffers) and
 * the internal state is updated to show that its disabled.
 *
 * Calling Context:
 *	OS NSR mapper, Control region
 */
STATIC void
NSR_disable(udi_cb_t *gcb,
	    void (*cbfn) (udi_cb_t *,
			  udi_status_t))
{
	nsrmap_region_data_t *rdata = gcb->context;

	DBG_ND2(("NSR_disable: state %s\n", state_names[rdata->State]));

	NSRMAP_ASSERT(rdata->State == ENABLED || rdata->State == ACTIVE);

	BSET(rdata->NSR_state, NsrmapS_Disabling);

	rdata->scratch_cb = gcb;	/* Save for completion */
	rdata->cbfn = cbfn;		/* Callback function   */

	/*
	 * udi_nd_disable_req only allowed if state is currently ENABLED
	 */
	if (rdata->State != ENABLED && rdata->State != ACTIVE) {
		(rdata->cbfn) (rdata->scratch_cb, UDI_STAT_INVALID_STATE);
		rdata->cbfn = NULL;
		rdata->scratch_cb = NULL;
		return;
	}
	udi_cb_alloc(NSR_disable_alloc, gcb, NSRMAP_NET_CB_IDX,
		     rdata->bind_info.bind_channel);
}

STATIC void
NSR_disable_alloc(udi_cb_t *gcb,
		  udi_cb_t *new_cb)
{
	nsrmap_region_data_t *rdata = gcb->context;
	udi_nic_cb_t *disable_cb = UDI_MCB(new_cb, udi_nic_cb_t);

	/*
	 * Disable ND - no acknowledgement
	 */
	udi_nd_disable_req(disable_cb);

	/*
	 * Issue state change to secondary regions
	 */
	nsr_change_state(rdata, BOUND);
}

/*
 * NSR_ctrl_req:
 * ------------
 * Issue a udi_nd_ctrl_req to the underlying ND. The calling routine should
 * wait for completion [indicated by receipt of osnsr_ctrl_ack] and must
 * provide the appropriate control block for this routine's use.
 *
 * Calling Context:
 *	OS NSR mapper, primary region
 */
STATIC void
NSR_ctrl_req(udi_nic_ctrl_cb_t *cb)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;

	/*
	 * Verify that we're in a legal state to despatch this request 
	 */
	if (rdata->State != BOUND && rdata->State != ENABLED &&
	    rdata->State != ACTIVE) {
		osnsr_ctrl_ack(cb, UDI_STAT_NOT_UNDERSTOOD);
		return;
	}

	udi_nd_ctrl_req(cb);
}

/*
 * NSR_txdata:
 * ----------
 * Return the Transmit regions local data handle if available. This allows the
 * OS mapper to access the Tx local data structures.
 *
 * Calling Context:
 *	OS NSR mapper, primary region
 */
#ifndef UDI_POSIX_ENV
STATIC
#endif /* UDI_POSIX_ENV */
nsrmap_Tx_region_data_t *
NSR_txdata(nsrmap_region_data_t * rdata)
{
	_udi_channel_t *ch;

	ch = (_udi_channel_t *)rdata->tx_chan;

	if (!UDI_HANDLE_IS_NULL(ch, _udi_channel_t *)) {
		return (nsrmap_Tx_region_data_t *) ch->other_end->chan_context;
	} else {
		return NULL;
	}
}

#if 0
/*
 * nsr_recycle_Rx_q:
 * ----------------
 * Reinitialise the NSR Rx CB queues. Delete all on the rxonq (the ND will have
 * freed all associated buffers and CBs) and then start the allocation chain.
 */
STATIC void
nsr_recycle_Rx_q(nsrmap_region_data_t * rdata)
{
	udi_queue_t *elem;

	DBG_GIO3(("nsr_recycle_Rx_q: #rxonq = %d, #rxcbq = %d\n",
		  _OSDEP_ATOMIC_INT_READ(&rdata->rxonq.numelem),
		  _OSDEP_ATOMIC_INT_READ(&rdata->rxcbq.numelem)));

	while (elem = NSR_UDI_DEQUEUE_HEAD(&rdata->rxonq))
		udi_mem_free(elem);

	/*
	 * Start the allocation chain off again
	 */
	BSET(rdata->NSR_Rx_state, NsrmapS_AllocPending);
	udi_mem_alloc(nsrmap_got_rxmem, rdata->rx_cb, sizeof (nsr_cb_t), 0);

}
#endif

/* -------------------------------------------------------------------------- */

/* 				NSR Receive Meta 			      */

/*									      */

/* nsrmap_rx_channel_event						      */

/* _nsrmap_rx_ind							      */

/* nsrmap_rx_ind							      */

/* nsrmap_exp_rx_ind							      */

/* -------------------------------------------------------------------------- */

/*
 * nsrmap_rx_channel_event:
 * -----------------------
 * Channel event handler for Receive channel.
 * Handle constraints propagation and channel close
 *
 * Calling Context:
 *	ND Receive channel -> Receive region
 */
STATIC void
nsrmap_rx_channel_event(udi_channel_event_cb_t *cb)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;
	udi_channel_t l_chan = UDI_NULL_CHANNEL;
	udi_boolean_t close_channel = FALSE;

	DBG_MGMT1(("nsrmap_rx_channel_event(%p) context = %p\n", cb, rdata));

	switch (cb->event) {
	case UDI_CHANNEL_CLOSED:
		DBG_MGMT3(("UDI_CHANNEL_CLOSED\n"));
		/*
		 * TODO: Handle abrupt unbind & release all allocated data 
		 */
		l_chan = UDI_GCB(cb)->channel;
		close_channel = TRUE;
		break;
	default:
		break;
	}

	udi_channel_event_complete(cb, UDI_OK);

	/*
	 * Close channel if necessary
	 */
	if (close_channel) {
		udi_channel_close(l_chan);
	}
}

STATIC void
nsrmap_rx_ind_common(udi_nic_rx_cb_t *cb,
		     udi_boolean_t expedited)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;

#ifdef NSRMAP_DEBUG
	udi_assert(rdata == rdata->rdatap);
#endif /* NSRMAP_DEBUG */

	NSRMAP_ASSERT(rdata->State == ACTIVE);

	/*
	 * The osnsr_rx_packet() is responsible for dequeueing containers from
	 * Rx onq and for handling expedited vs non-expedited requests. 
	 * The containers will be returned to Rx cbq by NSR_return_packet
	 */

	if (!BTST(rdata->NSR_state, NsrmapS_Disabling)) {
		/*
		 * Pass data up to OS stack 
		 */
		osnsr_rx_packet(cb, expedited);
	}
}

STATIC void
nsrmap_rx_ind(udi_nic_rx_cb_t *cb)
{
	nsrmap_rx_ind_common(cb, FALSE);
}


STATIC void
nsrmap_exp_rx_ind(udi_nic_rx_cb_t *cb)
{
	nsrmap_rx_ind_common(cb, TRUE);
}

/*
 * nsrmap_commit_cbs:
 * -----------------
 * Inform the ND of any receive buffers that we have available. 
 */
STATIC void
nsrmap_commit_cbs(nsrmap_region_data_t * rdata)
{
	nsr_cb_t *pcb;
	udi_queue_t *elem;
	udi_nic_rx_cb_t *rxcb, *prev = NULL;

	DBG_RX2(("nsrmap_commit_cbs( %p )\n", rdata));
	DBG_RX3(("nsrmap_commit_cbs NSR_state = %p\n", rdata->NSR_state));
	DBG_RX3(("nsrmap_commit_cbs State %s\n", state_names[rdata->State]));

	if (rdata->State == ENABLED || rdata->State == ACTIVE) {
		DBG_RX3(("nsrmap_commit_cbs: #rxonq %d, #rxcbq %d\n",
			 NSR_UDI_QUEUE_SIZE(&rdata->rxonq),
			 NSR_UDI_QUEUE_SIZE(&rdata->rxcbq)));

		/*
		 * Inform the ND of the number of Rx CBs that we have 
		 */
		while ((elem = NSR_UDI_DEQUEUE_HEAD(&rdata->rxcbq)) != NULL) {
			pcb = (nsr_cb_t *) elem;
			rxcb = UDI_MCB(pcb->cbp, udi_nic_rx_cb_t);

			rxcb->chain = prev;
			prev = rxcb;
			NSR_UDI_ENQUEUE_HEAD(&rdata->rxonq, elem);
		}
		if (!UDI_HANDLE_IS_NULL(prev, udi_nic_rx_cb_t *)) {
			udi_nd_rx_rdy(prev);
		}
		/*
		 * Mark the link as enabled (i.e. ready for data xfer)
		 */
		BSET(rdata->NSR_state, NsrmapS_Enabled);

	}
}

/* -------------------------------------------------------------------------- */

/*			Operating System specific Interfaces		      */

/*									      */

/* NSR_return_packet							      */

/* -------------------------------------------------------------------------- */

/*
 * NSR_return_packet:
 * --------------------
 * Return the given packet to the underlying network device. This is called
 * when the OS-specific receive processing has been completed. Once called
 * the data associated with the <cb> should _not_ be accessed as it will be
 * given to the ND for subsequent receive processing
 *
 * Calling Context:
 *	OS NSR mapper, Receive region
 */
STATIC void
NSR_return_packet(udi_nic_rx_cb_t *cb)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;
	nsr_cb_t *pcb;
	udi_nic_rx_cb_t *tcb;

	/*
	 * We only have one container per Rx CB. This means we only dequeue
	 * once per udi_nsr_rx_ind / udi_nd_rx_rdy pair. The OS-specific part
	 * of the NSR will have dequeued this container, we just need to make
	 * it available again by posting it onto the rxonq.
	 */
	NSR_GET_SCRATCH(UDI_GCB(cb)->scratch, pcb);
	udi_assert(pcb != NULL);
	NSR_UDI_ENQUEUE_TAIL(&rdata->rxonq, &pcb->q);

	for (tcb = cb; tcb; tcb = tcb->chain) {
		/*
		 * Delete all valid data from receive buffer 
		 */
		tcb->rx_buf->buf_size = 0;
	}

	udi_nd_rx_rdy(cb);
}


/* ========================================================================== */

/* 			GIO Provider Ops Tx region			      */

/* ========================================================================== */

/*
 * nsrmap_gio_Tx_prov_channel_event:
 * --------------------------------
 * Channel event indication issued to secondary region [provider]
 */
STATIC void
nsrmap_gio_Tx_prov_channel_event(udi_channel_event_cb_t *cb)
{
	udi_channel_t l_chan = UDI_NULL_CHANNEL;
	udi_boolean_t close_channel = FALSE;

	DBG_MGMT1(("nsrmap_gio_Tx_prov_channel_event(%p) context = %p\n", cb,
		   UDI_GCB(cb)->context));

	switch (cb->event) {
	case UDI_CHANNEL_CLOSED:
		DBG_MGMT3(("UDI_CHANNEL_CLOSED\n"));
		l_chan = UDI_GCB(cb)->channel;
		close_channel = TRUE;
		break;
	default:
		break;
	}

	udi_channel_event_complete(cb, UDI_OK);

	/*
	 * Close channel if necessary
	 */
	if (close_channel) {
		udi_channel_close(l_chan);
	}
}

/*
 * nsrmap_gio_Tx_bind_req:
 * ----------------------
 * Secondary region handling of udi_gio_bind_req
 */
STATIC void
nsrmap_gio_Tx_bind_req(udi_gio_bind_cb_t *cb)
{
	nsrmap_Tx_region_data_t *rdata = UDI_GCB(cb)->context;

	DBG_GIO1(("nsrmap_gio_Tx_bind_req(%p)\n", cb));

	rdata->rdatap = (void *)rdata;

	NSR_UDI_QUEUE_INIT(&rdata->cbq);
	NSR_UDI_QUEUE_INIT(&rdata->onq);
	NSR_UDI_QUEUE_INIT(&rdata->deferq);

	udi_gio_bind_ack(cb, 0, 0, UDI_OK);
}

/*
 * nsrmap_gio_Tx_unbind_req:
 * ------------------------
 * Secondary region handling of udi_gio_unbind_req
 */
STATIC void
nsrmap_gio_Tx_unbind_req(udi_gio_bind_cb_t *cb)
{
	nsrmap_Tx_region_data_t *rdata = UDI_GCB(cb)->context;

	DBG_GIO1(("nsrmap_gio_Tx_unbind_req(%p)\n", cb));

	/*
	 * Release our CB 
	 */
	udi_cb_free(rdata->my_cb);

	NSR_UDI_QUEUE_DEINIT(&rdata->cbq);
	NSR_UDI_QUEUE_DEINIT(&rdata->onq);
	NSR_UDI_QUEUE_DEINIT(&rdata->deferq);

	udi_gio_unbind_ack(cb);
}

/*
 * nsrmap_gio_Tx_xfer_req:
 * ----------------------
 * Secondary region handling of udi_gio_xfer_req
 *
 *	NSR_GIO_SET_CHANNEL:
 *		Anchor the passed in channel to this region
 *		Allocate a udi_nic_tx_cb_t to use for all Tx requests
 *		Allocate a udi_buf_path_t to use for Tx buffer allocations
 *		ACK the operation
 *	NSR_GIO_SET_STATE:
 *		Set the rdata->State to the passed in value
 *		ACK the operation
 *	NSR_GIO_ALLOC_RESOURCES:
 *		Set the buf_size field from the passed max_size parameter
 *		ACK the operation
 *	NSR_GIO_FREE_RESOURCES:
 *		Free all allocated resources
 *		ACK the operation
 *	NSR_GIO_COMMIT_CBS:
 *		Pass the allocated udi_nic_tx_cb_t + buffer to the ND
 *		ACK the operation
 */
STATIC void
nsrmap_gio_Tx_xfer_req(udi_gio_xfer_cb_t *cb)
{
	nsrmap_Tx_region_data_t *rdata = UDI_GCB(cb)->context;
	nsrmap_gio_t op;
	_nsrmap_trace_t *tp;
	udi_queue_t *elem;
	_udi_nic_state_t oldstate;
	_nsrmap_chan_param_t *chanp;

	DBG_GIO1(("nsrmap_gio_Tx_xfer_req(%p) context %p\n", cb, rdata));

	switch (cb->op) {
	case UDI_GIO_OP_WRITE:
		udi_buf_read(cb->data_buf, 0, sizeof (op), &op);
		switch (op.cmd) {
		case NSR_GIO_SET_TRACEMASK:
			tp = &op.un.tr;
			DBG_GIO3(("=>NSR_GIO_SET_TRACEMASK meta %d, mask %p\n",
				  tp->meta_idx, tp->tracemask));

			rdata->ml_tracemask[tp->meta_idx] = tp->tracemask;
			udi_gio_xfer_ack(cb);
			break;
		case NSR_GIO_SET_CHANNEL:
			chanp = (_nsrmap_chan_param_t *) cb->tr_params;
			DBG_GIO3(("=>NSR_GIO_SET_CHANNEL %p\n",
				  chanp->channel));
			udi_channel_anchor(nsrmap_channel_anchored,
					   UDI_GCB(cb), chanp->channel,
					   NSRMAP_TX_IDX, rdata);
			break;
		case NSR_GIO_SET_STATE:
			DBG_GIO3(("=>NSR_GIO_SET_STATE %s\n",
				  state_names[op.un.st]));
			oldstate = rdata->State;
			rdata->State = op.un.st;
			switch (rdata->State) {
			case ENABLED:
				BSET(rdata->NSR_state, NsrmapS_Enabled);
				break;
			case BOUND:
				switch (oldstate) {
				case ENABLED:
					BCLR(rdata->NSR_state,
					     NsrmapS_Enabled);
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
			udi_gio_xfer_ack(cb);
			break;
		case NSR_GIO_COMMIT_CBS:
			DBG_GIO3(("=>NSR_GIO_COMMIT_CBS\n"));
			/*
			 * If we're not fully allocated, queue the commit
			 */
			rdata->xfer_cb = cb;
#if 0 || OLDWAY
			if (!BTST(rdata->NSR_state, NsrmapS_AllocDone)) {
				BSET(rdata->NSR_state, NsrmapS_CommitWanted);
			} else {
				nsr_commit_tx_cbs(rdata);
			}
#else
			if (BTST(rdata->NSR_state, NsrmapS_AllocPending)) {
				BSET(rdata->NSR_state, NsrmapS_CommitWanted);
			} else {
				nsr_commit_tx_cbs(rdata);
			}
#endif /* 0 || OLDWAY */
			break;
		case NSR_GIO_ALLOC_RESOURCES:
			DBG_GIO3(("=>NSR_GIO_ALLOC_RESOURCES\n"));
			/*
			 * Save the max_size in buf_size for OS mapper use
			 */
			rdata->buf_size = op.un.res.max_size;
			udi_gio_xfer_ack(cb);
			break;
		case NSR_GIO_FREE_RESOURCES:
			DBG_GIO3(("=>NSR_GIO_FREE_RESOURCES\n"));
			/*
			 * Release all container nsr_cb_t's from onq. cbq should
			 * be empty already
			 */
			DBG_GIO3(("#Tx onq = %d, #Tx cbq = %d\n",
				  NSR_UDI_QUEUE_SIZE(&rdata->onq),
				  NSR_UDI_QUEUE_SIZE(&rdata->cbq)));
			while ((elem =
				NSR_UDI_DEQUEUE_HEAD(&rdata->onq)) != NULL)
				udi_mem_free(elem);

			/*
			 * Release the Tx udi_buf_path
			 */
			udi_buf_path_free(rdata->buf_path);

			udi_gio_xfer_ack(cb);
			break;
		default:
			udi_gio_xfer_nak(cb, UDI_STAT_NOT_UNDERSTOOD);
		}
		break;
	default:
		udi_gio_xfer_nak(cb, UDI_STAT_NOT_SUPPORTED);
	}
}

/*
 * nsrmap_gio_Tx_event_res:
 * -----------------------
 * 
 */
STATIC void
nsrmap_gio_Tx_event_res(udi_gio_event_cb_t *cb)
{
	DBG_GIO1(("nsrmap_gio_Tx_event_res(%p)\n", cb));
}


/*
 * nsrmap_channel_anchored:
 * -----------------------
 * Callback for udi_channel_anchor. Need to allocate a suitable CB for use as
 * our data CB and associate this new channel with it. 
 *
 * Calling Context:
 *	Secondary Transmit Region
 */
STATIC void
nsrmap_channel_anchored(udi_cb_t *gcb,
			udi_channel_t anchored_channel)
{
	udi_cb_alloc(nsrmap_reg_cb_alloc, gcb, NSRMAP_TX_CB_IDX,
		     anchored_channel);
}

/*
 * nsrmap_reg_cb_alloc:
 * -------------------
 * Callback for secondary region CB allocation. This CB will be used for all
 * future CB allocations within the region. 
 * We need to allocate a udi_buf_path_t for future Tx buffer allocations.
 * When this has completed we can complete the NSR_GIO_SET_CHANNEL command
 *
 * Calling Context:
 *	Secondary Transmit Region
 */
STATIC void
nsrmap_reg_cb_alloc(udi_cb_t *gcb,
		    udi_cb_t *new_cb)
{
	nsrmap_Tx_region_data_t *rdata = gcb->context;

	rdata->my_cb = new_cb;
	udi_buf_path_alloc(nsrmap_Tx_buf_path_cbfn, gcb);
}

/*
 * nsrmap_Tx_buf_path_cbfn:
 * -----------------------
 * Callback for secondary region buf_path which must be used for all Tx buffer
 * allocations. This is the last step in the NSR_GIO_SET_CHANNEL operation so
 * we simply ACK for outstanding udi_gio_xfer_req
 */
STATIC void
nsrmap_Tx_buf_path_cbfn(udi_cb_t *gcb,
			udi_buf_path_t new_buf_path)
{
	nsrmap_Tx_region_data_t *rdata = gcb->context;

	rdata->buf_path = new_buf_path;
	udi_gio_xfer_ack(UDI_MCB(gcb, udi_gio_xfer_cb_t));
}

/* ========================================================================== */

/* 			GIO Client Ops Primary Region			      */

/* ========================================================================== */

/*
 * nsrmap_gio_Tx_client_channel_event:
 * -------------------------------
 * Channel event issued to primary [client] region from secondary Tx region.
 *
 * Calling Context:
 *	GIO meta
 */
STATIC void
nsrmap_gio_Tx_client_channel_event(udi_channel_event_cb_t *cb)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;

	DBG_GIO1(("nsrmap_gio_Tx_client_channel_event(%p), context %p\n",
		  cb, rdata));

	switch (cb->event) {
	case UDI_CHANNEL_BOUND:
		DBG_GIO3(("CHANNEL_BOUND chan = %p\n", cb->gcb.channel));
		rdata->tx_chan = cb->gcb.channel;
		udi_cb_alloc(nsr_gio_chan_cbfn, UDI_GCB(cb),
			     NSRMAP_GIO_BIND_CB_IDX, cb->gcb.channel);
		return;
	default:
		udi_assert(0);
	}
}

/*
 * nsr_gio_chan_cbfn:
 * -----------------
 * Callback for GIO client CHANNEL_BOUND sequence. Issue a udi_gio_bind_req
 * to the appropriate channel and update the rdata->tx_bind_cb field .
 * Channel event completion occurs in the udi_gio_bind_ack callback function.
 *
 * Calling Context:
 *	GIO meta callback from udi_cb_alloc()
 */
STATIC void
nsr_gio_chan_cbfn(udi_cb_t *gcb,
		  udi_cb_t *new_cb)
{
	nsrmap_region_data_t *rdata = gcb->context;
	udi_gio_bind_cb_t *bind_cb = UDI_MCB(new_cb, udi_gio_bind_cb_t);

	DBG_GIO2(("nsr_gio_chan_cbfn(%p, %p)\n", gcb, new_cb));

	/*
	 * Issue a udi_gio_bind_req over the new_cb. This will result in a
	 * udi_gio_bind_ack handshake which allows us to complete this
	 * channel_event_ind process
	 */
	rdata->tx_bind_cb = bind_cb;

	new_cb->initiator_context = gcb;	/* channel_event_cb */
	udi_gio_bind_req(bind_cb);
}

/*
 * nsrmap_gio_bind_ack:
 * -------------------
 * Acknowledgement of udi_gio_bind_req() to secondary region
 *
 * Need to complete the pending udi_channel_event_ind by issuing 
 * udi_channel_event_complete on the stored channel_event_cb
 *
 * Calling context:
 *	GIO meta
 */
STATIC void
nsrmap_gio_bind_ack(udi_gio_bind_cb_t *cb,
		    udi_ubit32_t dev_sz_lo,
		    udi_ubit32_t dev_sz_hi,
		    udi_status_t status)
{
	nsrmap_region_data_t *rdata = cb->gcb.context;
	udi_channel_event_cb_t *ecb = cb->gcb.initiator_context;

	DBG_GIO1(("nsrmap_gio_bind_ack(%p, %d, %d, %d)\n", cb,
		  dev_sz_lo, dev_sz_hi, status));

	/*
	 * Handle CB reallocation across channel operation
	 */
	rdata->tx_bind_cb = cb;


	/*
	 * Allocate xfer_cb and event_cb for the secondary region
	 */
	udi_cb_alloc(nsr_gio_bind_cbfn, UDI_GCB(cb),
		     NSRMAP_GIO_XFER_CB_IDX, ecb->gcb.channel);
}

/*
 * nsr_gio_bind_cbfn:
 * -----------------
 * Callback for CBs allocated after secondary region has acknowledged the
 * udi_gio_bind_req. Save the newly allocated CB in the appropriate location
 * and allocate a buffer for the gio_xfer_cb. 
 *
 * Calling Context:
 *	GIO meta callback from nsrmap_gio_bind_ack()
 */
STATIC void
nsr_gio_bind_cbfn(udi_cb_t *gcb,
		  udi_cb_t *new_cb)
{
	nsrmap_region_data_t *rdata = gcb->context;
	udi_channel_event_cb_t *ecb = gcb->initiator_context;

	DBG_GIO2(("nsr_gio_bind_cbfn(%p, %p)\n", gcb, new_cb));
	/*
	 * Transmit region 
	 */
	if (UDI_HANDLE_IS_NULL(rdata->tx_xfer_cb, udi_gio_xfer_cb_t *)) {
		rdata->tx_xfer_cb = UDI_MCB(new_cb, udi_gio_xfer_cb_t);

		UDI_BUF_ALLOC(nsr_gio_buf_cbfn, gcb, NULL,
			      sizeof (nsrmap_gio_t), rdata->buf_path);
#if 0
		udi_buf_path_alloc(nsr_buf_path_alloc_cbfn, gcb);
#endif
	} else {
		rdata->tx_event_cb = UDI_MCB(new_cb, udi_gio_event_cb_t);

		udi_channel_event_complete(ecb, UDI_OK);
	}
}

/*
 * nsr_gio_buf_cbfn:
 * ----------------
 * Callback for the GIO xfer buffer allocation. Schedule an event_cb_t to be
 * allocated for the appropriate XX_event_cb.
 *
 * Calling Context:
 *	GIO meta, callback from UDI_BUF_ALLOC in nsr_gio_bind_cbfn()
 */
STATIC void
nsr_gio_buf_cbfn(udi_cb_t *gcb,
		 udi_buf_t *new_buf)
{
	nsrmap_region_data_t *rdata = gcb->context;

	DBG_GIO2(("nsr_gio_buf_cbfn(%p, %p)\n", gcb, new_buf));

	udi_assert(rdata->tx_chan == gcb->channel);

	rdata->tx_xfer_cb->data_buf = new_buf;

	/*
	 * Allocate a udi_gio_event_cb_t for our use 
	 */
	udi_cb_alloc(nsr_gio_bind_cbfn, gcb, NSRMAP_GIO_EVENT_CB_IDX,
		     gcb->channel);
}

/*
 * nsrmap_gio_unbind_ack:
 * ---------------------
 * Acknowledgement of udi_gio_unbind_req() to secondary region
 */
STATIC void
nsrmap_gio_unbind_ack(udi_gio_bind_cb_t *cb)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;

	DBG_GIO1(("nsrmap_gio_unbind_ack(%p)\n", cb));

	/*
	 * Release the appropriate CBs for the region 
	 */
	udi_buf_free(rdata->tx_xfer_cb->data_buf);
	udi_cb_free(UDI_GCB(rdata->tx_xfer_cb));
	udi_cb_free(UDI_GCB(rdata->tx_event_cb));
	rdata->tx_event_cb = NULL;

	/*
	 * Release the bind CB 
	 */
	udi_cb_free(UDI_GCB(cb));

	/*
	 * If we have a final_cleanup_req pending, ack it now.   It will
	 * have stored the cb for us as a flag.
	 */
	if (rdata->fcr_cb != NULL) {
		udi_final_cleanup_ack(rdata->fcr_cb);
	}
}

/*
 * nsrmap_gio_xfer_ack:
 * -------------------
 * Acknowledgement of udi_gio_xfer_req() to secondary region
 *
 * This routine is responsible for transitioning the NSR state according to the
 * current NSR_Rx_state, NSR_Tx_state and NSR_state variables.
 *
 * Calling Context:
 *	Primary region [receipt of GIO completion from Tx secondary region]
 */
STATIC void
nsrmap_gio_xfer_ack(udi_gio_xfer_cb_t *cb)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;
	nsrmap_gio_t gio_cmd;
	_udi_nic_state_t *statep, old_state;
	udi_ubit32_t *maskp;
	enum { Transmit, Receive } regionType;
	void (*cbfn) (udi_cb_t *,
		      udi_status_t) = rdata->cbfn;
	udi_cb_t *scratch_cb = rdata->scratch_cb;
	udi_ubit32_t maskval;
	udi_index_t ml_idx;
	udi_time_t tval;		/* LINK-UP timer */
	udi_nic_bind_cb_t *bind_cb;	/* CB for udi_nd_bind_req */

	DBG_GIO1(("nsrmap_gio_xfer_ack(%p)\n", cb));

	udi_buf_read(cb->data_buf, 0, sizeof (gio_cmd), &gio_cmd);

	BCLR(rdata->NSR_Tx_state, NsrmapS_TxCmdActive);

	/*
	 * Restore our internal CB handles to cater for CB reallocation across
	 * a channel-op
	 */
	regionType = Transmit;
	rdata->tx_xfer_cb = cb;
	statep = &rdata->Tx_State;
	maskp = &rdata->NSR_Tx_state;

	switch (gio_cmd.cmd) {
	case NSR_GIO_SET_STATE:
		/*
		 * Nothing to do except update rdata->State if both regions
		 * have completed their state transitions
		 */
		rdata->Tx_State = gio_cmd.un.st;
		old_state = rdata->State;
		if (rdata->Rx_State == rdata->Tx_State) {
			rdata->State = rdata->Tx_State;
		}
		DBG_GIO3(("<=NSR_GIO_SET_STATE: Old %s, New %s\n",
			  state_names[old_state], state_names[rdata->State]));
		if (old_state != rdata->State) {
			switch (rdata->State) {
			case BOUND:
				switch (old_state) {
				case UNBOUND:
				case BINDING:
					/*
					 * Callout to osnsr_bind_done() to make
					 * the device accessible
					 */
					osnsr_bind_done(UDI_MCB
							(rdata->bind_info.
							 bind_cb,
							 udi_channel_event_cb_t),
							rdata->bind_status);

					break;
				case ACTIVE:
				case ENABLED:
					/*
					 * Return all outstanding Tx CBs to
					 * the ND and recycle the Rx CBs
					 */
					nsr_commit_Tx_cbs(rdata);
#if 0
					nsr_recycle_Rx_q(rdata);
#endif
					break;
				default:
					break;
				}
				break;
			case ENABLED:
				switch (old_state) {
				case BOUND:
					/*
					 * Inform ND of available Rx CBs 
					 */
					nsr_commit_Rx_cbs(rdata);
					/*
					 * Start a timeout chain to
					 * complete the NSR_enable if the link
					 * doesn't become active. This happens
					 * if we open the NIC when the cable is
					 * not present. The timeout is ignored
					 * if NSR_LINKUP_SECS and 
					 * NSR_LINKUP_NSECS are both zero
					 */
					tval.seconds = NSR_LINKUP_SECS;
					tval.nanoseconds = NSR_LINKUP_NSECS;
					if (tval.seconds != 0 ||
					    tval.nanoseconds != 0) {
						BSET(rdata->NSR_state,
						     NsrmapS_LinkTimerActive);
						udi_timer_start
							(nsrmap_fake_linkup,
							 rdata->link_timer_cb,
							 tval);
					}
					break;
				default:
					break;
				}
				break;
			case ACTIVE:
				switch (old_state) {
				case ENABLED:
					/*
					 * Inform ND of available Rx CBs 
					 */
					nsr_commit_Rx_cbs(rdata);
					/*
					 * Unblock user NSR_enable thread 
					 */
					if (!UDI_HANDLE_IS_NULL(cbfn, void *)
					    && !UDI_HANDLE_IS_NULL(scratch_cb,
								   udi_cb_t *)) {
						rdata->cbfn = NULL;
						rdata->scratch_cb = NULL;
						(*cbfn) (scratch_cb, UDI_OK);
					}
					break;
				default:
					break;
				}
				break;
			case UNBINDING:
				switch (old_state) {
				case BOUND:
					nsr_change_state(rdata, UNBOUND);
					break;
				default:
					break;
				}
				break;
			case UNBOUND:
				switch (old_state) {
				case UNBINDING:
					/*
					 * Release all resources 
					 */
					nsr_release_resources(rdata);
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
		}
		break;
	case NSR_GIO_SET_CHANNEL:
		/*
		 * Region has successfully anchored its data channel.
		 * Update the appropriate bitmask and when both are anchored
		 * instigate a udi_nd_bind_req
		 */
		DBG_GIO3(("<=NSR_GIO_SET_CHANNEL\n"));
		BSET(rdata->NSR_Tx_state, NsrmapS_Anchored);
		if (BTST(rdata->NSR_Rx_state, NsrmapS_Anchored) &&
		    BTST(rdata->NSR_Tx_state, NsrmapS_Anchored)) {
			BSET(rdata->NSR_state, NsrmapS_Anchored);
			/*
			 * rdata->bind_cb is our udi_nic_bind_cb_t to use....
			 * NOTE: This may change across the channel op, 
			 * so reset it on callback.
			 * This is where we can specify which capabilites the
			 * NSR is going to use (currently only
			 * UDI_NIC_CAP_USE_RX_CKSUM is defined). This allows for
			 * the ND to optimize its setup.
			 *
			 * Step 5 in CHANNEL_BOUND processing
			 */
			bind_cb = UDI_MCB(rdata->bind_cb, udi_nic_bind_cb_t);

			bind_cb->capabilities = rdata->capabilities;
			udi_nd_bind_req(bind_cb, NSR_TX_CHAN, NSR_RX_CHAN);
		}
		break;
	case NSR_GIO_ALLOC_RESOURCES:
		DBG_GIO3(("<=NSR_GIO_ALLOC_RESOURCES\n"));
		/*
		 * Region has successfully allocated its resources [Rx only].
		 * Update the state and then issue a state change to BOUND.
		 * On completion of this we will call the osnsr_bind_done()
		 * to allow access to the device.
		 */
		BSET(rdata->NSR_Tx_state, NsrmapS_AllocDone);
		if (BTST(rdata->NSR_Rx_state, NsrmapS_AllocDone) &&
		    BTST(rdata->NSR_Tx_state, NsrmapS_AllocDone)) {
			BSET(rdata->NSR_state, NsrmapS_AllocDone);
			nsr_change_state(rdata, BOUND);
		}
		break;
	case NSR_GIO_COMMIT_CBS:
		DBG_GIO3(("<=NSR_GIO_COMMIT_CBS\n"));
		/*
		 * If we're in the BOUND state and we're disabling, we have 
		 * just sent all the outstanding Tx CBs back to the ND. 
		 * We can now complete the blocked user NSR_disable() call
		 */
		if (BTST(rdata->NSR_state, NsrmapS_Disabling)) {
			BCLR(rdata->NSR_state, NsrmapS_Disabling);
			DBG_GIO3(("Transmit Disable complete\n"));
			if (!UDI_HANDLE_IS_NULL(cbfn, void *) &&
			    !UDI_HANDLE_IS_NULL(scratch_cb,
						udi_cb_t *)) {
				rdata->cbfn = NULL;
				rdata->scratch_cb = NULL;
				(*cbfn) (scratch_cb, UDI_OK);
			}
		}
		break;
	case NSR_GIO_SET_TRACEMASK:
		DBG_GIO3(("<=NSR_GIO_SET_TRACEMASK\n"));
		/*
		 * Need to propagate all meta-language tracemasks to the
		 * secondary regions. The meta_idx gives us the one we've just
		 * completed, so we update the flags with the appropriate state
		 * and set the next tracemask. When all have been completed we
		 * finish the cycle.
		 */
		ml_idx = gio_cmd.un.tr.meta_idx;
		switch (ml_idx) {
		case 0:
			maskval = NsrmapS_MgmtTraceSet;
			break;
		case NSRMAP_NIC_META:
			maskval = NsrmapS_NicTraceSet;
			break;
		case NSRMAP_GIO_META:
			maskval = NsrmapS_GioTraceSet;
			break;
		default:
			udi_assert(0);
			maskval = 0;
			break;
		}
		BSET(*maskp, maskval);
		if (BTST(rdata->NSR_Rx_state, maskval) &&
		    BTST(rdata->NSR_Tx_state, maskval)) {
			BSET(rdata->NSR_state, maskval);
			BCLR(rdata->NSR_Rx_state, maskval);
			BCLR(rdata->NSR_Tx_state, maskval);
			if (BTST(rdata->NSR_state, NsrmapS_MgmtTraceSet) &&
			    BTST(rdata->NSR_state, NsrmapS_GioTraceSet) &&
			    BTST(rdata->NSR_state, NsrmapS_NicTraceSet)) {
				/*
				 * Propagated all tracemasks. Reset our state
				 * so that subsequent usage_ind's will also
				 * flow through to the secondary regions
				 */
				BCLR(rdata->NSR_state, (NsrmapS_MgmtTraceSet |
							NsrmapS_GioTraceSet |
							NsrmapS_NicTraceSet));
			} else {
				/*
				 * Propagate next meta-language mask
				 */
				nsr_set_tracemask(rdata, ml_idx + 1,
						  rdata->ml_tracemask[ml_idx +
								      1]);
			}
		}
		break;
	case NSR_GIO_FREE_RESOURCES:
		DBG_GIO3(("<=NSR_GIO_FREE_RESOURCES\n"));
		/*
		 * We are tearing down the whole instance, close the appropriate
		 * channel to the secondary region as no further operations will
		 * be scheduled.
		 */
		BCLR(rdata->NSR_Rx_state, NsrmapS_AllocDone);
		BCLR(rdata->NSR_state, NsrmapS_AllocDone);
		BCLR(rdata->NSR_Tx_state, NsrmapS_AllocDone);
		udi_gio_unbind_req(rdata->tx_bind_cb);
		break;
	default:
		DBG_GIO3(("UNKNOWN OP-CODE %d\n", gio_cmd.cmd));
		break;
	}
}

/*
 * nsrmap_gio_xfer_nak:
 * -------------------
 * Negative acknowledgement of udi_gio_xfer_req() to secondary region
 */
STATIC void
nsrmap_gio_xfer_nak(udi_gio_xfer_cb_t *cb,
		    udi_status_t status)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;
	nsrmap_gio_t gio_cmd;

	DBG_GIO1(("nsrmap_gio_xfer_nak(%p, %d)\n", cb, status));

	/*
	 * Handle CB reallocation 
	 */
	rdata->tx_xfer_cb = cb;

	udi_buf_read(cb->data_buf, 0, sizeof (gio_cmd), &gio_cmd);

	BCLR(rdata->NSR_Tx_state, NsrmapS_TxCmdActive);

	switch (gio_cmd.cmd) {
	case NSR_GIO_ALLOC_RESOURCES:
		/*
		 * Fatal -- we're out of memory so we must fail any device
		 *          access
		 *
		 * TODO: Log this failure
		 */
		rdata->bind_status = status;
		osnsr_bind_done(UDI_MCB(rdata->bind_info.bind_cb,
					udi_channel_event_cb_t),
				status);

		break;
	}
}

/*
 * nsrmap_gio_event_ind:
 * --------------------
 * Asynchronous event indication from secondary region
 */
STATIC void
nsrmap_gio_event_ind(udi_gio_event_cb_t *cb)
{
	DBG_GIO1(("nsrmap_gio_event_ind(%p)\n", cb));
}

/*
 * nsrmap_fake_linkup:
 * ------------------
 * Timeout routine to fake a LINK-UP status indication. This will complete a
 * blocked OS NSRmap_enable call if necessary.
 */
STATIC void
nsrmap_fake_linkup(udi_cb_t *gcb)
{
	nsrmap_region_data_t *rdata = gcb->context;
	void (*cbfn) (udi_cb_t *,
		      udi_status_t) = rdata->cbfn;
	udi_cb_t *scratch_cb = rdata->scratch_cb;

	BCLR(rdata->NSR_state, NsrmapS_LinkTimerActive);

	DBG_GIO1(("nsrmap_fake_linkup: Completing deferred open\n"));

	if (!UDI_HANDLE_IS_NULL(cbfn, void *) &&
	    !UDI_HANDLE_IS_NULL(scratch_cb,
				udi_cb_t *)) {
		rdata->cbfn = NULL;
		rdata->scratch_cb = NULL;
		(*cbfn) (scratch_cb, UDI_OK);
	}
}

/* ========================================================================== */

/*			Inter-region Command Despatch			      */

/* ========================================================================== */

/*
 * nsr_change_state:
 * ----------------
 * Issue a NSR_GIO_SET_STATE command to both Rx and Tx secondary regions.
 * The [state] parameter contains the new state to be set.
 */
STATIC void
nsr_change_state(nsrmap_region_data_t * rdata,
		 _udi_nic_state_t state)
{
	nsrmap_gio_t *gio_cmdp = rdata->gio_cmdp;

	gio_cmdp->cmd = NSR_GIO_SET_STATE;
	gio_cmdp->un.st = state;

	DBG_GIO2(("nsr_change_state( %p, %s )\n", rdata, state_names[state]));

	rdata->Rx_State = state;
	nsr_send_region(rdata, NSRMAP_TX_REG_IDX, NULL, gio_cmdp);
}

/*
 * nsr_alloc_Tx_resources:
 * ----------------------
 * Issue a NSR_GIO_ALLOC_RESOURCES command to the Tx secondary region.
 * This simply sets the buffer size for use by the OS transmit routines, [ncbs]
 * is ignored
 */
STATIC void
nsr_alloc_Tx_resources(nsrmap_region_data_t * rdata,
		       udi_ubit32_t ncbs,	/* Unused */

		       udi_ubit32_t size)
{
	nsrmap_gio_t *gio_cmdp = rdata->gio_cmdp;

	gio_cmdp->cmd = NSR_GIO_ALLOC_RESOURCES;
	gio_cmdp->un.res.n_cbs = ncbs;
	gio_cmdp->un.res.max_size = size;

	DBG_GIO2(("nsr_alloc_Tx_resources( %p, %d, %d )\n", rdata, ncbs,
		  size));

	nsr_send_region(rdata, NSRMAP_TX_REG_IDX, NULL, gio_cmdp);
}

/*
 * nsr_release_resources:
 * ---------------------
 * Issue NSR_GIO_FREE_RESOURCES to both Tx and Rx regions. This is called as
 * part of the tear-down process. On completion of the request we unbind
 * the secondary region(s).
 */
STATIC void
nsr_release_resources(nsrmap_region_data_t * rdata)
{
	nsrmap_gio_t *gio_cmdp = rdata->gio_cmdp;
	udi_queue_t *elem;
	nsr_cb_t *pcb;
	udi_nic_rx_cb_t *rxcb;

	gio_cmdp->cmd = NSR_GIO_FREE_RESOURCES;

	DBG_GIO2(("nsr_release_resources( %p )\n", rdata));

	/*
	 * Release the Rx resources
	 */

	DBG_GIO3(("# Rx cbq = %d\n", NSR_UDI_QUEUE_SIZE(&rdata->rxcbq)));
	while ((elem = NSR_UDI_DEQUEUE_HEAD(&rdata->rxcbq)) != NULL) {
		pcb = (nsr_cb_t *) elem;
		rxcb = UDI_MCB(pcb->cbp, udi_nic_rx_cb_t);

		if (rxcb) {
			udi_buf_free(rxcb->rx_buf);
			rxcb->rx_buf = NULL;
			udi_cb_free(pcb->cbp);
			pcb->cbp = NULL;
		}
		udi_mem_free(pcb);
	}

	/*
	 * Handle case where the ND has not returned 
	 * previously allocated Rx CBs to us. The ND
	 * _must_ have released the buffer and CB, all we
	 * need to do is to free the enclosing memory.
	 */
	DBG_GIO3(("# Rx onq = %d\n", NSR_UDI_QUEUE_SIZE(&rdata->rxonq)));
	while ((elem = NSR_UDI_DEQUEUE_HEAD(&rdata->rxonq)) != NULL)
		udi_mem_free(elem);

	BCLR(rdata->NSR_Rx_state, NsrmapS_AllocDone);

	nsr_send_region(rdata, NSRMAP_TX_REG_IDX, NULL, gio_cmdp);
}

/*
 * nsr_commit_Rx_cbs:
 * -----------------
 * Tell the Rx region to pass any allocated udi_nic_rx_cb_t's to the ND
 */
STATIC void
nsr_commit_Rx_cbs(nsrmap_region_data_t * rdata)
{
	DBG_GIO2(("nsr_commit_Rx_cbs( %p )\n", rdata));

	nsrmap_commit_cbs(rdata);
}

/*
 * nsr_commit_Tx_cbs:
 * -----------------
 * Tell the Tx region to return any unused udi_nic_tx_cb_t's to the ND. This
 * happens when the NSR has issued a udi_nd_disable_req and the state has
 * moved to BOUND from ENABLED/ACTIVE
 */
STATIC void
nsr_commit_Tx_cbs(nsrmap_region_data_t * rdata)
{
	nsrmap_gio_t *gio_cmdp = rdata->gio_cmdp;

	gio_cmdp->cmd = NSR_GIO_COMMIT_CBS;

	DBG_GIO2(("nsr_commit_Tx_cbs( %p )\n", rdata));

	nsr_send_region(rdata, NSRMAP_TX_REG_IDX, NULL, gio_cmdp);
}

/*
 * nsr_set_tracemask:
 * -----------------
 * Issue a NSR_GIO_SET_TRACEMASK command to both Tx and Rx secondary regions.
 */
STATIC void
nsr_set_tracemask(nsrmap_region_data_t * rdata,
		  udi_ubit32_t meta_idx,
		  udi_ubit32_t tracemask)
{
	nsrmap_gio_t *gio_cmdp = rdata->gio_cmdp;

	gio_cmdp->cmd = NSR_GIO_SET_TRACEMASK;
	gio_cmdp->un.tr.meta_idx = meta_idx;
	gio_cmdp->un.tr.tracemask = tracemask;

	DBG_GIO2(("nsr_set_tracemask( %p, %d, %p )\n", rdata, meta_idx,
		  tracemask));

	nsr_send_region(rdata, NSRMAP_TX_REG_IDX, NULL, gio_cmdp);

}

/*
 * nsr_send_region:
 * ---------------
 * Send a command to the specified region [reg_idx] using the 
 * primary<->secondary udi_gio_xfer_cb_t control block.
 * [cbfn] contains the next function to execute if non-NULL.
 *
 * Calling Context:
 *	NSR Primary Region
 */

STATIC void
nsr_send_region(nsrmap_region_data_t * rdata,
		udi_ubit32_t reg_idx,
		void (*cbfn) (udi_cb_t *),
		nsrmap_gio_t * cmdp)
{
	DBG_GIO2(("nsr_send_region(%p, %d, %p, %p)\n", rdata, reg_idx, cbfn,
		  cmdp));
	switch (reg_idx) {
	case NSRMAP_TX_REG_IDX:
		/*
		 * This is a pathological condition. Normally the inter-region
		 * request should have completed by now. If it hasn't we have
		 * already modified the cmdp contents so the best bet is to
		 * bail out.
		 */
		if (BTST(rdata->NSR_Tx_state, NsrmapS_TxCmdActive)) {
			udi_debug_printf("nsr_send_region: Tx region busy!\n");
			return;
		}
		udi_assert(!BTST(rdata->NSR_Tx_state, NsrmapS_TxCmdActive));
		BSET(rdata->NSR_Tx_state, NsrmapS_TxCmdActive);
		if (!UDI_HANDLE_IS_NULL(rdata->tx_xfer_cb, udi_cb_t)) {
			rdata->tx_xfer_cb->data_buf->buf_size = sizeof (*cmdp);
			rdata->next_func = cbfn;
			udi_buf_write(_nsrmap_tx_cmd_cbfn,
				      UDI_GCB(rdata->tx_xfer_cb),
				      cmdp, sizeof (*cmdp),
				      rdata->tx_xfer_cb->data_buf,
				      0, sizeof (*cmdp), UDI_NULL_BUF_PATH);
		} else {
			BCLR(rdata->NSR_Tx_state, NsrmapS_TxCmdActive);
		}
		break;

	default:
		udi_assert(0);
	}
}

/*
 * ---------------------------------------------------------------------------
 * Internal allocation routines
 */


/* ========================================================================== */

/*			Transmit Allocation Routines			      */

/*									      */

/* nsrmap_got_txmem							      */

/* ========================================================================== */

/*
 * nsrmap_got_txmem:
 * ----------------
 * Callback routine for internal Tx CB queue element. Schedule another 
 * allocation if we haven't exhausted the number of posted udi_nic_tx_cb_t's
 * from the ND.
 *
 * Calling Context:
 *	Transmit region, NIC meta
 */
STATIC void
nsrmap_got_txmem(udi_cb_t *gcb,
		 void *new_mem)
{
	nsrmap_Tx_region_data_t *rdata = gcb->context;
	nsr_cb_t *pcb = new_mem;
	udi_nic_tx_cb_t *nextcb;

	NSR_UDI_ENQUEUE_TAIL(&rdata->cbq, (udi_queue_t *)pcb);

	/*
	 * Store the ND supplied tx_cb in our queue-element's cbp field.
	 * We break the chain of tx_cbs so that we can more easily handle
	 * arbitrary sized requests
	 */
	NSRMAP_ASSERT(rdata->ND_txcb != NULL);

	if (rdata->ND_txcb == NULL) {
		DBG_TX3(("nsrmap_got_txmem NULL ND_txcb, State %s\n",
			 state_names[rdata->State]));
		pcb->cbp = NULL;	/* Check This */
	} else {
		nextcb = rdata->ND_txcb->chain;
		rdata->ND_txcb->chain = NULL;
		pcb->cbp = UDI_GCB(rdata->ND_txcb);
		rdata->ND_txcb = nextcb;
	}

	if (rdata->ND_txcb) {
		/*
		 * More outstanding Tx CBs to process. Get more memory 
		 */
		udi_mem_alloc(nsrmap_got_txmem, rdata->my_cb, sizeof (nsr_cb_t), 0);	/* Zero fill */
	} else {
		DBG_TX3(("nsrmap_got_txmem: Finished alloc %d on list\n",
			 NSR_UDI_QUEUE_SIZE(&rdata->cbq)));
		BSET(rdata->NSR_state, NsrmapS_AllocDone);
		BCLR(rdata->NSR_state, NsrmapS_AllocPending);
		if (BTST(rdata->NSR_state, NsrmapS_CommitWanted)) {
			nsr_commit_tx_cbs(rdata);
		}
	}
}

/* ========================================================================== */

/*			Receive Allocation Routines			      */

/*									      */

/*	nsrmap_got_rxmem						      */

/*	nsrmap_got_rxcb							      */

/*	nsrmap_got_rxbuf						      */

/* ========================================================================== */

/*
 * nsrmap_got_rxmem:
 * ----------------
 * Callback routine for internal Rx CB queue element. We need to allocate
 * one udi_cb_t for each queue element and also allocate a udi_buf_t for use
 * in the data transfer case
 *
 * Calling Context:
 *	Primary Rx region
 */
STATIC void
nsrmap_got_rxmem(udi_cb_t *gcb,
		 void *new_mem)
{
	nsrmap_region_data_t *rdata = gcb->context;
	nsr_cb_t *pcb = new_mem;

	NSR_UDI_ENQUEUE_HEAD(&rdata->rxcbq, (udi_queue_t *)pcb);


	if (NSR_UDI_QUEUE_SIZE(&rdata->rxcbq) <
	    rdata->link_info.rx_hw_threshold) {
		udi_mem_alloc(nsrmap_got_rxmem, gcb, sizeof (nsr_cb_t), 0);
		return;
	}
	/*
	 * we have allocated enough "hangers" for our cbs
	 * now allocate the cbs
	 */
	NSRMAP_CB_ALLOC_BATCH(nsrmap_got_cb_batch, gcb, rdata);
}

/*
 * nsrmap_got_cb_batch:
 * ----------------
 * Callback for a batch of udi_nic_rx_cb_t with data buffer. 
 * Set all the buffer size to 0. 
 * then we inform the Tx region of the maximum packet
 * size and let it drive the completion of the bind request.
 *
 * Calling Context:
 *	Primary (Rx) region
 */
STATIC void
nsrmap_got_cb_batch(udi_cb_t *gcb,
		    udi_cb_t *first_cb)
{
	nsrmap_region_data_t *rdata = gcb->context;
	nsr_cb_t *pcb;
	udi_nic_rx_cb_t *rxcb;
	udi_queue_t *elem, *tmp;
	_nsr_defer_op_t *op;

	/*
	 * hang the cbs onto the previously allocated hangers
	 * TODO: exmaine the necessity of the hangers 
	 */
	rxcb = UDI_MCB(first_cb, udi_nic_rx_cb_t);

	UDI_QUEUE_FOREACH(&rdata->rxcbq.Q, elem, tmp) {
		pcb = UDI_BASE_STRUCT(elem, nsr_cb_t, q);
		pcb->rdatap = rdata;
		pcb->cbp = UDI_GCB(rxcb);
		pcb->buf = rxcb->rx_buf;
		/*
		 * Set buf_size to 0. ND must 
		 * update buf_size _before_ placing data into
		 * the buffer. This avoids another allocation 
		 * when updating the contents
		 */
		pcb->buf->buf_size = 0;
		pcb->scratch = UDI_GCB(rxcb)->scratch;
		rxcb = rxcb->chain;
	}


	DBG_RX3(("nsrmap_got_rxbuf: FINISHED ALLOCATION\n"));
	/*
	 * Tell the Tx region the maximum buffer size that the ND will
	 * accept if we're in BINDING state. Otherwise, just commit the
	 * newly allocated resources to the ND
	 */
	BSET(rdata->NSR_Rx_state, NsrmapS_AllocDone);
	BCLR(rdata->NSR_Rx_state, NsrmapS_AllocPending);
	switch (rdata->State) {
	case BINDING:
		nsr_alloc_Tx_resources(rdata, 0,
				       rdata->link_info.max_pdu_size);
		break;
	case UNBINDING:
	case UNBOUND:
		/*
		 * Unstall any command received while allocation was
		 * in progress. This is typically the DMGMT_UNBIND
		 */
		op = (_nsr_defer_op_t *) NSR_UDI_DEQUEUE_HEAD(&rdata->deferq);
		if (op)
			(*(op->func)) (op->cb);

		break;
	case ENABLED:
	case ACTIVE:
		nsr_commit_Rx_cbs(rdata);
		break;
	case BOUND:
		break;
	default:
		/*
		 * Shouldn't happen 
		 */
		udi_assert(0);
	}
}

#ifndef NSR_USE_SERIALISATION

/*
 * Queue access mechanisms to ensure OS and UDI can both access the same queues
 * without corruption. These are only used if the OS mapper does not make use
 * of UDI serialisation [by queueing requests on a regions chain of outstanding
 * CBs]. These functions are expensive.
 */
STATIC udi_queue_t *
_nsr_udi_dequeue_head(_nsr_q_head * qhd)
{
	udi_queue_t *retval;

	_OSDEP_SIMPLE_MUTEX_LOCK(&(qhd)->lock);
	if (qhd->numelem) {
		qhd->numelem--;
		retval = UDI_DEQUEUE_HEAD(&(qhd)->Q);
	} else
		retval = (udi_queue_t *)NULL;
	_OSDEP_SIMPLE_MUTEX_UNLOCK(&(qhd)->lock);
	return retval;
}

STATIC udi_sbit32_t
_nsr_udi_queue_empty(_nsr_q_head * qhd)
{
	udi_sbit32_t retval;

	_OSDEP_SIMPLE_MUTEX_LOCK(&(qhd)->lock);
	retval = qhd->numelem == 0;
	_OSDEP_SIMPLE_MUTEX_UNLOCK(&(qhd)->lock);
	return retval;
}

STATIC udi_sbit32_t
_nsr_udi_queue_size(_nsr_q_head * qhd)
{
	udi_sbit32_t retval;

	_OSDEP_SIMPLE_MUTEX_LOCK(&(qhd)->lock);
	retval = qhd->numelem;
	_OSDEP_SIMPLE_MUTEX_UNLOCK(&(qhd)->lock);
	return retval;
}
#endif /* NSR_USE_SERIALISATION */
