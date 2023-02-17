
/*
 * File:  pseudond/pseudond.c
 *
 * UDI Prototype Pseudo Driver.   Uses no hardware to generate and
 * consume test streams of data.
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
 * =============================================================================
 * This driver provides a pseudo network device driver. The following diagram
 * shows the channel connectivity between the UDI environment and this
 * driver:
 *
 *				______________			
 *				|	     |			
 *				|  NSR	     |			
 *				|	     |			
 *				--^-----^--^--	[Child]		
 *				  |	|  |			
 *				 3|    4| 5|			
 *				  |	|  |			
 *	__________		__v_____v__v__ 	[Parent]	
 *	|        |	        |	     |			
 *	| Mgmt	 |<------------>| Pseudo ND  |<------------>	
 *	| Agent  |       1      |	     |	     2		
 *	----------		--^-----------			
 *				  |
 *				 6|
 *				  |
 *				  v
 *
 * Channel Definitions:
 * -------------------
 *	1:	Management MetaLanguage channel -- controls driver configuration
 *			and operating conditions; driven by Management Agent
 *	2:	GIO Metalanguage channel	-- allows diagnostic and
 *			bespoke operations to be sent to driver; 
 *			set by udi_prep_for_child_req()
 *	3:	Network Metalanguage control channel	-- controls network
 *			stack connection, set by udi_prep_for_child_req()
 *	4:	Network Metalanguage transmit channel	-- controls data flow
 *			to the ND, spawned from (3)
 *	5:	Network Metalanguage receive channel	-- controls data flow
 *			from the ND, spawned from (3)
 *	6:	Bus Bridge channel		-- used to enumerate device
 *
 * Note:
 *	Both (2) and (3) will be established, but it is not permissible to
 *	have both the GIO and NSR metas simultaneously attempt data transfer
 * -----------------------------------------------------------------------------
 */

#define UDI_VERSION 0x101
#define UDI_NIC_VERSION 0x101
#define	UDI_PHYSIO_VERSION 0x101

#include <udi.h>         		/* Standard UDI definitions */
#include <udi_nic.h>	 		/* Network Interface Meta definitions */
#include <udi_physio.h>			/* Physical I/O Extensions */

#define	Private_visible			/* For our own use */
#include "pseudond.h"			/* Driver specific definitions */

/*
 * User-modifiable definitions (for Posix test environment)
 */
#ifndef __PSEUDOND_MEDIA_TYPE
#define	__PSEUDOND_MEDIA_TYPE	UDI_NIC_FASTETHER
#endif	/* __PSEUDOND_MEDIA_TYPE */
#ifndef __PSEUDOND_MEDIA_SPEED
#define	__PSEUDOND_MEDIA_SPEED	10000000	/* Default 10Mbps */
#endif	/* __PSEUDOND_MEDIA_SPEED */

#ifndef __PSEUDOND_CTRL_SCRATCH_SZ
#define	__PSEUDOND_CTRL_SCRATCH_SZ	0
#endif	/* __PSEUDOND_CTRL_SCRATCH_SZ */
#if (__PSEUDOND_CTRL_SCRATCH_SZ - 0) == 0
#undef __PSEUDOND_CTRL_SCRATCH_SZ
#define	__PSEUDOND_CTRL_SCRATCH_SZ	0
#endif	/* (__PSEUDOND_CTRL_SCRATCH_SZ - 0) == 0 */

#define	PSEUDO_GIO_INTERFACE	1	/* ops_idx for GIO */
#define	PSEUDO_NET_CTRL_IDX	2	/* ops_idx for control ops */
#define	PSEUDO_NET_TX_IDX	3	/* ops_idx for transmit ops */
#define	PSEUDO_NET_RX_IDX	4	/* ops_idx for receive ops */

#define	PSEUDO_NET_PARENT_IDX	5	/* For parent of this driver [bridge]*/

/*
 * Control Block Indices for GIO and NET metalanguage
 */
#define	PSEUDO_GIO_BIND_CB_IDX	1	/* GIO bind cb_idx */
#define	PSEUDO_GIO_XFER_CB_IDX	2	/* GIO xfer cb_idx */
#define	PSEUDO_GIO_EVENT_CB_IDX	3	/* GIO event cb_idx */

#define	PSEUDO_NET_CTRL_CB_IDX	4	/* Control cb_idx */
#define	PSEUDO_NET_TX_CB_IDX	5	/* Transmit cb_idx */
#define	PSEUDO_NET_RX_CB_IDX	6	/* Receive cb_idx */
#define	PSEUDO_NET_STD_CB_IDX	7	/* Standard net cb_idx */
#define	PSEUDO_NET_STATUS_CB_IDX 8	/* Status cb_idx */
#define	PSEUDO_NET_INFO_CB_IDX	9	/* Info cb_idx */
#define	PSEUDO_NET_BIND_CB_IDX	10	/* Bind cb_idx */

#define	PSEUDO_BRIDGE_BIND_CB_IDX 11	/* Bus-bridge bind cb_idx */

/*
 * State names for Meta_state
 */
static char	* const state_names[] = {
	"UNBOUND",
	"GIO_BOUND",
	"BINDING",
	"BOUND",
	"ENABLED",
	"ACTIVE",
	"DISABLING",
	"UNBINDING",
	"ILLEGAL"
};

static char	* const op_names[] = {
	"udi_nd_enable_req",
	"udi_nd_unbind_req"
};

/*
 *---------------------------------------------------------------------------
 * Management Meta Language entrypoints
 *---------------------------------------------------------------------------
 */
STATIC udi_usage_ind_op_t		pseudo_nd_usage_ind;
STATIC udi_enumerate_req_op_t		pseudo_nd_enumerate_req;
STATIC udi_devmgmt_req_op_t		pseudo_nd_devmgmt_req;
STATIC udi_final_cleanup_req_op_t	pseudo_nd_final_cleanup_req;

#if 0 || LATER
/*
 *---------------------------------------------------------------------------
 * Generic I/O Metalanguage entrypoints
 *---------------------------------------------------------------------------
 */
STATIC udi_channel_event_ind_op_t	pseudond_gio_channel_event;
STATIC udi_gio_bind_req_op_t		pseudond_gio_bind_req;
STATIC udi_gio_unbind_req_op_t		pseudond_gio_unbind_req;
STATIC udi_gio_xfer_req_op_t		pseudond_gio_xfer_req;
#if 0 || OLDWAY
STATIC udi_gio_abort_req_op_t		pseudond_gio_abort_req;
#endif	/* 0 || OLDWAY */
STATIC udi_gio_event_res_op_t		pseudond_gio_event_res;
#endif	/* 0 || LATER */

/*
 *---------------------------------------------------------------------------
 * Network Meta Language entrypoints
 *---------------------------------------------------------------------------
 */
/* Control Operations */
STATIC udi_channel_event_ind_op_t	pseudo_nd_channel_event;
STATIC udi_nd_bind_req_op_t		pseudo_nd_bind_req;
STATIC udi_nd_unbind_req_op_t		pseudo_nd_unbind_req;
STATIC udi_nd_enable_req_op_t		pseudo_nd_enable_req;
STATIC udi_nd_disable_req_op_t		pseudo_nd_disable_req;
STATIC udi_nd_ctrl_req_op_t		pseudo_nd_ctrl_req;
STATIC udi_nd_info_req_op_t		pseudo_nd_info_req;

/* Data Transfer Operations */
STATIC udi_nd_tx_req_op_t		pseudo_nd_tx_req;
STATIC udi_nd_exp_tx_req_op_t		pseudo_nd_exp_tx_req;
STATIC udi_nd_rx_rdy_op_t		pseudo_nd_rx_rdy;

void Pseudo_ND_Alloc_Resources( Pseudo_Card_t * );
void Pseudo_ND_Free_Resources( Pseudo_Card_t * );
void Pseudo_ND_Alloc_Resources_Done( Pseudo_Card_t *, udi_status_t );

/*
 *-----------------------------------------------------------------------------
 * Bus Bridge Metalanguage entrypoints
 *-----------------------------------------------------------------------------
 */
STATIC udi_channel_event_ind_op_t	pseudond_bus_channel_event;
STATIC udi_bus_bind_ack_op_t		pseudond_bus_bind_ack;
STATIC udi_bus_unbind_ack_op_t		pseudond_bus_unbind_ack;
 
/*
 *-----------------------------------------------------------------------------
 * Support routines
 *-----------------------------------------------------------------------------
 */
STATIC udi_mem_alloc_call_t	pseudo_nd_bind_complete;
STATIC udi_channel_spawn_call_t	pseudo_nd_tx_anchored;
STATIC udi_channel_spawn_call_t	pseudo_nd_rx_anchored;
STATIC udi_cb_alloc_call_t	pseudo_nd_bind_step1;
STATIC udi_cb_alloc_call_t	pseudo_nd_bind_cbfn;
STATIC udi_cb_alloc_call_t	pseudo_nd_rxalloc_cbfn;
STATIC udi_cb_alloc_call_t	pseudo_nd_got_txcb;
STATIC udi_mem_alloc_call_t	pseudo_nd_got_rxmem;
STATIC udi_mem_alloc_call_t	pseudo_nd_got_txmem;
STATIC udi_mem_alloc_call_t	pseudo_nd_gotcb;

STATIC void	pseudo_nd_free_rsrc( Pseudo_Card_t * );
STATIC void	pseudo_nd_getnext_rsrc( Pseudo_Card_t * );

STATIC void	pseudo_nd_transmit( Pseudo_Card_t * );
STATIC void	pseudo_nd_receive( Pseudo_Card_t * );

STATIC udi_buf_write_call_t	pseudo_nd_rxbuf;

STATIC udi_cb_alloc_call_t	pseudo_nd_status;

STATIC udi_mem_alloc_call_t	pseudo_nd_unbind_step1;
STATIC udi_mem_alloc_call_t	pseudo_nd_enable_step1;

STATIC void	pseudo_nd_defer( udi_cb_t * );
STATIC void	pseudo_nd_stat_trans( udi_cb_t * );

STATIC udi_buf_write_call_t	pseudo_nd_ctrl_req1;

/*
 * Global data
 * XXX: Violates the UDI model.
 */
static udi_ubit32_t	nPseudoNDs;

/*
 * Management Metalanguage/Pseudo Driver Entrypoints
 */

static udi_mgmt_ops_t  pseudond_mgmt_ops = {
	pseudo_nd_usage_ind, 		/* usage_ind_op */
	pseudo_nd_enumerate_req, 	/* enumerate_req_op */
	pseudo_nd_devmgmt_req,		/* devmgmt_req_op */
	pseudo_nd_final_cleanup_req	/* final_cleanup_req_op */
};

#if 0 || LATER
/*
 * GIO Metalanguage/Pseudo Driver Entrypoints
 */
static udi_gio_provider_ops_t	pseudond_gio_provider_ops = {
	pseudond_gio_channel_event,
	pseudond_gio_bind_req,
	pseudond_gio_unbind_req,
	pseudond_gio_xfer_req,
	pseudond_gio_event_res
};
#endif	/* 0 || LATER */

/*
 * Network Meta/Pseudo Driver Control Operation Entrypoints
 */
static udi_nd_ctrl_ops_t  pseudond_ctrl_ops = {
	pseudo_nd_channel_event,	/* channel_event_ind_op	*/
	pseudo_nd_bind_req,		/* nd_bind_req_op	*/
	pseudo_nd_unbind_req,		/* nd_unbind_req_op	*/
	pseudo_nd_enable_req,		/* nd_enable_req_op	*/
	pseudo_nd_disable_req,		/* nd_disable_req_op	*/
	pseudo_nd_ctrl_req,		/* nd_ctrl_req_op	*/
	pseudo_nd_info_req		/* nd_info_req_op	*/
};

/*
 * Network Meta/Pseudo Driver Transmit Entrypoints
 */
static udi_nd_tx_ops_t	pseudond_tx_ops = {
	pseudo_nd_channel_event,	/* channel_event_ind_op	*/
	pseudo_nd_tx_req,		/* nd_tx_req_op		*/
	pseudo_nd_exp_tx_req		/* nd_exp_tx_req_op	*/
};

/*
 * Network Meta/Pseudo Driver Receive Entrypoints
 */
static udi_nd_rx_ops_t	pseudond_rx_ops = {
	pseudo_nd_channel_event,	/* channel_event_ind_op	*/
	pseudo_nd_rx_rdy		/* nd_rx_rdy_op		*/
};

/*
 * Bus Bridge Meta entrypoints
 */
static udi_bus_device_ops_t pseudond_bus_device_ops = {
	pseudond_bus_channel_event,
	pseudond_bus_bind_ack,
	pseudond_bus_unbind_ack,
	udi_intr_attach_ack_unused,
	udi_intr_detach_ack_unused
};

/*
 * a default op_flags array
 */
static const udi_ubit8_t pseudond_default_op_flags[]  = {
	0, 0, 0, 0, 0, 0 
};

/*
 * -----------------------------------------------------------------------------
 * Management operations init section:
 * -----------------------------------------------------------------------------
 */
static udi_primary_init_t	pseudond_primary_init = {
	&pseudond_mgmt_ops,
	pseudond_default_op_flags,
	0,			/* mgmt scratch */
	0,			/* enumerate no children */
	sizeof( Pseudo_Card_t ),
	0,			/* child data size */
	0,			/* buf path */
};

/*
 * -----------------------------------------------------------------------------
 * Meta operations init section:
 * -----------------------------------------------------------------------------
 */
static udi_ops_init_t	pseudond_ops_init_list[] = {
#if 0
	{
		PSEUDO_GIO_INTERFACE,
		ND_GIO_META,			/* meta index */
		UDI_GIO_PROVIDER_OPS_NUM,	/* meta ops num */
		0,				/* chan_context size */
		(udi_ops_vector_t *)&pseudond_gio_provider_ops,	/* ops vector */
		pseudond_default_op_flags
	},
#endif
	{
		PSEUDO_NET_CTRL_IDX,
		ND_NSR_META,			/* meta index */
		UDI_ND_CTRL_OPS_NUM,
		0,				/* chan_context size */
		(udi_ops_vector_t *)&pseudond_ctrl_ops,		/* ops vector */
		pseudond_default_op_flags
	},
	{
		PSEUDO_NET_TX_IDX,
		ND_NSR_META,			/* meta index */
		UDI_ND_TX_OPS_NUM,
		0,				/* chan_context size */
		(udi_ops_vector_t *)&pseudond_tx_ops,		/* ops vector */
		pseudond_default_op_flags
	},
	{
		PSEUDO_NET_RX_IDX,
		ND_NSR_META,			/* meta index */
		UDI_ND_RX_OPS_NUM,
		0,				/* chan_context size */
		(udi_ops_vector_t *)&pseudond_rx_ops,		/* ops vector */
		pseudond_default_op_flags
	},
	{
		PSEUDO_NET_PARENT_IDX,
		ND_BRIDGE_META,			/* meta index */
		UDI_BUS_DEVICE_OPS_NUM,
		0,				/* no channel context */
		(udi_ops_vector_t *)&pseudond_bus_device_ops,
		pseudond_default_op_flags
	},
	{
		0	/* Terminator */
	}
};

/*
 * -----------------------------------------------------------------------------
 * Meta control block init section:
 * -----------------------------------------------------------------------------
 */
static udi_layout_t	xfer_layout[] = { UDI_DL_UBIT32_T, UDI_DL_UBIT32_T,
					  UDI_DL_UBIT16_T, UDI_DL_END };

static udi_cb_init_t	pseudond_cb_init_list[] = {
#if 0
	{
		PSEUDO_GIO_BIND_CB_IDX,		/* GIO bind idx */
		ND_GIO_META,			/* meta index	*/
		UDI_GIO_BIND_CB_NUM,		/* meta cb_num	*/
		0,				/* scratch req	*/
		0,				/* inline size  */
		NULL				/* inline layout*/
	},
	{
		PSEUDO_GIO_XFER_CB_IDX,		/* GIO xfer idx	*/
		ND_GIO_META,			/* meta index	*/
		UDI_GIO_XFER_CB_NUM,		/* meta cb_num	*/
		0,				/* scratch req	*/
		sizeof( udi_gio_rw_params_t ),	/* inline size	*/
		xfer_layout			/* inline layout*/
	},
	{
		PSEUDO_GIO_EVENT_CB_IDX,	/* GIO event idx*/
		ND_GIO_META,			/* meta index	*/
		UDI_GIO_EVENT_CB_NUM,		/* meta cb_num	*/
		0,				/* scratch req	*/
		0,				/* inline size	*/
		NULL				/* inline layout*/
	},
#endif
	{
		PSEUDO_NET_CTRL_CB_IDX,		/* NSR ctrl idx	*/
		ND_NSR_META,			/* meta index	*/
		UDI_NIC_CTRL_CB_NUM,		/* meta cb_num	*/
		__PSEUDOND_CTRL_SCRATCH_SZ,	/* scratch req	*/
		0,				/* inline size	*/
		NULL				/* inline layout*/
	},
	{
		PSEUDO_NET_TX_CB_IDX,		/* NSR Tx idx	*/
		ND_NSR_META,			/* meta index	*/
		UDI_NIC_TX_CB_NUM,		/* meta cb_num	*/
		0,				/* scratch req	*/
		0,				/* inline size	*/
		NULL				/* inline layout*/
	},
	{
		PSEUDO_NET_RX_CB_IDX,		/* NSR Rx idx	*/
		ND_NSR_META,			/* meta index	*/
		UDI_NIC_RX_CB_NUM,		/* meta cb_num	*/
		0,				/* scratch req	*/
		0,				/* inline size	*/
		NULL				/* inline layout*/
	},
	{
		PSEUDO_NET_STD_CB_IDX,		/* NSR std idx	*/
		ND_NSR_META,			/* meta index	*/
		UDI_NIC_STD_CB_NUM,		/* meta cb_num	*/
		0,				/* scratch req	*/
		0,				/* inline size	*/
		NULL				/* inline layout*/
	},
	{
		PSEUDO_NET_STATUS_CB_IDX,	/* NSR status idx*/
		ND_NSR_META,			/* meta index	*/
		UDI_NIC_STATUS_CB_NUM,		/* meta cb_num	*/
		0,				/* scratch req	*/
		0,				/* inline size	*/
		NULL				/* inline layout*/
	},
	{
		PSEUDO_NET_INFO_CB_IDX,		/* NSR info idx */
		ND_NSR_META,			/* meta index	*/
		UDI_NIC_INFO_CB_NUM,		/* meta cb_num	*/
		0,				/* scratch size	*/
		0,				/* inline size	*/
		NULL				/* inline layout*/
	},
	{
		PSEUDO_NET_BIND_CB_IDX,		/* NSR bind idx	*/
		ND_NSR_META,			/* meta index	*/
		UDI_NIC_BIND_CB_NUM,		/* meta cb_num	*/
		0,				/* scratch size	*/
		0,				/* inline size	*/
		NULL				/* inline layout*/
	},
	{
		PSEUDO_BRIDGE_BIND_CB_IDX,	/* Bridge bind idx */
		ND_BRIDGE_META,			/* meta index	*/
		UDI_BUS_BIND_CB_NUM,		/* meta cb_num	*/
		0,				/* scratch	*/
		0,				/* inline size	*/
		NULL				/* inline layout*/
	},
	{
		0	/* Terminator */
	}
};

/*
 * -----------------------------------------------------------------------------
 * Global init data:
 * -----------------------------------------------------------------------------
 */

udi_init_t udi_init_info = {
	&pseudond_primary_init,
	NULL,				/* Secondary init list */
	pseudond_ops_init_list,
	pseudond_cb_init_list,
	NULL,				/* GCB init list	*/
	NULL,				/* CB select list	*/
};

/*
 * --------------------------------------------------------------------------
 * Management Metalanguage entrypoints
 */

/*
 * First operation on a new channel. We need to allocate some space for our
 * shared region-local data and then respond successfully
 */
STATIC void
pseudo_nd_usage_ind(
	udi_usage_cb_t	*cb,
	udi_ubit8_t	resource_level)
{
	Pseudo_Card_t	*Pc = UDI_GCB(cb)->context;

	DBG_MGMT(("pseudo_nd_usage_ind(cb = %p, level = %d)\n", cb, 
		  resource_level));
	
	DBG_MGMT(("\t...trace_mask = %p, meta_idx = %d\n", cb->trace_mask,
		  cb->meta_idx));
	
	DBG_MGMT(("\t...context = %p\n", Pc));

	if ( Pc->Card_Pcp != (void *)Pc ) {
		/*
	 	 * Initialise the queue heads
	 	 */
		UDI_QUEUE_INIT(&Pc->cbsegq.Q);	/* (Pseudo_cbseg_t *) 	*/
		UDI_QUEUE_INIT(&Pc->cbq.Q );	/* (Pseudo_cb_t *) 	*/

		UDI_QUEUE_INIT(&Pc->Card_rxatq.Q );
		UDI_QUEUE_INIT(&Pc->Card_rxdoq.Q );
		UDI_QUEUE_INIT(&Pc->Card_rxcbq.Q );
		UDI_QUEUE_INIT(&Pc->Card_rxdoneq.Q );

		UDI_QUEUE_INIT(&Pc->Card_txatq.Q );
		UDI_QUEUE_INIT(&Pc->Card_txcbq.Q );
		UDI_QUEUE_INIT(&Pc->Card_txrdyq.Q );
		UDI_QUEUE_INIT(&Pc->Card_txdoq.Q );

		UDI_QUEUE_INIT(&Pc->Card_defer_op.Q );

		Pc->Card_media_type = __PSEUDOND_MEDIA_TYPE; 
		Pc->Card_Pcp = (void *)Pc;

		Pc->Card_num = nPseudoNDs++;
		Pc->Meta_state = ND_UNBOUND;
	}

	udi_usage_res( cb );
}

STATIC void
pseudo_nd_enumerate_req(
	udi_enumerate_cb_t *cb,
	udi_ubit8_t enum_level)
{
	 udi_ubit8_t result;

	/*
	 * We don't enumerate any attributes; the meta alone should be
	 * enough to bind to the mapper.
	 */
	cb->attr_valid_length = 0;

	switch (enum_level) {
	case UDI_ENUMERATE_START:
	case UDI_ENUMERATE_START_RESCAN:
		result = UDI_ENUMERATE_OK;
		break;
 
	case UDI_ENUMERATE_NEXT:
		result = UDI_ENUMERATE_DONE;
		break;
 
	case UDI_ENUMERATE_RELEASE:
		/* release resources for the specified child */
		result = UDI_ENUMERATE_RELEASED;
		break;
 
	case UDI_ENUMERATE_NEW:
	case UDI_ENUMERATE_DIRECTED:
	default:
		result = UDI_ENUMERATE_FAILED;
	}
	udi_enumerate_ack(cb, result, PSEUDO_NET_CTRL_IDX);
}


STATIC void 
pseudo_nd_devmgmt_req(
	udi_mgmt_cb_t	*cb,
	udi_ubit8_t	mgmt_op,
	udi_ubit8_t	parent_id)
{
	Pseudo_Card_t	*Pc = UDI_GCB(cb)->context;
	udi_ubit8_t	flags;

	DBG_MGMT(("pseudo_nd_devmgmt_req(cb = %p, mgmt_op = %u, context = %u\n",
		  cb, mgmt_op, parent_id));

	switch( mgmt_op ) {
	case UDI_DMGMT_PREPARE_TO_SUSPEND:
	case UDI_DMGMT_SUSPEND:
		flags = UDI_DMGMT_NONTRANSPARENT;
		break;
	case UDI_DMGMT_UNBIND:
		/* Keep a link back to this CB for use in the ack. */
		Pc->Card_BUS_cb->gcb.initiator_context = cb;

		/* Unbind from the bus */
		udi_bus_unbind_req( Pc->Card_BUS_cb );
		return;
	default:
		flags = 0;
		break;
	}

	udi_devmgmt_ack( cb, flags, UDI_OK );
}

STATIC void
pseudo_nd_final_cleanup_req(
	udi_mgmt_cb_t	*cb)
{
	DBG_MGMT(("pseudo_nd_final_cleanup_req(cb = %p)\n", cb));

	udi_final_cleanup_ack(cb);
}

#if 0 || LATER
/*
 * --------------------------------------------------------------------------
 * GIO Metalanguage entrypoints
 */
STATIC void
pseudond_gio_channel_event(
	udi_channel_event_cb_t	*channel_event_cb)
{
	DBG_GIO(("pseudond_gio_channel_event( cb = %p )\n", channel_event_cb));

#if 0
	udi_cb_free( UDI_GCB(channel_event_cb) );
#else
	udi_channel_event_complete( channel_event_cb, UDI_OK );
#endif
}
#endif

#if 0 || OLDWAY
/*
 * Bind to GIO. Multiple metas are bound simultaneously, it is only when we
 * try to perform a 'stack' bind (e.g. TCP or GIO request) that we may have
 * to fail the operation.
 */
STATIC void
pseudond_gio_bind_req(
	udi_gio_bind_cb_t	*gio_bind_cb)
{
	Pseudo_Card_t	*Pc = UDI_GCB(gio_bind_cb)->context;


	DBG_GIO(("pseudond_gio_bind_req( cb = %p, context = %p )\n", 
		gio_bind_cb, Pc));


	udi_gio_bind_ack( gio_bind_cb,
			  0,	/* LS 32bits device size */
			  0,	/* MS 32bits device size */
			  UDI_OK );
}

/*
 * Unbind from GIO. 
 */
STATIC void
pseudond_gio_unbind_req(
	udi_gio_bind_cb_t	*gio_bind_cb)
{
	Pseudo_Card_t	*Pc = UDI_GCB(gio_bind_cb)->context;

	DBG_GIO(("pseudond_gio_unbind_req( cb = %p )\n", gio_bind_cb));

	udi_gio_unbind_ack( gio_bind_cb );
	Pc->Card_GIO = NULL;
	Pc->Card_GIO_cb = NULL;
}

/*
 * pseudond_gio_xfer_req:
 * ---------------------
 * Perform GIO specific operation (ioctl-style interface)
 *
 * Supported ops are:
 *	PSEUDOND_GIO_MEDIA:	Set max_pdu_size, min_pdu_size and media_type
 *				link_info entries to <buf> contents. This allows
 *				different media types and PDU sizes to be used
 *				for the same pseudo instance
 */
STATIC void
pseudond_gio_xfer_req(
	udi_gio_xfer_cb_t	*gio_xfer_cb)
{
	Pseudo_Card_t	*Pc = UDI_GCB(gio_xfer_cb)->context;
	pseudond_media_t	media;


	DBG_GIO(("pseudond_gio_xfer_req( cb = %p )\n", gio_xfer_cb));

	switch ( gio_xfer_cb->op ) {
	case (PSEUDOND_GIO_MEDIA|UDI_GIO_DIR_WRITE):
		if ( gio_xfer_cb->data_len == sizeof( pseudond_media_t ) ) {
			(void)udi_buf_read( gio_xfer_cb->data_buf,
				      0,
				      gio_xfer_cb->data_len,
				      &media );

			Pc->Card_max_pdu = media.max_pdu_size;
			Pc->Card_min_pdu = media.min_pdu_size;
			Pc->Card_media_type = media.media_type;

			udi_gio_xfer_ack( gio_xfer_cb );
		} else {
			udi_gio_xfer_nak( gio_xfer_cb, 0 );
		}
		break;
	default:
		udi_gio_xfer_nak( gio_xfer_cb, 0 );
	}
}
#endif	/* 0 || OLDWAY */

#if 0 || OLDWAY
STATIC void
pseudond_gio_abort_req(
	udi_gio_abort_cb_t	*gio_abort_cb)
{
	udi_gio_abort_ack( gio_abort_cb );
}

STATIC void
pseudond_gio_event_res(
	udi_gio_event_cb_t	*gio_event_cb)
{
}
#endif	/* 0 || OLDWAY */


/*
 * --------------------------------------------------------------------------
 * Network Meta Control entrypoints
 */

STATIC void 
pseudo_nd_channel_event(
	udi_channel_event_cb_t *cb)
{
	DBG_ND(("pseudo_nd_channel_event(%p), event = %d\n", cb, cb->event));

	udi_channel_event_complete(cb, UDI_OK);
}

/*
 * Bind to NSR. Multiple metas can be simultaneously bound, but only one
 * can have physical access to the device. This is controlled by the meta
 * specific attachment ops.
 * NOTE: We have to perform some of the same actions that udi_prep_for_child
 * used to do. In particular, we need a control CB that won't disappear until
 * the pseudo_nd_unbind_req has been processed. The incoming udi_nic_bind_cb_t
 * is only valid for the duration of the _bind_ op, and will be freed by the
 * NSR once we respond.
 */
STATIC void 
pseudo_nd_bind_req(
	udi_nic_bind_cb_t *cb,
	udi_index_t	  tx_chan_index,
	udi_index_t	  rx_chan_index )		/* From MA */
{
	Pseudo_Card_t	*Pc = UDI_GCB(cb)->context;
#if 0
	udi_nic_bind_ack_cb_t	*ack_cb;
#endif

	DBG_ND(("pseudo_nd_bind_req( cb = %p )\n", cb));
	DBG_ND(("\t...tx_chan = %d, rx_chan = %d\n", tx_chan_index,
		rx_chan_index));
	DBG_ND(("\t...context = %p\n", Pc));

	/* Validate state */
	if ( Pc->Meta_state != ND_UNBOUND ) {
		udi_nsr_bind_ack( cb, UDI_STAT_INVALID_STATE );
		return;
	} else {
		Pc->Meta_state = ND_NSR_BINDING;
	}

	Pc->Card_Tx_idx = tx_chan_index;
	Pc->Card_Rx_idx = rx_chan_index;

	udi_cb_alloc( pseudo_nd_bind_step1, UDI_GCB(cb), 
		PSEUDO_NET_BIND_CB_IDX, UDI_GCB(cb)->channel );
}

/*
 * pseudond_attr_get:
 * ------------------
 * Obtain all attributes for this card. On completion we continue with the
 * delayed udi_nd_enable_req.
 */
STATIC void
pseudond_attr_get(
	udi_cb_t			*gcb,
	udi_instance_attr_type_t	attr_type,
	udi_size_t			actual_length )
{
	Pseudo_Card_t	*Pc = gcb->context;

	switch( Pc->Card_attr ) {
	case ND_ATTR_LINK_SPEED:
		/* Obtained link speed, get next attribute */
		DBG_ND(("pseudond_attr_get: LINK_SPEED = %d\n",
			Pc->Card_link_speed));
		Pc->Card_attr = ND_ATTR_CPU_SAVER;
		udi_instance_attr_get( pseudond_attr_get, gcb,
					"%cpu_saver",
					0,
					&Pc->Card_cpu_saver,
					sizeof( Pc->Card_cpu_saver )
				     );
		break;
	case ND_ATTR_CPU_SAVER:
		/* Obtained cpu_saver, get next attribute */
		DBG_ND(("pseudond_attr_get: CPU_SAVER = %d\n",
			Pc->Card_cpu_saver));
		Pc->Card_attr = ND_ATTR_SPECIOUS_PARAM;
		udi_instance_attr_get( pseudond_attr_get, gcb,
					"%utterly_specious_parameter",
					0,
					&Pc->Card_specious,
					sizeof( Pc->Card_specious )
				     );
		break;
	case ND_ATTR_SPECIOUS_PARAM:
		/* Obtained utterly_specious_parameter, get next attribute */
		DBG_ND(("pseudond_attr_get: specious_parameter = %d\n",
			Pc->Card_specious));
		
		/*
	 	 * Complete the udi_nd_enable_req...
	 	 */
		Pseudo_ND_Alloc_Resources(Pc);
		break;
	}
	return;
}


STATIC void
pseudo_nd_bind_step1(
	udi_cb_t	*gcb,
	udi_cb_t	*new_cb)
{
	Pseudo_Card_t	*Pc = gcb->context;

	Pc->Card_NSR = new_cb->channel;		/* For channel_spawn */
	Pc->Card_NSR_cb = new_cb;

	Pc->Card_scratch_cb = gcb;	/* Original udi_nic_bind_cb */

	udi_channel_spawn( pseudo_nd_tx_anchored,
			   new_cb,
			   Pc->Card_NSR,
			   Pc->Card_Tx_idx,
			   PSEUDO_NET_TX_IDX,		/* Bound */
			   Pc );
}

STATIC void
pseudo_nd_bind_cbfn(
	udi_cb_t	*gcb,			/* From MA */
	udi_cb_t	*new_cb)		/* Tx CB */
{
	Pseudo_Card_t	*Pc = gcb->context;

	DBG_ND(("pseudo_nd_bind_cbfn( gcb = %p, new_cb = %p )\n",
		gcb, new_cb));
	
	/* Bind to NSR */

	Pc->Card_Tx_cb = new_cb;

	udi_channel_spawn( pseudo_nd_rx_anchored,
			   gcb,
			   Pc->Card_NSR,
			   Pc->Card_Rx_idx,
			   PSEUDO_NET_RX_IDX, 	/* Bound */
			   Pc );
}

/*
 * Callback for Tx channel udi_channel_anchor()
 */
STATIC void
pseudo_nd_tx_anchored(
	udi_cb_t	*cb,			/* From Driver */
	udi_channel_t	anchored_channel )
{
	Pseudo_Card_t	*Pc = cb->context;

	DBG_MGMT(("pseudo_nd_tx_anchored( cb = %p, channel = %p)\n",
		  cb, anchored_channel));
	
	Pc->Card_Tx_chan = anchored_channel;

	udi_cb_alloc( pseudo_nd_bind_cbfn, cb, PSEUDO_NET_TX_CB_IDX,
		      anchored_channel );
}

STATIC void
pseudo_nd_rxalloc_cbfn(
	udi_cb_t	*gcb,			/* From Driver */
	udi_cb_t	*new_cb )		/* Rx CB */
{
	Pseudo_Card_t	*Pc = gcb->context;
	udi_nic_bind_cb_t	*cb = 
			UDI_MCB(Pc->Card_scratch_cb, udi_nic_bind_cb_t );
	udi_ubit8_t	nic_addr[6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xBA, 0xBE };

	DBG_MGMT(("pseudo_nd_rxalloc_cbfn( gcb = %p, new_cb = %p )\n",
		  gcb, new_cb));

	Pc->Card_Rx_cb = new_cb;


	/* The state field gets updated by meta-specific enablers, not us */

	cb->media_type = Pc->Card_media_type;
	cb->min_pdu_size = Pc->Card_min_pdu;
	cb->max_pdu_size = Pc->Card_max_pdu;
	cb->rx_hw_threshold = NUM_RX_PDL; /* # of receive requests wanted */
	cb->mac_addr_len = 0;		  /* Default 	*/
	nic_addr[5] = Pc->Card_num;	  /* Ensure unique MAC address */
	udi_memcpy( &cb->mac_addr, nic_addr, sizeof(nic_addr) );

	/*
	 * To do: implement a simple multicast address hashing algorithm
	 * and a small perfect-matching table. Give appropriate limits.
	 * This would only be for emulating actual hardware. Real UDI ND
	 * drivers should never implement their own multicast address
	 * table and should let the NSR handle this missing hardware
	 * functionality since the OS will likely have an optimized
	 * filtering function.
	 * For now, say this hardware doesn't have multicast
	 * address filtering abilities.
	 */
	cb->max_total_multicast = 0;
	cb->max_perfect_multicast = 0;

	/* Save factory MAC address for future use */
	udi_memcpy( &Pc->Card_fact_macaddr, nic_addr, sizeof(nic_addr) );
	
	/* Save to current MAC address for future use */
	udi_memcpy( &Pc->Card_macaddr, nic_addr, sizeof(nic_addr) );

	/* Allocate a PCB for us to use when allocating per-card info */

	/*
	 * Create an empty Pseudo_cb_t to allow us to start deferred
	 * processing in pseudo_nd_getnext_rsrc
	 */
	udi_mem_alloc( pseudo_nd_bind_complete, UDI_GCB(cb), 
		       sizeof( Pseudo_cb_t ), 0 );

}

/*
 * Callback for Rx channel udi_channel_anchor()
 */
STATIC void
pseudo_nd_rx_anchored(
	udi_cb_t	*cb,			/* From Driver */
	udi_channel_t	anchored_channel )
{
	Pseudo_Card_t	*Pc = cb->context;

	DBG_MGMT(("pseudo_nd_rx_anchored( cb = %p, channel = %p)\n",
		  cb, anchored_channel));
	
	Pc->Card_Rx_chan = anchored_channel;

	udi_cb_alloc( pseudo_nd_rxalloc_cbfn, cb, PSEUDO_NET_BIND_CB_IDX,
		      Pc->Card_scratch_cb->channel );
}

STATIC void
pseudo_nd_bind_complete(
	udi_cb_t	*gcb,
	void		*new_mem )
{
	Pseudo_Card_t	*Pc = gcb->context;
	udi_nic_bind_cb_t	*cb = 
			UDI_MCB(Pc->Card_scratch_cb, udi_nic_bind_cb_t );

	DBG_MGMT(("pseudo_nd_bind_complete entry state %s\n",
		  state_names[Pc->Meta_state]));


	PSEUDOND_ASSERT( Pc->cbq.numelem == 0 );

	UDI_ENQUEUE_TAIL( &Pc->cbq.Q, (udi_queue_t *)new_mem );
	Pc->cbq.numelem++;

	Pc->Meta_state = ND_NSR_BOUND;

	udi_nsr_bind_ack( cb, UDI_OK );
}

/*
 * Unbind from NSR. If the ND is currently bound to the NSR close all control
 * transmit and receive channels and notify the NSR that this succeeded.
 * If the NIC hasn't completed the tear-down that was initiated by an earlier
 * udi_nd_disable_req() we defer processing until the tear-down has completed.
 * This allows the NSR to 'do the right thing' state-wise.
 */
STATIC void 
pseudo_nd_unbind_req(
	udi_nic_cb_t *cb)
{
	Pseudo_Card_t	*Pc = UDI_GCB(cb)->context;

	if ( Pc->Meta_state == ND_NSR_DISABLING ) {
		/*
		 * TODO: Defer processing until previous udi_nd_disable_req has
		 * completed
		 */
		udi_mem_alloc( pseudo_nd_unbind_step1, UDI_GCB(cb),
			      sizeof( pseudo_defer_el_t ), 0 );
		return;
	} else {

		/*
	 	 * Validate the state we're in 
	 	 */
		if ( Pc->Meta_state != ND_NSR_BOUND && 
	     	     Pc->Meta_state != ND_NSR_ACTIVE &&
	     	     Pc->Meta_state != ND_NSR_ENABLED ) {
			udi_nsr_unbind_ack( cb, UDI_STAT_INVALID_STATE );
			return;
		}

		Pc->Meta_state = ND_NSR_UNBINDING;

		/* TODO: Check this ordering */
		udi_channel_close( Pc->Card_Tx_chan );
		udi_channel_close( Pc->Card_Rx_chan );

		Pc->Card_Tx_chan = UDI_NULL_CHANNEL;
		Pc->Card_Rx_chan = UDI_NULL_CHANNEL;

		/* TODO: Release allocated memory. When all memory is freed we 
	 	*	 can complete the udi_nsr_unbind_ack handshake 
	 	*/

		Pc->Card_scratch_cb = UDI_GCB(cb);
	
		Pseudo_ND_Free_Resources( Pc );

#if 0 || OLDWAY
		udi_cb_free( Pc->Card_Tx_cb );
		udi_cb_free( Pc->Card_Rx_cb );

		udi_nsr_unbind_ack( cb, UDI_OK );

		Pc->Meta_state = ND_NSR_UNBOUND;
#endif
	}

}

/*
 * pseudo_nd_unbind_step1:
 * ----------------------
 * Enqueue the pending udi_nd_unbind_req until the Meta_state changes back
 * from ND_NSR_DISABLING.
 */
STATIC void
pseudo_nd_unbind_step1(
	udi_cb_t	*gcb,
	void		*new_mem )
{
	Pseudo_Card_t		*Pc = gcb->context;
	pseudo_defer_el_t	*elp = (pseudo_defer_el_t *)new_mem;

	elp->op = _udi_nd_unbind_req;
	elp->cb = UDI_MCB( gcb, udi_nic_cb_t );

	UDI_ENQUEUE_TAIL( &Pc->Card_defer_op.Q, (udi_queue_t *)elp );
	return;
}

/*
 * Enable the network connection:
 *	Respond to NSR that connection was successfully enabled if we are
 *	not already in an enabled state.
 *	Flag link-up by calling udi_nsr_status_ind,
 *	Pass transmit buffers to NSR by udi_nsr_tx_rdy()
 */
STATIC void 
pseudo_nd_enable_req(
	udi_nic_cb_t *cb)
{
	Pseudo_Card_t	*Pc = UDI_GCB(cb)->context;

	if ( Pc->Meta_state == ND_NSR_DISABLING ) {
		/* TODO: Defer the pseudo_nd_enable_req */
		udi_mem_alloc( pseudo_nd_enable_step1, UDI_GCB(cb),
			       sizeof( pseudo_defer_el_t ), 0 );
		return;
	} else {

		if ( Pc->Meta_state != ND_NSR_BOUND ) {
			if ( Pc->Meta_state == ND_NSR_ENABLED ) {
			  DBG_MGMT(("pseudo_nd_enable_req: Already ENABLED\n"));
				return;
			}
			DBG_MGMT(("pseudo_nd_enable_req: State = %s\n", 
				state_names[Pc->Meta_state]));

			udi_nsr_enable_ack( cb, UDI_STAT_INVALID_STATE );
		} else {
			Pc->Meta_state = ND_NSR_ENABLED;

			/* mark link-up and allocate transmit buffers */
			Pc->Card_link_cb = UDI_GCB( cb );

			BSET(Pc->Card_state, CardS_Enabling);

			/* Obtain per-instance attribute settings */
			Pc->Card_attr = ND_ATTR_LINK_SPEED;
			udi_instance_attr_get( pseudond_attr_get, UDI_GCB(cb),
			       	"%link_speed",			/*attr_name*/
			       	0,				/*child_ID*/
			       	&Pc->Card_link_speed,		/*attr_value*/
			       	sizeof( Pc->Card_link_speed )	/*attr_length*/
			     );
			return;
#if 0
			Pseudo_ND_Alloc_Resources( Pc );
#endif

		}
	}
}

/*
 * pseudo_nd_enable_step1:
 * -----------------------
 * Enqueue the pending udi_nd_enable_req until the card has complete its
 * ND_NSR_DISABLING processing.
 */
STATIC void
pseudo_nd_enable_step1(
	udi_cb_t	*gcb,
	void		*new_mem )
{
	Pseudo_Card_t		*Pc = gcb->context;
	pseudo_defer_el_t	*elp = (pseudo_defer_el_t *)new_mem;

	elp->op = _udi_nd_enable_req;
	elp->cb = UDI_MCB( gcb, udi_nic_cb_t );

	UDI_ENQUEUE_TAIL( &Pc->Card_defer_op.Q, (udi_queue_t *)elp );
	return;
}

/*
 * Disable Link:
 *	Issue udi_nsr_status_ind to show link state change
 */
STATIC void 
pseudo_nd_disable_req(
	udi_nic_cb_t *cb)
{
	Pseudo_Card_t	*Pc = UDI_GCB(cb)->context;

	PSEUDOND_ASSERT((Pc->Meta_state == ND_NSR_ENABLED 
			|| Pc->Meta_state == ND_NSR_ACTIVE));

	DBG_MGMT(("pseudo_nd_disable_req state %s\n", 
		  state_names[Pc->Meta_state]));

	/*
	 * Any control requests that arrive in the DISABLING state will be
	 * queued for later despatch.
	 */
	Pc->Meta_state = ND_NSR_DISABLING;

	/* TODO: Issue udi_nsr_status_ind */

	BCLR(Pc->Card_state, (CardS_LinkUp|CardS_Enabled));

	Pc->Card_link_status = UDI_NIC_LINK_DOWN;

	BSET(Pc->Card_state, CardS_Disabling);

#if 0 || OLDWAY
	Pc->Meta_state = ND_NSR_BOUND;
#endif

#if 0 || OLD_RECYCLE
	Pseudo_ND_Free_Resources( Pc );
#else
	/* Inform the NSR of the link status change */
	udi_cb_alloc(pseudo_nd_status,Pc->Card_NSR_cb,PSEUDO_NET_STATUS_CB_IDX,
		     Pc->Card_NSR);
#endif	/* OLD_RECYCLE */
	udi_cb_free(UDI_GCB(cb));
}

STATIC void 
pseudo_nd_ctrl_req(
	udi_nic_ctrl_cb_t *cb) 
{
	Pseudo_Card_t	*Pc = UDI_GCB(cb)->context;

	PSEUDOND_ASSERT( Pc->Meta_state == ND_NSR_BOUND || 
			 Pc->Meta_state == ND_NSR_ACTIVE ||
			 Pc->Meta_state == ND_NSR_ENABLED );

	/* TODO: Implement functionality */

	switch ( cb->command ) {
	case UDI_NIC_ADD_MULTI:
	case UDI_NIC_DEL_MULTI:
	case UDI_NIC_ALLMULTI_ON:
	case UDI_NIC_ALLMULTI_OFF:
	case UDI_NIC_PROMISC_ON:
	case UDI_NIC_PROMISC_OFF:
		udi_nsr_ctrl_ack(cb, UDI_OK);
		break;
	case UDI_NIC_HW_RESET:
		/*
		 * Cause a reset indication to be sent to the NSR.
		 */
		Pc->Card_link_status = UDI_NIC_LINK_RESET;
		udi_cb_alloc(pseudo_nd_status, Pc->Card_NSR_cb, 
				PSEUDO_NET_STATUS_CB_IDX, Pc->Card_NSR);
		udi_nsr_ctrl_ack(cb, UDI_OK);
		break;
	case UDI_NIC_GET_FACT_MAC:
		/*
		 * Return Card's factory MAC address to supplied buffer 
		 */
		cb->indicator = 6;
		udi_buf_write(pseudo_nd_ctrl_req1, UDI_GCB(cb), 
			      &Pc->Card_fact_macaddr, 6, cb->data_buf, 0, 6,
			      UDI_NULL_BUF_PATH);
		break;
	case UDI_NIC_GET_CURR_MAC:
		/*
		 * Return Card's currently set MAC address
		 */
		cb->indicator = 6;
		udi_buf_write(pseudo_nd_ctrl_req1, UDI_GCB(cb),
			      &Pc->Card_macaddr, 6, cb->data_buf, 0, 6,
			      UDI_NULL_BUF_PATH);
		break;
	case UDI_NIC_SET_CURR_MAC:
		/*
		 * Set Card's MAC address to that supplied in cb->data_buf
		 */
		udi_buf_read(cb->data_buf, 0, cb->indicator, &Pc->Card_macaddr);
		udi_nsr_ctrl_ack(cb, UDI_OK);
		break;
	default:
		udi_nsr_ctrl_ack( cb, UDI_STAT_NOT_UNDERSTOOD );
	}
}

/*
 * pseudo_nd_ctrl_req1:
 * --------------------
 * Callback for buffer modification in UDI_NIC_GET_FACT_MAC, 
 * UDI_NIC_GET_CURR_MAC command handling. The buffer originally in cb->data_buf
 * will have been updated and (potentially) reallocated. We need to free the
 * original buffer and replace it with the new one, and then complete the
 * udi_nd_ctrl_req() processing.
 */
STATIC void
pseudo_nd_ctrl_req1(
	udi_cb_t 	*gcb,
	udi_buf_t	*new_buf )
{
	udi_nic_ctrl_cb_t	*cb = UDI_MCB( gcb, udi_nic_ctrl_cb_t );

	if ( cb->data_buf != new_buf ) {
		udi_buf_free( cb->data_buf );
	}
	cb->data_buf = new_buf;

	udi_nsr_ctrl_ack( cb, UDI_OK );
}

/*
 * Return Information block to NSR
 */
STATIC void
pseudo_nd_info_req(
	udi_nic_info_cb_t	*cb,
	udi_boolean_t		reset_statistics)
{
	Pseudo_Card_t	*Pc = UDI_GCB(cb)->context;

	/* Check that we receive this in a sane state */
	PSEUDOND_ASSERT( Pc->Meta_state == ND_NSR_BOUND || 
			 Pc->Meta_state == ND_NSR_ACTIVE ||
			 Pc->Meta_state == ND_NSR_ENABLED );

	cb->interface_is_active = BTST(Pc->Card_state, CardS_LinkUp) ? TRUE :
				  FALSE;
	cb->link_is_active = TRUE;
	cb->is_full_duplex = TRUE;
	if ( __PSEUDOND_MEDIA_SPEED > 1000000 ) {
		cb->link_mbps = __PSEUDOND_MEDIA_SPEED / 1000000;
		cb->link_bps = 0;		/* Use link_mbps */
	} else {
		cb->link_mbps = 0;		/* Use link_bps */
		cb->link_bps = __PSEUDOND_MEDIA_SPEED;
	}
	cb->tx_packets = Pc->Cardstat_opackets;
	cb->rx_packets = Pc->Cardstat_ipackets;
	cb->tx_errors = Pc->Cardstat_oerrors;
	cb->rx_errors = Pc->Cardstat_ierrors;
	cb->tx_discards = Pc->Cardstat_odiscards;
	cb->rx_discards = Pc->Cardstat_idiscards;
	cb->tx_underrun = Pc->Cardstat_ounderrun;
	cb->rx_overrun = Pc->Cardstat_ioverrun;
	cb->collisions = Pc->Cardstat_collisions;

	udi_nsr_info_ack( cb );
}

/*
 * --------------------------------------------------------------------------
 * Network Meta Data Transfer entrypoints
 */

STATIC void
pseudo_nd_tx_req(
	udi_nic_tx_cb_t	*cb)
{
	/*
	 * Allocate a token from _txatq to keep track of the incoming cb.
	 * We allocate one queue element for each chained cb to keep things
	 * in some sort of order.
	 * If the link is disabled when we are called, we attempt to free up
	 * the data. This will only succeed when the NSR has posted all of
	 * our initially allocated net_tx_cb blocks back to us via this
	 * routine
	 */
	Pseudo_Card_t	*Pc = UDI_GCB(cb)->context;
	Pseudo_cb_t	*pcb;
	udi_queue_t	*elem;

	PSEUDOND_ASSERT( Pc->Meta_state == ND_NSR_ENABLED || 
			 Pc->Meta_state == ND_NSR_ACTIVE ||
			 Pc->Meta_state == ND_NSR_DISABLING ||
			 Pc->Meta_state == ND_NSR_BOUND	/* Spec violation */ );

#if 0
	if ( !BTST(Pc->Card_state, CardS_LinkUp) ) {
#else
#if 0
	if ( Pc->Meta_state != ND_NSR_ACTIVE || BTST(Pc->Card_state, CardS_Disabling) ){
#else
	if ( Pc->Meta_state != ND_NSR_ACTIVE ){
#endif
#endif
		/* Move CB onto _txcbq for immediate disposal */
		for ( ; cb ; cb = cb->chain ) {
			PSEUDOND_ASSERT( !UDI_QUEUE_EMPTY(&Pc->Card_txatq.Q) );
			elem = UDI_DEQUEUE_HEAD( &Pc->Card_txatq.Q );
			Pc->Card_txatq.numelem--;
			pcb = (Pseudo_cb_t *)elem;
			pcb->cbt = UDI_GCB(cb);
			UDI_ENQUEUE_TAIL( &Pc->Card_txcbq.Q, elem );
			Pc->Card_txcbq.numelem++;
		}

#if 0 || OLD_RECYCLE
		Pseudo_ND_Free_Resources( Pc );
#endif	/* OLD_RECYCLE */

	} else {
		/*
		 * Put CB onto _txrdyq for transmission, buffer has been
		 * provided by the NSR we just have to free it before
		 * posting the CB back
		 */
		for ( ; cb ; cb = cb->chain ) {
			PSEUDOND_ASSERT( !UDI_QUEUE_EMPTY(&Pc->Card_txatq.Q) );
			elem = UDI_DEQUEUE_HEAD( &Pc->Card_txatq.Q );
			Pc->Card_txatq.numelem--;
			pcb = (Pseudo_cb_t *)elem;
			pcb->cbt = UDI_GCB(cb);
			UDI_ENQUEUE_TAIL( &Pc->Card_txrdyq.Q, elem );
			Pc->Card_txrdyq.numelem++;
			Pc->Cardstat_opackets++;
		}

		/*
	 	 * Start transmit processing...
	 	 */
		pseudo_nd_transmit( Pc );
	}
}

STATIC void
pseudo_nd_exp_tx_req(
	udi_nic_tx_cb_t	*cb)
{
	/* TODO: */
}

/*
 * pseudo_nd_rx_rdy:
 * ----------------
 * Called by NSR to provide ND with list of Rx CBs which can be used for
 * receiving data.
 * Save these CBs on the rxcbq.
 */
STATIC void
pseudo_nd_rx_rdy(
	udi_nic_rx_cb_t	*cb)
{
	udi_nic_rx_cb_t	*tcb;
	Pseudo_cb_t	*pcb;
	Pseudo_Card_t	*Pc = UDI_GCB(cb)->context;
	udi_ubit32_t	ncbs;

	DBG_ND(("pseudo_nd_rx_rdy: State %s\n", state_names[Pc->Meta_state]));

	PSEUDOND_ASSERT( Pc->Meta_state == ND_NSR_ENABLED || 
			 Pc->Meta_state == ND_NSR_ACTIVE );

	for ( ncbs = 0; cb; cb = tcb, ncbs++ ) {
		tcb = cb->chain;
		pcb = (Pseudo_cb_t *)UDI_DEQUEUE_HEAD( &Pc->Card_rxatq.Q );
		Pc->Card_rxatq.numelem--;
		cb->chain = NULL;
		pcb->cbt = UDI_GCB(cb);
		UDI_ENQUEUE_TAIL( &Pc->Card_rxcbq.Q, (udi_queue_t *)pcb);
		Pc->Card_rxcbq.numelem++;
	}

	DBG_ND(("pseudo_nd_rx_rdy: Got %d RX CBs from NSR\n", ncbs));

	/* TODO: Issue LINK UP udi_nsr_status_ind if we're in ENABLED state */

	/* Tell the NSR that the link is physically up */

	if ( Pc->Meta_state == ND_NSR_ENABLED ) {
		Pc->Card_link_status = UDI_NIC_LINK_UP;
		udi_cb_alloc( pseudo_nd_status, Pc->Card_NSR_cb,
			      PSEUDO_NET_STATUS_CB_IDX, Pc->Card_NSR);
	}
}

/*
 * ---------------------------------------------------------------------------
 * Low-level transmit/receive routines
 */
STATIC void
pseudo_nd_transmit(
	Pseudo_Card_t	*Pc )
{
	udi_queue_t	*head, *elem, *tmp;
	Pseudo_cb_t	*pcb;
	udi_nic_tx_cb_t	*txcb;
	udi_nic_rx_cb_t	*rxcb;
	udi_size_t	bsize;

#if 0
	if ( BTST(Pc->Card_state, CardS_LinkUp) ) {
#else
	if ( Pc->Meta_state == ND_NSR_ACTIVE ) {
#endif
		/* TODO: pass transmit data backup receive side -- loopback */

		head = &Pc->Card_txrdyq.Q;
		UDI_QUEUE_FOREACH( head, elem, tmp ) {
			pcb = (Pseudo_cb_t *)UDI_DEQUEUE_HEAD(head);
			Pc->Card_txrdyq.numelem--;
			txcb = UDI_MCB( pcb->cbt, udi_nic_tx_cb_t );
			PSEUDOND_ASSERT( txcb->tx_buf != NULL );

			/* Put pcb back onto txatq for future use */
			UDI_ENQUEUE_TAIL(&Pc->Card_txatq.Q, (udi_queue_t *)pcb);
			Pc->Card_txatq.numelem++;

			/* Get a Rx queue element */
			PSEUDOND_ASSERT( !UDI_QUEUE_EMPTY(&Pc->Card_rxcbq.Q) );
			pcb=(Pseudo_cb_t *)UDI_DEQUEUE_HEAD(&Pc->Card_rxcbq.Q);
			Pc->Card_rxcbq.numelem--;

			rxcb = UDI_MCB( pcb->cbt, udi_nic_rx_cb_t );
#if 0
			PSEUDOND_ASSERT( rxcb->rx_buf != NULL );
#endif

			/*
			 * Enqueue onto rxdoq, this will be moved to rxdoneq
			 * when the asynchronous udi_buf_copy() has completed
			 * Note: we overwite the Rx buffer provided by the NSR.
			 * Before calling udi_buf_copy() we must set the buffer
			 * size to the amount of data being looped back. There
			 * is always max_pdu of backing store associated with
			 * the Rx buffer.
			 */
			UDI_ENQUEUE_TAIL(&Pc->Card_rxdoq.Q, (udi_queue_t *)pcb);
			Pc->Card_rxdoq.numelem++;
			bsize = txcb->tx_buf->buf_size;
			PSEUDOND_ASSERT( bsize > 0 );
			/* Save Tx CB so we can free buffer when Rx runs */
			pcb->cb_scratch = (void *)txcb;
			rxcb->rx_buf->buf_size = bsize;		/* Rx size */
			udi_buf_copy( pseudo_nd_rxbuf, Pc->Card_Tx_cb,
				      txcb->tx_buf, 0, bsize,
				      rxcb->rx_buf, 0, bsize,	/* Overwrite */
				      UDI_NULL_BUF_PATH );

		}
	} else {
		/* Put internal queue CB onto free-list (_txcbq) */

		head = &Pc->Card_txrdyq.Q;
		UDI_QUEUE_FOREACH( head, elem, tmp ) {
			pcb = (Pseudo_cb_t *)UDI_DEQUEUE_HEAD(head);
			Pc->Card_txrdyq.numelem--;
			txcb = UDI_MCB( pcb->cbt, udi_nic_tx_cb_t );
			PSEUDOND_ASSERT( txcb->tx_buf == NULL );

			UDI_ENQUEUE_TAIL(&Pc->Card_txcbq.Q, (udi_queue_t *)pcb);
			Pc->Card_txcbq.numelem++;
		}

#if 0 || OLD_RECYCLE
		Pseudo_ND_Free_Resources( Pc );
#endif	/* OLD_RECYCLE */
	}
}

/*
 * Called on completion of udi_buf_copy() in the transmit case.
 * We move the first queue element from _rxdoq to _rxdoneq and update the
 * rx_buf entry with the newly created buffer. This will then be passed to
 * the NSR by calling udi_nsr_rx_ind()
 * We also free the associated Tx buffer, and post the Tx CB back to the NSR.
 */
STATIC void
pseudo_nd_rxbuf(
	udi_cb_t	*gcb,
	udi_buf_t	*new_buf )
{
	Pseudo_Card_t	*Pc = gcb->context;
	Pseudo_cb_t	*pcb;
	udi_queue_t	*elem;
	udi_nic_rx_cb_t	*rxcb;
	udi_nic_tx_cb_t	*txcb;

	PSEUDOND_ASSERT( !UDI_QUEUE_EMPTY(&Pc->Card_rxdoq.Q) );
	elem = UDI_DEQUEUE_HEAD( &Pc->Card_rxdoq.Q );
	Pc->Card_rxdoq.numelem--;

	pcb = (Pseudo_cb_t *)elem;
	rxcb = UDI_MCB( pcb->cbt, udi_nic_rx_cb_t );
	rxcb->rx_buf = new_buf;
	rxcb->chain  = NULL;		/* Single packet only */
	rxcb->rx_status = 0;		/* Everything AOK */
	rxcb->addr_match = UDI_NIC_RX_UNKNOWN;

	/*
	 * Release buffer associated with Tx CB, and tell the NSR that this is
	 * available for re-use
	 */
	txcb = (udi_nic_tx_cb_t *)pcb->cb_scratch;
	udi_buf_free( txcb->tx_buf );
	txcb->tx_buf = NULL;
	txcb->chain = NULL;

	udi_nsr_tx_rdy( txcb );


	/* Add request to list of Rx items to be posted to NSR */
	UDI_ENQUEUE_TAIL(&Pc->Card_rxdoneq.Q, elem);
	Pc->Card_rxdoneq.numelem++;

	/* Squirt the data up to the NSR */
	pseudo_nd_receive( Pc );
}

/*
 * Pseudo Receive routine. Takes the contents of the rxdoneq and passes them
 * up to the NSR. In a real driver this would happen in interrupt context.
 */
STATIC void
pseudo_nd_receive(
	Pseudo_Card_t	*Pc )
{
	Pseudo_cb_t	*pcb;
	udi_queue_t	*head, *elem, *tmp, *rxelem;
	udi_nic_rx_cb_t	*rxcb;

	PSEUDOND_ASSERT( Pc->Meta_state == ND_NSR_ACTIVE );

	head = &Pc->Card_rxdoneq.Q;
	UDI_QUEUE_FOREACH( head, elem, tmp ) {
		rxelem = UDI_DEQUEUE_HEAD( head );
		Pc->Card_rxdoneq.numelem--;

		pcb = (Pseudo_cb_t *)rxelem;

		rxcb = UDI_MCB( pcb->cbt, udi_nic_rx_cb_t );
		pcb->cbt = NULL;		/* Stale from now on */

		/* 
		 * Make this queue entry available for subsequent udi_nd_rx_rdy
		 */
		UDI_ENQUEUE_TAIL( &Pc->Card_rxatq.Q, rxelem );
		Pc->Card_rxatq.numelem++;
		
		Pc->Cardstat_ipackets++;

		/* Pass up to NSR */

		udi_nsr_rx_ind( rxcb );
	}
}

/*
 * ============================================================================
 * Internal resource allocation routines. This provides a centralised place
 * for performing all allocations which require asynchronous callbacks.
 * On completion of the allocation Pseudo_Alloc_Resources_Done will be called
 */
void
Pseudo_ND_Alloc_Resources( 
	Pseudo_Card_t *Pc )
{
	/* Allocate all resources needed to perform data Tx from NSR->ND */
	BCLR(Pc->Card_state, CardS_AllocFail);
	udi_mem_alloc( pseudo_nd_gotcb, 
			Pc->Card_Tx_cb,
			sizeof( Pseudo_cb_t ),
			0 );
}

void
Pseudo_ND_Alloc_Resources_Done(
	Pseudo_Card_t	*Pc,
	udi_status_t	status )
{
	Pseudo_cb_t	*pcb;
	udi_nic_tx_cb_t	*txcb;
	udi_queue_t	*head, *elem, *tmp, *txelem;

	DBG_MGMT(("Pseudo_ND_Alloc_Resources_Done( State = %s, status = %d )\n",
		 state_names[Pc->Meta_state], status));

	/*
	 * TODO: Issue a udi_nsr_tx_rdy() to inform the NSR that we're ready
	 *	 to go
	 */
	BSET(Pc->Card_state, (CardS_LinkUp|CardS_Enabled));
	BCLR(Pc->Card_state, CardS_Enabling);

	Pc->Meta_state = ND_NSR_ENABLED;

	udi_nsr_enable_ack( UDI_MCB(Pc->Card_link_cb, udi_nic_cb_t ),
			    status );
	
	/*
	 * Make NSR aware of our Transmit CBs. We've chained together the
	 * udi_nic_tx_cb_t's and must just dequeue our internal queue
	 * structures.
	 */
	
	head = &Pc->Card_txcbq.Q;
	elem = UDI_FIRST_ELEMENT( head );
	pcb = (Pseudo_cb_t *)elem;
	txcb = UDI_MCB( pcb->cbt, udi_nic_tx_cb_t );
	
	UDI_QUEUE_FOREACH( head, elem, tmp ) {
		txelem = UDI_DEQUEUE_HEAD( head );
		Pc->Card_txcbq.numelem--;
		pcb = (Pseudo_cb_t *)txelem;
		pcb->cbt = NULL;
		UDI_ENQUEUE_TAIL( &Pc->Card_txatq.Q, txelem );
		Pc->Card_txatq.numelem++;
	}

	/* Send the whole tx_cb list up to NSR */
	udi_nsr_tx_rdy( txcb );

	/* NSR is informed of link-up when we've received the udi_nd_rx_rdy */

	/*
	 * NOTE: If the NSR has not released the previously allocated RX CBs
	 *	 we must issue a LINK-UP status indication to complete the
	 *	 subsequent enable request
	 */
	if (Pc->Card_rxcbq.numelem > 0) {
		DBG_ND(("Pseudo_ND_Alloc_Resources_Done: already got %d RX cbs"
			"faking LINK-UP\n", Pc->Card_rxcbq.numelem));
		/* Tell the NSR that the link is physically up */
		Pc->Card_link_status = UDI_NIC_LINK_UP;
		udi_cb_alloc( pseudo_nd_status,Pc->Card_NSR_cb,
			PSEUDO_NET_STATUS_CB_IDX, Pc->Card_NSR);
	}
}

/*
 * pseudo_nd_status:
 * -----------------
 * Inform the NSR of a change in the ND status [Pc->Card_link_status]. The
 * NSR is reponsible for freeing this CB
 */
STATIC void
pseudo_nd_status(
	udi_cb_t	*gcb,
	udi_cb_t	*new_cb )
{
	Pseudo_Card_t	*Pc = gcb->context;
	udi_nic_status_cb_t  *stat_cb = UDI_MCB(new_cb, udi_nic_status_cb_t );
	udi_time_t	defer_intvl;
	udi_ubit8_t	event;			/* Local copy */

	stat_cb->event = Pc->Card_link_status;
	event = stat_cb->event;

	DBG_ND(("pseudo_nd_status( State = %s, event = %d )\n",
		state_names[Pc->Meta_state], stat_cb->event));

	/*
	 * Only send a status_ind if we're in a valid state to do so.
	 * If the state is invalid, simply free the cb
	 */
	if ( Pc->Meta_state == ND_NSR_UNBINDING ) {
		/* Complete the unbind if all of the resources have been freed*/
			udi_cb_free( new_cb );

			udi_cb_free( Pc->Card_Tx_cb );
			udi_cb_free( Pc->Card_Rx_cb );
			udi_cb_free( Pc->Card_NSR_cb );

			udi_nsr_unbind_ack( UDI_MCB(Pc->Card_scratch_cb,
						    udi_nic_cb_t ), UDI_OK );
			Pc->Meta_state = ND_UNBOUND;
			return;
	}

	if ( Pc->Meta_state == ND_NSR_DISABLING ) {
		/* Process the deferred operation after a small delay */
		if ( ! UDI_QUEUE_EMPTY( &Pc->Card_defer_op.Q ) ) {
		    DBG_ND(("pseudo_nd_status: Start deferred processing\n"));
			defer_intvl.seconds = 1;
			defer_intvl.nanoseconds = 0;

			udi_timer_start(pseudo_nd_defer, new_cb, defer_intvl);
			return;
		} else {
			/* Transfer to BOUND state */
			BCLR(Pc->Card_state, CardS_Disabling);
			Pc->Meta_state = ND_NSR_BOUND;
			udi_cb_free( new_cb );
			return;
		}
	}
	
#if 1
	if ( Pc->Meta_state != ND_NSR_ENABLED && Pc->Meta_state != ND_NSR_ACTIVE ) {
		udi_cb_free( new_cb );
		return;
	}
#endif

	/*
	 * Post the new status [Pc->Card_link_status] to the NSR
	 */
	switch ( stat_cb->event ) {
	case UDI_NIC_LINK_UP:
		Pc->Meta_state = ND_NSR_ACTIVE;
		break;
	case UDI_NIC_LINK_DOWN:
	case UDI_NIC_LINK_RESET:
		if ( BTST(Pc->Card_state, CardS_Disabling) ) {
			Pc->Meta_state = ND_NSR_BOUND;
			BCLR(Pc->Card_state, CardS_Disabling);
		} else {
			Pc->Meta_state = ND_NSR_ENABLED;
		}
		break;
	}

	udi_nsr_status_ind( stat_cb );

	/*
	 * Cannot reference stat_cb any more...
	 */

	if ( event == UDI_NIC_LINK_RESET ) {
		/*
		 * schedule a UDI_NIC_LINK_UP for a couple of seconds time.
		 * This will simulate the off/on transition that would
		 * normally occur with a hardware reset
		 */
		defer_intvl.nanoseconds = 0;
		defer_intvl.seconds = 2;

		udi_timer_start(pseudo_nd_stat_trans, gcb, defer_intvl);
	}
}

/*
 * Schedule a UDI_NIC_LINK_UP event
 */
STATIC void
pseudo_nd_stat_trans(
	udi_cb_t	*gcb )
{
	Pseudo_Card_t	*Pc = gcb->context;

	Pc->Card_link_status = UDI_NIC_LINK_UP;

	udi_cb_alloc(pseudo_nd_status, gcb, PSEUDO_NET_STATUS_CB_IDX, 
			Pc->Card_NSR);
}

/*
 * pseudo_nd_defer:
 * -----------------
 * Process the first deferred operation after the card has completed its
 * disabling processing.
 */
STATIC void
pseudo_nd_defer(
	udi_cb_t	*gcb )
{
	Pseudo_Card_t	*Pc = gcb->context;
	pseudo_defer_el_t	*elp;

	elp = (pseudo_defer_el_t *)UDI_DEQUEUE_HEAD(&Pc->Card_defer_op.Q);

	DBG_ND(("pseudo_nd_defer, State = %s, Deferred Op = %s\n",
		state_names[Pc->Meta_state],
		op_names[elp->op]));
	
	Pc->Meta_state = ND_NSR_BOUND;
	BCLR(Pc->Card_state, CardS_Disabling);

	switch ( elp->op ) {
	case _udi_nd_enable_req:
		pseudo_nd_enable_req( elp->cb );
		break;
	case _udi_nd_unbind_req:
		pseudo_nd_unbind_req( elp->cb );
		break;
	default:
		PSEUDOND_ASSERT( 0 );
	}

	udi_cb_free( gcb );
	udi_mem_free( elp );
}

void
Pseudo_ND_Free_Resources(
	Pseudo_Card_t *Pc )
{
	/*
	 * Free all resources that have been allocated.
	 * Note: the NSR may have some Tx CBs outstanding, these will be
	 * 	 released by subsequent udi_nd_tx_req calls. Once all CBs are
	 *	 under our control (on Card_txcbq) we can free them up
	 */

	DBG_ND(("Pseudo_ND_Free_Resources: State %s # txatq = %d, # txcbq = %d\n",
		state_names[Pc->Meta_state],
		Pc->Card_txatq.numelem, Pc->Card_txcbq.numelem));
	
	PSEUDOND_ASSERT( Pc->Meta_state == ND_NSR_DISABLING ||
			 Pc->Meta_state == ND_NSR_UNBINDING );
	
	if ( Pc->Card_txcbq.numelem == NUM_TX_PDL ) {
		/*
		 * All of the CBs are under our control. We can free the whole
		 * lot up now
		 */
		pseudo_nd_free_rsrc( Pc );
	} else if (Pc->Card_txcbq.numelem == 0 && Pc->Card_txatq.numelem == 0) {
		/*
		 * There's nothing to free, just indicate a UDI_NIC_LINK_DOWN
		 */
		Pc->Card_link_status = UDI_NIC_LINK_DOWN;
		pseudo_nd_free_rsrc( Pc );
	}

}

STATIC void
pseudo_nd_free_rsrc(
	Pseudo_Card_t	*Pc)
{
	udi_ubit32_t	ii;
	Pseudo_cb_t	*pcb;
	udi_queue_t	*head, *elem, *tmp;
	udi_nic_rx_cb_t	*rxcb;

	for (ii = 0x80000000; ii > 0; ii = ii >> 1) {
		if (!BTST(Pc->Card_resources, ii)) continue;
		switch (ii) {
		case CardR_TX_CB:
			head = &Pc->Card_txcbq.Q;
			UDI_QUEUE_FOREACH(head, elem, tmp) {
				pcb = (Pseudo_cb_t *)elem;
				PSEUDOND_ASSERT( pcb->cbt != NULL );
				udi_cb_free( pcb->cbt );
				pcb->cbt = NULL;
				UDI_QUEUE_REMOVE( elem );
				Pc->Card_txcbq.numelem--;
				udi_mem_free( pcb );
			}
			break;
		case CardR_RX_CB:
			/*
			 * Release any CB [and buffer ?] which the NSR
			 * passed to us but we haven't yet used
			 */
			head = &Pc->Card_rxcbq.Q;
			DBG_ND(("ND: #Card_rxcbq = %d\n", 
				Pc->Card_rxcbq.numelem));
			UDI_QUEUE_FOREACH(head, elem, tmp) {
				UDI_QUEUE_REMOVE( elem );
				pcb = (Pseudo_cb_t *)elem;
				rxcb = UDI_MCB( pcb->cbt, udi_nic_rx_cb_t );
				udi_buf_free( rxcb->rx_buf );
				rxcb->rx_buf = NULL;
				udi_cb_free( pcb->cbt );
				Pc->Card_rxcbq.numelem--;
				udi_mem_free( elem );
			}
			head = &Pc->Card_rxatq.Q;
			DBG_ND(("ND: #Card_rxatq = %d\n", 
				Pc->Card_rxatq.numelem));
			UDI_QUEUE_FOREACH(head, elem, tmp) {
				UDI_QUEUE_REMOVE( elem );
				Pc->Card_rxatq.numelem--;
				udi_mem_free( elem );
			}
			head = &Pc->Card_rxdoq.Q;
			DBG_ND(("ND: #Card_rxdoq = %d\n", 
				Pc->Card_rxdoq.numelem));
			UDI_QUEUE_FOREACH(head, elem, tmp) {
				UDI_QUEUE_REMOVE( elem );
				Pc->Card_rxdoq.numelem--;
				udi_mem_free( elem );
			}
			head = &Pc->Card_rxdoneq.Q;
			DBG_ND(("ND: #Card_rxdoneq = %d\n", 
				Pc->Card_rxdoneq.numelem));
			UDI_QUEUE_FOREACH(head, elem, tmp) {
				UDI_QUEUE_REMOVE( elem );
				Pc->Card_rxdoneq.numelem--;
				udi_mem_free( elem );
			}
			break;
		default:
			break;
		}
		BCLR(Pc->Card_resources, ii);
	}

	/* Release any cbq entries which were created [BUG-107971]*/
	UDI_QUEUE_FOREACH(&Pc->cbq.Q, elem, tmp) {
		UDI_QUEUE_REMOVE( elem );
		Pc->cbq.numelem--;
		udi_mem_free( elem );
 	}
	
	/* Reset the request field for subsequent allocation */
	Pc->Card_resource_rqst = 0;

	/* Inform the NSR of the link status change */
	udi_cb_alloc(pseudo_nd_status,Pc->Card_NSR_cb,PSEUDO_NET_STATUS_CB_IDX,
		     Pc->Card_NSR);
}

STATIC void
pseudo_nd_pcb_alloc(
	udi_cb_t	*gcb,
	void		*new_mem )
{
	Pseudo_Card_t	*Pc = gcb->context;

	DBG_MGMT(("pseudo_nd_pcb_alloc entry state %s\n",
		  state_names[Pc->Meta_state]));

	PSEUDOND_ASSERT( Pc->cbq.numelem == 0 );

	UDI_ENQUEUE_TAIL( &Pc->cbq.Q, (udi_queue_t *)new_mem );
	Pc->cbq.numelem++;

	/* Resume interrupted pseudo_nd_getnext_rsrc call */
	pseudo_nd_getnext_rsrc( Pc );
}

STATIC void
pseudo_nd_getnext_rsrc(
	Pseudo_Card_t	*Pc)
{
	Pseudo_cb_t	*pcb = NULL;
	udi_ubit32_t	ii;

	if (BTST(Pc->Card_state, CardS_AllocFail)) return;

	if (BTST(Pc->Card_resources, 0x80000000)) {
		Pseudo_ND_Alloc_Resources_Done(Pc, UDI_OK);
		return;
	}

	/* 
	 * Allocate a control block to allow us to run to completion if the
	 * pre-allocated udi_nd_bind_req one has already been processed without
	 * an intervening unbind/bind call
	 */
	if ( Pc->cbq.numelem == 0 ) {
		udi_mem_alloc( pseudo_nd_pcb_alloc, Pc->Card_NSR_cb, 
		       	sizeof( Pseudo_cb_t ), 0 );
		return;
	}

	for (ii = 1; ii < 0x80000000; ii = ii << 1) {
		if (BTST(Pc->Card_resource_rqst, ii)) continue;
		if (BTST(Pc->Card_resources, ii)) continue;

		if ( Pc->cbq.numelem == 0 ) {
			DBG_MGMT(("pseudo_nd_getnext_rsrc: Exhausted cbq\n"));
			return;
		} else {
			pcb = (Pseudo_cb_t *)UDI_DEQUEUE_HEAD(&Pc->cbq.Q);
			if ( pcb == NULL ) return;
			Pc->cbq.numelem--;
			CB_SETSCRATCH( pcb, ii );
		}
		BSET(Pc->Card_resource_rqst, ii);
		switch( ii ) {
			case CardR_TX_CB:
				udi_mem_alloc( pseudo_nd_got_txmem,
					       Pc->Card_Tx_cb,
					       sizeof( Pseudo_cb_t ),
					       0 );
				udi_mem_free( pcb );	/* Check This! */
				pcb = NULL;
				break;
			case CardR_RX_CB:
				udi_mem_alloc( pseudo_nd_got_rxmem,
					       Pc->Card_Rx_cb,
					       sizeof( Pseudo_cb_t ),
					       0 );
				udi_mem_free( pcb );
				pcb = NULL;
				break;
			default:
				UDI_ENQUEUE_TAIL(&Pc->cbq.Q,(udi_queue_t *)pcb);
				Pc->cbq.numelem++;
				BSET(Pc->Card_resources, ii);
				pcb = NULL;
				break;
		}
	}

	if ( Pc->Card_resources == 0x7fffffff) {
		BSET(Pc->Card_resources, 0x80000000);
		while ( Pc->cbq.numelem > 0 ) {
			pcb = (Pseudo_cb_t *)UDI_DEQUEUE_HEAD(&Pc->cbq.Q);
			Pc->cbq.numelem--;
			udi_mem_free(pcb);
		}
		Pseudo_ND_Alloc_Resources_Done(Pc, UDI_OK);
	}
}

/*
 * Callback routine for Pseudo_cb_t udi_mem_alloc. This data is used for
 * internal queuing of Rx packets which will be sent to the NSR. The actual
 * CB and buffer is provided by the NSR via the udi_nd_rx_rdy() interface.
 *
 * Issue: Should we expect the maximum # of Rx cbs to be rx_hw_threshold? This
 *	  seems to be the easiest to handle as we don't then have to special
 *	  case all the Rx CB queue handling code.
 */
STATIC void
pseudo_nd_got_rxmem(
	udi_cb_t	*gcb,
	void		*new_mem )
{
	Pseudo_Card_t	*Pc = gcb->context;
	Pseudo_cb_t	*pcb = new_mem;

	pcb->cb_card = Pc;
	UDI_ENQUEUE_TAIL( &Pc->Card_rxatq.Q, (udi_queue_t *)pcb);
	Pc->Card_rxatq.numelem++;

	if (Pc->Card_rxatq.numelem < NUM_RX_PDL) {
		udi_mem_alloc( pseudo_nd_got_rxmem,
			       Pc->Card_Rx_cb,
			       sizeof( Pseudo_cb_t ),
			       0 );
	} else {
		/* We're done, flag it and continue with the next allocation */
		BSET(Pc->Card_resources, CardR_RX_CB);
		udi_mem_alloc( pseudo_nd_gotcb, 
				Pc->Card_Rx_cb,
				sizeof( Pseudo_cb_t ),
				0 );
	}
}

/*
 * Callback routine for Pseudo_cb_t udi_mem_alloc. This is needed to contain
 * the underlying udi_nic_XX_cb_t which will be used by the NSR.
 * We simply initialise the new element by adding it to the end of the
 * Card_txcbq and complete the initialisation in pseudo_nd_got_txcb
 */
STATIC void
pseudo_nd_got_txmem(
	udi_cb_t	*gcb,
	void		*new_mem )
{
	Pseudo_Card_t	*Pc = gcb->context;
	Pseudo_cb_t	*pcb = new_mem;

	pcb->cb_card = Pc;
	UDI_ENQUEUE_TAIL(&Pc->Card_txcbq.Q, (udi_queue_t *)pcb);
	Pc->Card_txcbq.numelem++;

	/* Allocate the udi_nic_tx_cb_t for use */
	udi_cb_alloc( pseudo_nd_got_txcb,
		      Pc->Card_Tx_cb,
		      PSEUDO_NET_TX_CB_IDX,
		      Pc->Card_Tx_chan );
}

/*
 * Callback routine for Tx CB. Issue another allocation request if we don't
 * have sufficient CBs (rx_hw_threshold)
 */
STATIC void
pseudo_nd_got_txcb(
	udi_cb_t	*gcb,
	udi_cb_t	*new_cb )
{
	Pseudo_Card_t	*Pc = gcb->context;
	Pseudo_cb_t	*pcb;
	udi_nic_tx_cb_t	*txcb = UDI_MCB(new_cb, udi_nic_tx_cb_t );

	pcb = (Pseudo_cb_t *)UDI_LAST_ELEMENT(&Pc->Card_txcbq.Q);

	pcb->cbt = new_cb;

	/*
	 * Chain the new tx_cb to the end of the list
	 */
	if (Pc->Card_txcbq.numelem > 1 ) {
		Pseudo_cb_t	*tail;
		udi_nic_tx_cb_t	*prevtx;

		tail = (Pseudo_cb_t *)UDI_PREV_ELEMENT( (udi_queue_t *)pcb );
		prevtx = (udi_nic_tx_cb_t *)tail->cbt;
		prevtx->chain = txcb;
	}
				
	if (Pc->Card_txcbq.numelem < NUM_TX_PDL) {
		udi_mem_alloc( pseudo_nd_got_txmem,
			       Pc->Card_Tx_cb,
			       sizeof( Pseudo_cb_t ),
			       0 );
	} else {
		/* We're done, flag it and continue with the next allocation */
		BSET(Pc->Card_resources, CardR_TX_CB);
		udi_mem_alloc( pseudo_nd_gotcb, 
				Pc->Card_Tx_cb,
				sizeof( Pseudo_cb_t ),
				0 );
	}
}

/*
 * Callback routine for generic Pseudo_cb_t token allocation. This is used to
 * allow another request to be queued up for despatch by pseudo_nd_getnext_rsrc
 * There has to be a simpler way to do this ...
 */
STATIC void
pseudo_nd_gotcb(
	udi_cb_t	*gcb,
	void		*new_mem )
{
	Pseudo_Card_t	*Pc = gcb->context;
	udi_queue_t	*qp = new_mem;

	UDI_ENQUEUE_TAIL(&Pc->cbq.Q, qp);
	Pc->cbq.numelem++;
	pseudo_nd_getnext_rsrc( Pc );
}

/*
 *==============================================================================
 * Bus Bridge Interface
 *==============================================================================
 */

STATIC void
pseudond_bus_channel_event(
	udi_channel_event_cb_t *cb )
{
	udi_bus_bind_cb_t *bus_bind_cb = 
			UDI_MCB(cb->params.parent_bound.bind_cb, 
				udi_bus_bind_cb_t );
	switch( cb->event ) {
	case UDI_CHANNEL_BOUND:
		/* Keep a link back to the channel event CB for the ack. */
		UDI_GCB(bus_bind_cb)->initiator_context = cb;
		udi_bus_bind_req( bus_bind_cb );
		break;
	default:
		udi_channel_event_complete(cb, UDI_OK);
	}
}

STATIC void
pseudond_bus_bind_ack(
	udi_bus_bind_cb_t	*bus_bind_cb,
	udi_dma_constraints_t	dma_constraints,
	udi_ubit8_t		preferred_endianness,
	udi_status_t		status )
{
	udi_channel_event_cb_t	*channel_event_cb =
					UDI_GCB(bus_bind_cb)->initiator_context;
	Pseudo_Card_t		*Pc = UDI_GCB(bus_bind_cb)->context;

	/*
	 * As we don't have any hardware we can simply complete the 
	 * UDI_CHANNEL_BOUND event after stashing the bus_bind_cb for the
	 * subsequent unbind()
	 */
	
	Pc->Card_BUS_cb = bus_bind_cb;
	Pc->Card_dmacon = dma_constraints;
	udi_dma_constraints_free(dma_constraints);
	udi_channel_event_complete(channel_event_cb, status);
}

STATIC void
pseudond_bus_unbind_ack(
	udi_bus_bind_cb_t	*bus_bind_cb )
{
	udi_mgmt_cb_t	*cb = bus_bind_cb->gcb.initiator_context;

	udi_cb_free(UDI_GCB(bus_bind_cb));

	udi_devmgmt_ack(cb, 0, UDI_OK);
}
