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
/* shrkudi - sample UDI driver for Osicom 2300 */
/*
 * The following diagram shows the channel and regions in this driver:
 *				_____________________
 *				|	            |
 *				|        NSR        |
 *				|	            |
 *				--^----^------^------  [Child]
 *				  |    |      |
 *				 3|   5|     6|
 *				__v____v______v_______ [Parent]
 *	                        |   Network Driver   |
 *	__________	      0 | primary  |   rx    |
 *	|  Mgmt  |<------------>|  region  | region  |
 *      |  Agent |              |          |         |
 *	----------              |provider<1-2>client |
 *	           		-----+---------^---^-- [Child]
 *				    (h)       4|  7|
 *				     |         |   |
 *				_____v_________v___v__ [Parent]
 *	        	        |	             |
 *				|     Bus Bridge     |
 *				|                    |
 *				----------------------
 *				         NIC
 *
 *	0:	Management Metalanguage channel - controls driver configuration
 *			and operating conditions; driven by Management Agent
 *	1-2:	GIO Metalanguage inter-region channel - uses Generic I/O
 *			metalanguage to send status, etc. across regions
 *	3:	Network Metalanguage control channel - NSR initial bind channel
 *	4:	Bus Bridge control channel - Bus Bridge initial bind channel
 *	5:	Network Metalanguage transmit channel - controls tx data flow
 *	6:	Network Metalanguage receive channel - controls rx data flow
 *	7:	Bus Bridge Interrupt handler channel - handles interrupt events
 *	(h):	PIO handle - used for tx/setup I/O (not a channel)
 * -----------------------------------------------------------------------------
 */

#define UDI_VERSION 0x101
#define UDI_NIC_VERSION 0x101
#define UDI_PHYSIO_VERSION 0x101
#define UDI_PCI_VERSION 0x101

#include <udi.h>
#include <udi_nic.h>
#include <udi_physio.h>
#include <udi_pci.h>
#include "shrk.h"

#ifdef SHRK_DEBUG
/* TODO3 replace shrkdebug/DEBUG* debug output with tracing */
/* const udi_ubit32_t shrkdebug = 0xffff; */
const udi_ubit32_t shrkdebug = 0x00000842;
#endif

/* broadcast address for routing to this interface */
static const shrkaddr_t shrk_broad = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

/* MA (management agent) control entry points */
udi_usage_ind_op_t           shrk_usage_ind;
udi_enumerate_req_op_t       shrk_enumerate_req;
udi_devmgmt_req_op_t         shrk_devmgmt_req;
udi_final_cleanup_req_op_t   shrk_final_cleanup_req;

static udi_mgmt_ops_t shrk_mgmt_ops = {
	shrk_usage_ind,
	shrk_enumerate_req,
	shrk_devmgmt_req,
	shrk_final_cleanup_req,
};

/* network meta control entry points */
udi_channel_event_ind_op_t   shrk_nd_ctrl_channel_event_ind;
udi_nd_bind_req_op_t         shrk_nd_bind_req;
udi_nd_unbind_req_op_t       shrk_nd_unbind_req;
udi_nd_enable_req_op_t       shrk_nd_enable_req;
udi_nd_disable_req_op_t      shrk_nd_disable_req;
udi_nd_ctrl_req_op_t         shrk_nd_ctrl_req;
udi_nd_info_req_op_t         shrk_nd_info_req;

static udi_nd_ctrl_ops_t shrk_ctrl_ops = {
	shrk_nd_ctrl_channel_event_ind,
	shrk_nd_bind_req,
	shrk_nd_unbind_req,
	shrk_nd_enable_req,
	shrk_nd_disable_req,
	shrk_nd_ctrl_req,
	shrk_nd_info_req,
};

/* network meta transmit entry points */
udi_channel_event_ind_op_t   shrk_nd_tx_channel_event_ind;
udi_nd_tx_req_op_t           shrk_nd_tx_req;
udi_nd_exp_tx_req_op_t       shrk_nd_txexp_req;

static udi_nd_tx_ops_t shrk_tx_ops = {
	shrk_nd_tx_channel_event_ind,
	shrk_nd_tx_req,
	shrk_nd_txexp_req,
};

/* network meta receive entry points */
udi_channel_event_ind_op_t   shrkrx_nd_channel_event_ind;
udi_nd_rx_rdy_op_t           shrkrx_nd_rx_rdy;

static udi_nd_rx_ops_t shrk_rx_ops = {
	shrkrx_nd_channel_event_ind,
	shrkrx_nd_rx_rdy,
};

/* gio meta provider (primary region) entry points for inter-region channel */
udi_channel_event_ind_op_t   shrk_gio_channel_event_ind;   /* primary region */
udi_gio_bind_req_op_t        shrk_gio_bind_req;     /* client binds provider */
udi_gio_unbind_req_op_t      shrk_gio_unbind_req;   /* client unbind provider */
udi_gio_xfer_req_op_t        shrk_gio_xfer_req;     /* client req to provider */
udi_gio_event_res_op_t       shrk_gio_event_res;    /* client res to provider */

static udi_gio_provider_ops_t shrk_gio_provider_ops = {
	shrk_gio_channel_event_ind,
	shrk_gio_bind_req,   /* client binds provider */
	shrk_gio_unbind_req, /* client unbinds provider */
	shrk_gio_xfer_req,   /* udi_gio_xfer_req: inter-region data transfer */
	shrk_gio_event_res,  /* gio event response */
};

/* gio meta client (secondary region) entry points for inter-region channel */
udi_channel_event_ind_op_t   shrkrx_gio_channel_event_ind; /* 2ndary region */
udi_gio_bind_ack_op_t        shrkrx_gio_bind_ack;   /* provider acks bind */
udi_gio_unbind_ack_op_t      shrkrx_gio_unbind_ack; /* provider acks unbind */
udi_gio_xfer_ack_op_t        shrkrx_gio_xfer_ack;   /* provider acks xfer */
udi_gio_xfer_nak_op_t        shrkrx_gio_xfer_nak;   /* provider nacks xfer */
udi_gio_event_ind_op_t       shrkrx_gio_event_ind;  /* provider ind to client */

static udi_gio_client_ops_t shrk_gio_client_ops = {
	shrkrx_gio_channel_event_ind,
	shrkrx_gio_bind_ack,   /* provider acks bind */
	shrkrx_gio_unbind_ack, /* provider acks unbind */
	shrkrx_gio_xfer_ack,   /* provider acks inter-region data xfer */
	shrkrx_gio_xfer_nak,   /* provider naks inter-region data xfer */
	shrkrx_gio_event_ind,  /* gio event indication */
};

/* bus meta bridge-to-device entry points for rx (secondary) region */
udi_channel_event_ind_op_t   shrkrx_bus_channel_event_ind;
udi_bus_bind_ack_op_t        shrkrx_bus_bind_ack;
udi_bus_unbind_ack_op_t      shrkrx_bus_unbind_ack;
udi_intr_attach_ack_op_t     shrkrx_intr_attach_ack;
udi_intr_detach_ack_op_t     shrkrx_intr_detach_ack;

static udi_bus_device_ops_t shrk_bus_ops = {
	shrkrx_bus_channel_event_ind,
	shrkrx_bus_bind_ack,
	shrkrx_bus_unbind_ack,
	shrkrx_intr_attach_ack,
	shrkrx_intr_detach_ack,
};

/* bus meta interrupt entry points for rx (secondary) region */
udi_channel_event_ind_op_t   shrkrx_intr_channel_event_ind;
udi_intr_event_ind_op_t	     shrkrx_intr_event_ind;

static udi_intr_handler_ops_t shrk_intr_handler_ops = {
	shrkrx_intr_channel_event_ind,
	shrkrx_intr_event_ind
};

/*
 * a default op_flags without any special flags
 */
static const udi_ubit8_t shrk_default_op_flags[] = {
	0, 0, 0, 0, 0, 0, 0
};

/* udi operations vector registration for channel endpoints */
static udi_ops_init_t shrk_ops_init[] = {
	{
		SHRK_PRI_REG_IDX,       /* primary end of inter-region channel */
		SHRK_GIO_META,          /* my gio metalanguage index */
		UDI_GIO_PROVIDER_OPS_NUM, /* meta_ops_num */
		0,                      /* chan_context_size */
		(udi_ops_vector_t *) &shrk_gio_provider_ops,
		shrk_default_op_flags,	/* op_flags */
	},
	{
		SHRK_SEC_REG_IDX,       /* secondary end of inter-region channel */
		SHRK_GIO_META,          /* my gio metalanguage index */
		UDI_GIO_CLIENT_OPS_NUM, /* meta_ops_num */
		0,                      /* chan_context_size */
		(udi_ops_vector_t *) &shrk_gio_client_ops,
		shrk_default_op_flags,	/* op_flags */
	},
	{
		SHRK_ND_CTRL_IDX,       /* NSR initial bind channel */
		SHRK_NIC_META,          /* my network metalanguage index */
		UDI_ND_CTRL_OPS_NUM,    /* meta_ops_num */
		0,                      /* chan_context_size */
		(udi_ops_vector_t *) &shrk_ctrl_ops,
		shrk_default_op_flags,	/* op_flags */
	},
	{
		SHRK_BUS_BRIDGE_IDX,    /* bus initial bind channel */
		SHRK_BUS_META,          /* my bus bridge metalanguage index */
		UDI_BUS_DEVICE_OPS_NUM, /* meta_ops_num */
		0,                      /* chan_context_size */
		(udi_ops_vector_t *) &shrk_bus_ops,
		shrk_default_op_flags,	/* op_flags */
	},
	{
		SHRK_ND_TX_IDX,         /* NSR tx channel */
		SHRK_NIC_META,          /* my network metalanguage index */
		UDI_ND_TX_OPS_NUM,      /* meta_ops_num */
		0,                      /* chan_context_size */
		(udi_ops_vector_t *) &shrk_tx_ops,
		shrk_default_op_flags,	/* op_flags */
	},
	{
		SHRK_ND_RX_IDX,         /* NSR rx channel */
		SHRK_NIC_META,          /* my network metalanguage index */
		UDI_ND_RX_OPS_NUM,      /* meta_ops_num */
		0,                      /* chan_context_size */
		(udi_ops_vector_t *) &shrk_rx_ops,
		shrk_default_op_flags,	/* op_flags */
	},
	{
		SHRK_INTR_HANDLER_IDX,
		SHRK_BUS_META,	      /* meta index from udiprops.txt */
		UDI_BUS_INTR_HANDLER_OPS_NUM,
		0,				/* chan_context_size */
		(udi_ops_vector_t *) &shrk_intr_handler_ops,
		shrk_default_op_flags	/* op_flags */
	},
	{ 0 } /* end of ops list */
};

/* primary (transmit/ctrl) region and management meta initialization */
static udi_primary_init_t shrk_primary_init = {
	&shrk_mgmt_ops,
	shrk_default_op_flags,	/* op_flags */
	sizeof(udi_ubit32_t),	/* mgmt scratch_requirement */
	SHRK_ATTR_LEN, 		/* enumeration_attr_list_length */
	sizeof(shrktx_rdata_t), /* rdata_size */
	0, 				/* child_data_size */
	0				/* buf path */
};

/* secondary (receive) region initialization */
static udi_secondary_init_t shrk_rx_init[] = {
	{
		SHRK_RX_REGION, /* receive region index */
		sizeof(shrkrx_rdata_t)
	},
	{ 0 } /* end of list */
};

/* control block initialization */
static udi_cb_init_t shrk_cb_init_list[] = {
	{
		SHRK_NIC_STD_CB_IDX,        /* cb_idx */
		SHRK_NIC_META,              /* meta_idx */
		UDI_NIC_STD_CB_NUM,         /* meta_cb_num */
		sizeof(udi_ubit32_t),       /* scratch_requirement */
		0,                          /* inline_size */
		NULL                        /* inline_layout */
	},
	{
		SHRK_NIC_BIND_CB_IDX,       /* cb_idx */
		SHRK_NIC_META,              /* meta_idx */
		UDI_NIC_BIND_CB_NUM,        /* meta_cb_num */
		0,                          /* scratch_requirement */
		0,                          /* inline_size */
		NULL                        /* inline_layout */
	},
	{
		SHRK_NIC_CTRL_CB_IDX,       /* cb_idx */
		SHRK_NIC_META,              /* meta_idx */
		UDI_NIC_CTRL_CB_NUM,        /* meta_cb_num */
		sizeof(udi_ubit32_t),       /* scratch_requirement */
		0,                          /* inline_size */
		NULL                        /* inline_layout */
	},
	{
		SHRK_NIC_STATUS_CB_IDX,     /* cb_idx */
		SHRK_NIC_META,              /* meta_idx */
		UDI_NIC_STATUS_CB_NUM,      /* meta_cb_num */
		0,                          /* scratch_requirement */
		0,                          /* inline_size */
		NULL                        /* inline_layout */
	},
	{
		SHRK_NIC_INFO_CB_IDX,       /* cb_idx */
		SHRK_NIC_META,              /* meta_idx */
		UDI_NIC_INFO_CB_NUM,        /* meta_cb_num */
		0,                          /* scratch_requirement */
		0,                          /* inline_size */
		NULL                        /* inline_layout */
	},
	{
		SHRK_NIC_TX_CB_IDX,         /* cb_idx */
		SHRK_NIC_META,              /* meta_idx */
		UDI_NIC_TX_CB_NUM,          /* meta_cb_num */
		sizeof(shrk_dma_scratch_t), /* scratch_requirement */
		0,                          /* inline_size */
		NULL                        /* inline_layout */
	},
	{
		SHRK_NIC_RX_CB_IDX,         /* cb_idx */
		SHRK_NIC_META,              /* meta_idx */
		UDI_NIC_RX_CB_NUM,          /* meta_cb_num */
		sizeof(shrk_dma_scratch_t), /* scratch_requirement */
		0,                          /* inline_size */
		NULL                        /* inline_layout */
	},
	{
		SHRK_GIO_BIND_CB_IDX,       /* cb_idx */
		SHRK_GIO_META,              /* meta_idx */
		UDI_GIO_BIND_CB_NUM,        /* meta_cb_num */
		0, 			    /* scratch_requirement */
		0,                          /* inline_size */
		NULL                        /* inline_layout */
	},
	{
		SHRK_GIO_XFER_CB_IDX,       /* cb_idx */
		SHRK_GIO_META,              /* meta_idx */
		UDI_GIO_XFER_CB_NUM,        /* meta_cb_num */
		sizeof(shrk_gio_xfer_scratch_t), /* scratch_requirement */
		sizeof(shrk_gio_xfer_params_t),  /* inline_size */
		shrk_gio_xfer_params_layout /* inline_layout */
	},
	{
		SHRK_GIO_EVENT_CB_IDX,      /* cb_idx */
		SHRK_GIO_META,              /* meta_idx */
		UDI_GIO_EVENT_CB_NUM,       /* meta_cb_num */
		sizeof(udi_ubit32_t),       /* scratch_requirement */
		sizeof(shrk_gio_event_params_t),  /* inline_size */
		shrk_gio_event_params_layout /* inline_layout */
	},
	{
		SHRK_BUS_BIND_CB_IDX,       /* cb_idx */
		SHRK_BUS_META,              /* meta_idx */
		UDI_BUS_BIND_CB_NUM,        /* meta_cb_num */
		sizeof(shrk_constraints_set_scratch_t), /*scratch_requirement */
		0,                          /* inline_size */
		NULL                        /* inline_layout */
	},
	{
		SHRK_BUS_INTR_ATTACH_CB_IDX,/* cb_idx */
		SHRK_BUS_META,              /* meta_idx */
		UDI_BUS_INTR_ATTACH_CB_NUM, /* meta_cb_num */
		0,                          /* scratch_requirement */
		0,                          /* inline_size */
		NULL                        /* inline_layout */
	},
	{
		SHRK_BUS_INTR_EVENT_CB_IDX, /* cb_idx */
		SHRK_BUS_META,              /* meta_idx */
		UDI_BUS_INTR_EVENT_CB_NUM,  /* meta_cb_num */
		sizeof(udi_ubit32_t),       /* scratch_requirement */
		0,                          /* inline_size */
		NULL                        /* inline_layout */
	},
	{
		SHRK_BUS_INTR_DETACH_CB_IDX,/* cb_idx */
		SHRK_BUS_META,              /* meta_idx */
		UDI_BUS_INTR_DETACH_CB_NUM, /* meta_cb_num */
		0,                          /* scratch_requirement */
		0,                          /* inline_size */
		NULL                        /* inline_layout */
	},
	{ 0 } /* end of cb list */
};

static udi_gcb_init_t shrk_gcb_init_list[] = {
	{
		SHRK_GCB_CB_IDX,	         /* cb_idx */
		0				 /* scratch_requirement */
	},
	{
		SHRK_PROBE_CB_IDX,	         /* cb_idx */
		sizeof(shrk_probe_scratch_t) /* scratch_requirement */
	},
	{ 0 } /* end of cb list */
};

/* udi environment initialization */ 
udi_init_t udi_init_info = {
	&shrk_primary_init,
	shrk_rx_init,
	shrk_ops_init, /* channel operations */
	shrk_cb_init_list, /* control block initialization */
	shrk_gcb_init_list, /* generic control block initialization */
	NULL  /* cb_select_list */
};


/*
 *-----------------------------------------------------------------------------
 * PIO trans lists
 */
/* read serial ROM */
static udi_pio_trans_t shrk_srom_read_pio[] = {
	/* R1, R3, R5, and R7 aren't changed throughout the loops */
	/* R1 = srom chip select */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_2BYTE,
	  ROM_WR | ROM_SR | ROM_SCS },
	/* R3 = srom chip select + clock */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R3, UDI_PIO_2BYTE,
	  ROM_WR | ROM_SR | ROM_SCLK | ROM_SCS },
	/* R5 = srom chip select + data_in */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R5, UDI_PIO_2BYTE,
	  ROM_WR | ROM_SR | ROM_SDIN | ROM_SCS },
	/* R7 = srom chip select + data_in + clock */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R7, UDI_PIO_2BYTE,
	  ROM_WR | ROM_SR | ROM_SDIN | ROM_SCLK | ROM_SCS },

	/* R0 = srom loop index and memory offset */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 0 },

	/* srom loop start */
	{ UDI_PIO_LABEL, 0, 1 },

	/* Initialize srom */
	/* R4 = srom select */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R4, UDI_PIO_2BYTE, ROM_WR | ROM_SR },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R4, UDI_PIO_4BYTE, SHRK_CSR9 },
	/* latch srom chip select */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_4BYTE, SHRK_CSR9 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R3, UDI_PIO_4BYTE, SHRK_CSR9 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_4BYTE, SHRK_CSR9 },

	/* Command Phase: output srom read command (110) */
	/* latch one */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R5, UDI_PIO_4BYTE, SHRK_CSR9 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R7, UDI_PIO_4BYTE, SHRK_CSR9 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R5, UDI_PIO_4BYTE, SHRK_CSR9 },
	/* latch one */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R7, UDI_PIO_4BYTE, SHRK_CSR9 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R5, UDI_PIO_4BYTE, SHRK_CSR9 },
	/* latch zero */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_4BYTE, SHRK_CSR9 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R3, UDI_PIO_4BYTE, SHRK_CSR9 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_4BYTE, SHRK_CSR9 },

	/* Address Phase: output 6-bit srom read address offset */
	/* R4 = address loop index */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R4, UDI_PIO_2BYTE, 6 },
	/* R2 = current ubit16 address in R0 */
	{ UDI_PIO_STORE+UDI_PIO_DIRECT+UDI_PIO_R2, UDI_PIO_2BYTE, UDI_PIO_R0 },

	/* address loop start */
	{ UDI_PIO_LABEL, 0, 2 },
	/* R6 = shifted address in R2 */
	{ UDI_PIO_STORE+UDI_PIO_DIRECT+UDI_PIO_R6, UDI_PIO_2BYTE, UDI_PIO_R2 },

	/* R6 = high order bit of 6-bit address (bit 5) */
	{ UDI_PIO_AND_IMM+UDI_PIO_R6, UDI_PIO_2BYTE, 0x40 },
	/* latch 0 if address bit is clear */
	{ UDI_PIO_CSKIP+UDI_PIO_R6, UDI_PIO_2BYTE, UDI_PIO_Z },
	{ UDI_PIO_BRANCH, 0, 3 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_4BYTE, SHRK_CSR9 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R3, UDI_PIO_4BYTE, SHRK_CSR9 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_4BYTE, SHRK_CSR9 },
	{ UDI_PIO_BRANCH, 0, 4 },
	{ UDI_PIO_LABEL, 0, 3 },
	/* latch 1 if address bit is set */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R5, UDI_PIO_4BYTE, SHRK_CSR9 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R7, UDI_PIO_4BYTE, SHRK_CSR9 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R5, UDI_PIO_4BYTE, SHRK_CSR9 },
	/* falls through */
	{ UDI_PIO_LABEL, 0, 4 },
	/* R2 <<= 1 (obtain next address bit) */
	{ UDI_PIO_SHIFT_LEFT+UDI_PIO_R2, UDI_PIO_2BYTE, 1 },

	/* decrement address loop index */
	{ UDI_PIO_ADD_IMM+UDI_PIO_R4, UDI_PIO_2BYTE, (udi_ubit16_t)-1 },
	{ UDI_PIO_CSKIP+UDI_PIO_R4, UDI_PIO_2BYTE, UDI_PIO_Z },
	{ UDI_PIO_BRANCH, 0, 2 },
	/* address loop end */

	/* Data Phase: read 16 bits of data, one bit at a time */
	/* R4 = data loop index */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R4, UDI_PIO_2BYTE, 16 },
	/* R6 = data */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R6, UDI_PIO_2BYTE, 0 },

	/* data loop start */
	{ UDI_PIO_LABEL, 0, 5 },

	/* shift data left one bit to make room for next bit */
	{ UDI_PIO_SHIFT_LEFT+UDI_PIO_R6, UDI_PIO_4BYTE, 1 },

	/* R2 = srom read, CLK high */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R2, UDI_PIO_2BYTE,
	  ROM_RD | ROM_SR | ROM_SCLK | ROM_SCS },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R2, UDI_PIO_4BYTE, SHRK_CSR9 },

	/* R2 = data */
	{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R2, UDI_PIO_4BYTE, SHRK_CSR9 },
	{ UDI_PIO_AND_IMM+UDI_PIO_R2, UDI_PIO_4BYTE, ROM_SDOUT },
	{ UDI_PIO_SHIFT_RIGHT+UDI_PIO_R2, UDI_PIO_4BYTE, 3 },

	/* copy data bit into R6 */
	{ UDI_PIO_OR+UDI_PIO_R6, UDI_PIO_4BYTE, UDI_PIO_R2 },

	/* R2 = srom read, CLK low */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R2, UDI_PIO_2BYTE,
	  ROM_RD | ROM_SR | ROM_SCS },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R2, UDI_PIO_4BYTE, SHRK_CSR9 },

	/* decrement data loop index R4 */
	{ UDI_PIO_ADD_IMM+UDI_PIO_R4, UDI_PIO_2BYTE, (udi_ubit16_t)-1 },
	{ UDI_PIO_CSKIP+UDI_PIO_R4, UDI_PIO_2BYTE, UDI_PIO_Z },
	{ UDI_PIO_BRANCH, 0, 5 },
	/* data loop end */

	/* copy data into memory */
	{ UDI_PIO_STORE+UDI_PIO_MEM+UDI_PIO_R0, UDI_PIO_2BYTE, UDI_PIO_R6 },

	/* disable srom */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R4, UDI_PIO_2BYTE, 0x0000ff00 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R4, UDI_PIO_4BYTE, SHRK_CSR9 },

	/* R0 (srom loop index and address offset) += 2 */
	{ UDI_PIO_ADD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 2 },
	/* R4 = max srom loop iterations */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R4, UDI_PIO_2BYTE, SHRK_SROMSIZE },
	{ UDI_PIO_SUB+UDI_PIO_R4, UDI_PIO_2BYTE, UDI_PIO_R0 },
	{ UDI_PIO_CSKIP+UDI_PIO_R4, UDI_PIO_2BYTE, UDI_PIO_Z },
	{ UDI_PIO_BRANCH, 0, 1 },
	/* srom loop end */

	/* end of transaction list */
	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 }
};

/* read MII register
 *
 * input scratch layout:
 *	dword 0 - command
 * output scratch layout:
 *	dword 0 - result
 */
static udi_pio_trans_t shrk_mii_read_pio[] = {
#if defined(SHRK_MII_DEBUG)
	{UDI_PIO_DEBUG, UDI_PIO_1BYTE, UDI_PIO_TRACE_DEV3},
#endif

	/* R7 = MII command */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 0 },
	{ UDI_PIO_LOAD+UDI_PIO_SCRATCH+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R7 },

	/* send 32 ones for sync */
	/* R0 = loop index */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 32 },
	/* R1 = MII data out */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_4BYTE, 0 },
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_4BYTE, (MII_MDO) >> 16 },
	/* R2 = MII data out + clock */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R2, UDI_PIO_4BYTE, 0 },
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R2, UDI_PIO_4BYTE, (MII_MDO|MII_MDC) >> 16 },

	/* sync loop start */
	{ UDI_PIO_LABEL, 0, 1 },

	/* data out */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_4BYTE, SHRK_CSR9 },
	/* data out + clock */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R2, UDI_PIO_4BYTE, SHRK_CSR9 },

	/* sync loop end */
	{ UDI_PIO_ADD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, (udi_ubit16_t)-1 },
	{ UDI_PIO_CSKIP+UDI_PIO_R0, UDI_PIO_2BYTE, UDI_PIO_Z },
	{ UDI_PIO_BRANCH, 0, 1 },

	/* send command */
	/* R0 = current bit in 14-bit command */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 0x2000 },
	/* R1 still == MII data out */
	/* R2 = clock */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R2, UDI_PIO_4BYTE, 0 },
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R2, UDI_PIO_4BYTE, MII_MDC >> 16 },

	/* command loop start */
	{ UDI_PIO_LABEL, 0, 2 },

	/* set MII data out in R3 if current bit in command is set */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R3, UDI_PIO_2BYTE, 0 },      /* R3 = 0   */
	{ UDI_PIO_LOAD+UDI_PIO_DIRECT+UDI_PIO_R7,
				   UDI_PIO_4BYTE, UDI_PIO_R4 }, /* R4 = R7  */
	{ UDI_PIO_AND+UDI_PIO_R4, UDI_PIO_4BYTE, UDI_PIO_R0 },  /* R4 &= R0 */
	{ UDI_PIO_CSKIP+UDI_PIO_R4, UDI_PIO_4BYTE, UDI_PIO_Z }, /* R4 == 0? */
	{ UDI_PIO_LOAD+UDI_PIO_DIRECT+UDI_PIO_R1,
				   UDI_PIO_4BYTE, UDI_PIO_R3 }, /* R3 = R1  */

	/* output R3 */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R3, UDI_PIO_4BYTE, SHRK_CSR9 },

	/* OR clock with R3 and output */
	{ UDI_PIO_OR+UDI_PIO_R3, UDI_PIO_4BYTE, UDI_PIO_R2 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R3, UDI_PIO_4BYTE, SHRK_CSR9 },

	/* command loop end */
	{ UDI_PIO_SHIFT_RIGHT+UDI_PIO_R0, UDI_PIO_2BYTE, 1 },
	{ UDI_PIO_CSKIP+UDI_PIO_R0, UDI_PIO_2BYTE, UDI_PIO_Z },
	{ UDI_PIO_BRANCH, 0, 2 },

        /* Read the two transition and 16 data bits (18 total) */
	/* R0 = loop index */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 18 },
	/* R1 = MII read enable */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_4BYTE, 0 },
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_4BYTE, MII_MII >> 16 },
	/* R2 still = clock */
	/* R3 = MII data in */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R3, UDI_PIO_4BYTE, 0 },
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R3, UDI_PIO_4BYTE, MII_MDI >> 16 },
	/* R7 = result; initialize to zero */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R7, UDI_PIO_2BYTE, 0 },

	/* read loop start */
	{ UDI_PIO_LABEL, 0, 3 },

	/* shift result left one bit */
	{ UDI_PIO_SHIFT_LEFT+UDI_PIO_R7, UDI_PIO_4BYTE, 1 },

	/* clock down, enable read */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_4BYTE, SHRK_CSR9 },

	/* read register into R5 */
	{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R5, UDI_PIO_4BYTE, SHRK_CSR9 },
	/* AND data with mask */
	{ UDI_PIO_AND+UDI_PIO_R5, UDI_PIO_4BYTE, UDI_PIO_R3 },  /* R5 &= R3 */
	/* set bit in result if data bit was set */
	{ UDI_PIO_CSKIP+UDI_PIO_R5, UDI_PIO_4BYTE, UDI_PIO_Z }, /* R5 == 0? */
	{ UDI_PIO_OR_IMM+UDI_PIO_R7, UDI_PIO_4BYTE, 1 },

	/* clock up */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R2, UDI_PIO_4BYTE, SHRK_CSR9 },

	/* read loop end */
	{ UDI_PIO_ADD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, (udi_ubit16_t)-1 },
	{ UDI_PIO_CSKIP+UDI_PIO_R0, UDI_PIO_2BYTE, UDI_PIO_Z },
	{ UDI_PIO_BRANCH, 0, 3 },

	/* store result */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 0 },
	{ UDI_PIO_STORE+UDI_PIO_SCRATCH+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R7 },

	/* end of transaction list */
	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 }
};

/* write MII register
 *
 * input scratch layout:
 *	dword 0 - command
 */
static udi_pio_trans_t shrk_mii_write_pio[] = {
#if defined(SHRK_MII_DEBUG)
	{UDI_PIO_DEBUG, UDI_PIO_1BYTE, UDI_PIO_TRACE_DEV3},
#endif

	/* R7 = MII command */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 0 },
	{ UDI_PIO_LOAD+UDI_PIO_SCRATCH+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R7 },

	/* send 32 ones for sync */
	/* R0 = loop index */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 32 },
	/* R1 = MII data out */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_4BYTE, 0 },
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_4BYTE, (MII_MDO) >> 16 },
	/* R2 = MII data out + clock */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R2, UDI_PIO_4BYTE, 0 },
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R2, UDI_PIO_4BYTE, (MII_MDO|MII_MDC) >> 16 },

	/* sync loop start */
	{ UDI_PIO_LABEL, 0, 1 },

	/* data out */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_4BYTE, SHRK_CSR9 },
	/* data out + clock */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R2, UDI_PIO_4BYTE, SHRK_CSR9 },

	/* sync loop end */
	{ UDI_PIO_ADD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, (udi_ubit16_t)-1 },
	{ UDI_PIO_CSKIP+UDI_PIO_R0, UDI_PIO_2BYTE, UDI_PIO_Z },
	{ UDI_PIO_BRANCH, 0, 1 },

	/* send command */
	/* R0 = current bit in 32-bit command */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_4BYTE, 0 },
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_4BYTE, 0x8000 },
	/* R1 still == MII data out */
	/* R2 = clock */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R2, UDI_PIO_4BYTE, 0 },
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R2, UDI_PIO_4BYTE, MII_MDC >> 16 },

	/* command loop start */
	{ UDI_PIO_LABEL, 0, 2 },

	/* set MII data out in R3 if current bit in command is set */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R3, UDI_PIO_2BYTE, 0 },      /* R3 = 0   */
	{ UDI_PIO_LOAD+UDI_PIO_DIRECT+UDI_PIO_R7,
				   UDI_PIO_4BYTE, UDI_PIO_R4 }, /* R4 = R7  */
	{ UDI_PIO_AND+UDI_PIO_R4, UDI_PIO_4BYTE, UDI_PIO_R0 },  /* R4 &= R0 */
	{ UDI_PIO_CSKIP+UDI_PIO_R4, UDI_PIO_4BYTE, UDI_PIO_Z }, /* R4 == 0? */
	{ UDI_PIO_LOAD+UDI_PIO_DIRECT+UDI_PIO_R1,
				   UDI_PIO_4BYTE, UDI_PIO_R3 }, /* R3 = R1  */

	/* output R3 */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R3, UDI_PIO_4BYTE, SHRK_CSR9 },

	/* OR clock with R3 and output */
	{ UDI_PIO_OR+UDI_PIO_R3, UDI_PIO_4BYTE, UDI_PIO_R2 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R3, UDI_PIO_4BYTE, SHRK_CSR9 },

	/* command loop end */
	{ UDI_PIO_SHIFT_RIGHT+UDI_PIO_R0, UDI_PIO_4BYTE, 1 },
	{ UDI_PIO_CSKIP+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_Z },
	{ UDI_PIO_BRANCH, 0, 2 },

	/* end of transaction list */
	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 }
};

/*
 * Take 21143 out of sleep mode
 *  Mapped to CFDD config register
 */
static udi_pio_trans_t shrk_nosnooze_pio[] = {
#if defined(SHRK_PIO_DEBUG)
	{UDI_PIO_DEBUG, UDI_PIO_1BYTE, UDI_PIO_TRACE_DEV3},
#endif

	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 0 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_4BYTE, 0 },
	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 }
};

/*
 * shrk_nic_reset_pio: reset NIC
 * input scratch layout:
 *	dword 0 - CSR0 value (bus mode)
 *	dword 1 - CSR6 value (operating mode)
 *	dword 2 - CSR12 value (general purpose register)
 *
 * returns 0 if h/w is present
 */
static udi_pio_trans_t shrk_nic_reset_pio[] = {
#if defined(SHRK_PIO_DEBUG)
	{UDI_PIO_DEBUG, UDI_PIO_1BYTE, UDI_PIO_TRACE_DEV3},
#endif

	/* R1 = CSR0 value */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 0 },
	{ UDI_PIO_LOAD+UDI_PIO_SCRATCH+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R1 },

	/* R2 = CSR6 value */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 4 },
	{ UDI_PIO_LOAD+UDI_PIO_SCRATCH+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R2 },

	/* R3 = gpreg value */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 8 },
	{ UDI_PIO_LOAD+UDI_PIO_SCRATCH+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R3 },

	/* disable tx/rx and set port select before reset */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R2, UDI_PIO_4BYTE, SHRK_CSR6 },

	/* R0 = reset NIC bus mode register.  Loaded as 2BYTE and internally
	 * extended to 4 bytes during the subsequent store. */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, BM_SWR },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_4BYTE, SHRK_CSR0 },

	/* set bus mode (CSR0) */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_4BYTE, SHRK_CSR0 },

	/* reset operating mode (CSR6) */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R2, UDI_PIO_4BYTE, SHRK_CSR6 },

	/* done if gpreg == 0 (non-Osicom cards) */
	{ UDI_PIO_CSKIP+UDI_PIO_R3, UDI_PIO_4BYTE, UDI_PIO_NZ },
	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 },

	/* Osicom GP register control */
	/* write GP mode */
	/* better to read gpcontrol from srom, parse infoleaf struct */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, GP_GPC|GP_OSI_GPCONTROL },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_4BYTE, SHRK_CSR12 },

	/* write GP value */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R3, UDI_PIO_4BYTE, SHRK_CSR12 },

	/* R0 = read GP register */
	{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_4BYTE, SHRK_CSR12 },

	/* return fail if read returns all 1's */ 
	/* R6 = 0xffffffff */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R6, UDI_PIO_4BYTE, 0xffff },
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R6, UDI_PIO_4BYTE, 0xffff },

	{ UDI_PIO_XOR+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R6 },
	{ UDI_PIO_CSKIP+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_NZ },
	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 1 },

	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 }
};

/*
 * shrk_read_gpreg_pio: read GP register
 *
 * Checks GP_OSI_SIGDET then GP_OSI_LINK10M_L, and returns first one found
 * Mapped at CSR12
 */
static udi_pio_trans_t shrk_read_gpreg_pio[] = {
#if defined(SHRK_PIO_DEBUG)
	{UDI_PIO_DEBUG, UDI_PIO_1BYTE, UDI_PIO_TRACE_DEV3},
#endif

	/* R1 = read GP register */
	{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_4BYTE, 0 },
	/* R2 = R1 - save for second test */
	{ UDI_PIO_STORE+UDI_PIO_DIRECT+UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_R1 },

	{ UDI_PIO_AND_IMM+UDI_PIO_R1, UDI_PIO_4BYTE, GP_OSI_SIGDET },
	{ UDI_PIO_CSKIP+UDI_PIO_R1, UDI_PIO_4BYTE, UDI_PIO_Z },
	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, GP_OSI_SIGDET },

	/* GP_OSI_LINK10M_L is reverse sense */
	{ UDI_PIO_AND_IMM+UDI_PIO_R2, UDI_PIO_4BYTE, GP_OSI_LINK10M_L },
	{ UDI_PIO_CSKIP+UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_NZ },
	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, GP_OSI_LINK10M_L },

	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 }
};

/*
 * shrk_abort_pio: pio abort sequence, MA resets hardware on region-kill
 */
static udi_pio_trans_t shrk_abort_pio[] = {
#if defined(SHRK_PIO_DEBUG)
	{UDI_PIO_DEBUG, UDI_PIO_1BYTE, UDI_PIO_TRACE_DEV3},
#endif

	/* R0 = reset NIC bus mode register.  Loaded as 2BYTE and internally
	 * zero extended to 32-bit word during subsequent store. */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, BM_SWR },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_4BYTE, SHRK_CSR0 },
	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 },
};

/*
 * shrk_tx_enable_pio: enable transmitter and interrupts
 * input scratch layout:
 *	dword 0 - txpaddr
 *	dword 1 - bits to OR into csr6
 */
static udi_pio_trans_t shrk_tx_enable_pio[] = {
#if defined(SHRK_PIO_DEBUG)
	{UDI_PIO_DEBUG, UDI_PIO_1BYTE, UDI_PIO_TRACE_DEV3},
#endif

	/* R1 = txpaddr offset */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_2BYTE, 0 },
	/* R2 = txpaddr (passed in scratch) */
	{ UDI_PIO_LOAD+UDI_PIO_SCRATCH+UDI_PIO_R1, UDI_PIO_4BYTE, UDI_PIO_R2 },

	/* R1 = csr6_bits offset */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_2BYTE, 4 },
	/* R3 = csr6_bits (passed in scratch) */
	{ UDI_PIO_LOAD+UDI_PIO_SCRATCH+UDI_PIO_R1, UDI_PIO_4BYTE, UDI_PIO_R3 },

	/* write txpaddr start to Transmit List Base Address register, CSR4 */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R2, UDI_PIO_4BYTE, SHRK_CSR4 },

	/* OR bits into Operating Mode register, CSR6 */
	/* R0 = read Operating Mode register */
	{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_4BYTE, SHRK_CSR6 },
	{ UDI_PIO_OR+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R3 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_4BYTE, SHRK_CSR6 },

	/* R1 = interrupt enable */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_4BYTE, INT_ENAB_LO16 },
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_4BYTE, INT_ENAB_HI16 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_4BYTE, SHRK_CSR7 },

	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 },
};

/*
 * shrk_tx_pio: issue transmit poll to NIC, mapped at CSR1 offset
 */
static udi_pio_trans_t shrk_tx_pio[] = {
#if defined(SHRK_PIO_DEBUG)
	{UDI_PIO_DEBUG, UDI_PIO_1BYTE, UDI_PIO_TRACE_DEV3},
#endif

	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 1 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_4BYTE, 0 },

	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 },
};

/*
 * shrk_rx_enable_pio: enable receiver
 * input scratch layout:
 *	dword 0 - rxpaddr
 */
static udi_pio_trans_t shrk_rx_enable_pio[] = {
#if defined(SHRK_PIO_DEBUG)
	{UDI_PIO_DEBUG, UDI_PIO_1BYTE, UDI_PIO_TRACE_DEV3},
#endif

	/* R1 = rxpaddr offset */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_2BYTE, 0 },
	/* R2 = rxpaddr (passed in scratch) */
	{ UDI_PIO_LOAD+UDI_PIO_SCRATCH+UDI_PIO_R1, UDI_PIO_4BYTE, UDI_PIO_R2 },
	/* write rxpaddr start to Receive List Base Address register, CSR3 */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R2, UDI_PIO_4BYTE, SHRK_CSR3 },

	/* write start receive/transmit to Operating Mode register, CSR6 */
	/* R0 = read Operating Mode register */
	{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_4BYTE, SHRK_CSR6 },
	/* set Start Receive bit in OMR value */
	{ UDI_PIO_OR_IMM+UDI_PIO_R0, UDI_PIO_4BYTE, OM_SR },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_4BYTE, SHRK_CSR6 },

	/* use R2 (paddr) to do receive poll */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R2, UDI_PIO_4BYTE, SHRK_CSR2 },

	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 },
};

/*
 * shrk_intr_preproc_pio: enable and process interrupts
 */
static udi_pio_trans_t shrk_intr_preproc_pio[] = {
	/*
	 * LABEL 0 entry point (implicit)
	 * interrupts disabled, dispatcher has min_event_pend cbs
	 */

#if defined(SHRK_PIO_DEBUG)
	{UDI_PIO_DEBUG, UDI_PIO_1BYTE, UDI_PIO_TRACE_DEV3},
#endif

	/* R1 = interrupt enable */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_4BYTE,
		IE_FBE|IE_RU|IE_RI|IE_TJ|IE_TI },
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_4BYTE, IE_NI>>16 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_4BYTE, SHRK_CSR7 },
	/* fall through */

	/*
	 * LABEL 1 entry point
	 * interrupts enabled, dispatcher has more than 1 cb
	 */
	{ UDI_PIO_LABEL, 0, 1 },

#if defined(SHRK_PIO_DEBUG)
	{UDI_PIO_DEBUG, UDI_PIO_1BYTE, UDI_PIO_TRACE_DEV3},
#endif

	/* R6 = 0 (entered trans list from label 0 or 1, dispatcher has cbs) */ 
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R6, UDI_PIO_2BYTE, 0 },

	{ UDI_PIO_LABEL, 0, 10 },

	/* read status (CSR5) into R0 */
	{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_4BYTE, SHRK_CSR5 },

	/* R1 = all status bits */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_4BYTE, STAT_CLR_LO16 },
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_4BYTE, STAT_CLR_HI16 },

	/* R0 (status) &= R1 (all bits) */
	{ UDI_PIO_AND+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R1 },

	/* not our interrupt if status bits are clear */
	{ UDI_PIO_CSKIP+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_NZ },
	{ UDI_PIO_BRANCH, 0, 30 },
	
	/* clear all status */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_4BYTE, SHRK_CSR5 },

	/* R1 = enabled interrupt mask */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_4BYTE, INT_ENAB_LO16 },
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_4BYTE, INT_ENAB_HI16 },

	/* R0 (status) &= R1 (enabled bits) */
	{ UDI_PIO_AND+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R1 },

	/* return status if we were called from label 0 or 1 (normal case) */
	{ UDI_PIO_CSKIP+UDI_PIO_R6, UDI_PIO_4BYTE, UDI_PIO_NZ },
	{ UDI_PIO_END, UDI_PIO_2BYTE, UDI_PIO_R0 },

	/* disable interrupts if we were called from label 2 */
	{ UDI_PIO_CSKIP+UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_NZ },
	{ UDI_PIO_BRANCH, 0, 20 },

	/* our interrupt, called from label 3: return NO_EVENT */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R6, UDI_PIO_2BYTE, UDI_INTR_NO_EVENT },
	{ UDI_PIO_BRANCH, 0, 40 },

	/*
	 * LABEL 2 entry point
	 * interrupts enabled, dispatcher has only 1 cb
	 */
	{ UDI_PIO_LABEL, 0, 2 },

#if defined(SHRK_PIO_DEBUG)
	{UDI_PIO_DEBUG, UDI_PIO_1BYTE, UDI_PIO_TRACE_DEV3},
#endif

	/* R6 = 1 (entered trans list from label 2 or 3) */ 
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R6, UDI_PIO_2BYTE, 1 },
	/* R7 = 0 (entered trans list from label 2 */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R7, UDI_PIO_2BYTE, 0 },

	/* process interrupt */
	{ UDI_PIO_BRANCH, 0, 10 },

	/* return here from common processing */
	{ UDI_PIO_LABEL, 0, 20 },
	/* disable all interrupts */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R4, UDI_PIO_2BYTE, 0 },
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R4, UDI_PIO_4BYTE, SHRK_CSR7 },

	/* return status */
	{ UDI_PIO_END, UDI_PIO_2BYTE, UDI_PIO_R0 },

	/*
	 * LABEL 3 entry point
	 * interrupt overflow condition - not our device;
	 * could verify our device didn't generate the interrupt
	 */
	{ UDI_PIO_LABEL, 0, 3 },

#if defined(SHRK_PIO_DEBUG)
	{UDI_PIO_DEBUG, UDI_PIO_1BYTE, UDI_PIO_TRACE_DEV3},
#endif
	/* R6 = 1 (entered trans list from label 2 or 3) */ 
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R6, UDI_PIO_2BYTE, 1 },
	/* R7 = 1 (entered trans list from label 3) */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R7, UDI_PIO_2BYTE, 0 },

	/* process interrupt; returns either UNCLAIMED or NO_EVENT */
	{ UDI_PIO_BRANCH, 0, 10 },


	/* UNCLAIMED (from any label) */
	{ UDI_PIO_LABEL, 0, 30 },
	/* R6 = interrupt unclaimed */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R6, UDI_PIO_2BYTE, UDI_INTR_UNCLAIMED },

	/* NO_EVENT returns here */
	{ UDI_PIO_LABEL, 0, 40 },
	/* R7 = 0 (scratch offset) */ 
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R7, UDI_PIO_2BYTE, 0 },
	/* store result in scratch */
	{ UDI_PIO_STORE+UDI_PIO_SCRATCH+UDI_PIO_R7, UDI_PIO_1BYTE, UDI_PIO_R6 },
	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 },
};

/* set bits in a csr
 * this trans list is mapped at csr to be written
 * scratch[0] contains 32 bit mask to "or" into mapped csr
 */
static udi_pio_trans_t shrk_or32_pio[] = {
#if defined(SHRK_PIO_DEBUG)
	{UDI_PIO_DEBUG, UDI_PIO_1BYTE, UDI_PIO_TRACE_DEV3},
#endif
	/* R0 = zero, bitmask memory offset  */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 0 },
	/* R1 = bitmask */
	{ UDI_PIO_LOAD+UDI_PIO_SCRATCH+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R1 },

	/* R0 = current value of mapped csr */
	{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_4BYTE, 0 },

	/* R0 |= R1 */
	{ UDI_PIO_OR+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R1 },

	/* write new value */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_4BYTE, 0 },

	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 },
};

/* clear bits in a csr
 * this trans list is mapped at csr to be written
 * scratch[0] contains 32 bit mask to "and" with mapped csr
 */
static udi_pio_trans_t shrk_and32_pio[] = {
#if defined(SHRK_PIO_DEBUG)
	{UDI_PIO_DEBUG, UDI_PIO_1BYTE, UDI_PIO_TRACE_DEV3},
#endif
	/* R0 = zero, bitmask memory offset  */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 0 },
	/* R1 = bitmask */
	{ UDI_PIO_LOAD+UDI_PIO_SCRATCH+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R1 },

	/* R0 = current value of mapped csr */
	{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_4BYTE, 0 },

	/* R0 &= R1 */
	{ UDI_PIO_AND+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R1 },

	/* write new value */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_4BYTE, 0 },

	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 },
};

#define NUM_EL(array) (sizeof(array) / sizeof(array[0]))

/*
 * args to udi_pio_map() for each trans list in primary region
 */
static shrk_pio_map_args_t shrk_pio_args[SHRK_PIO_COUNT] = {
	{ SHRK_PIO_WAKEUP,	/* index into pio handle array in rdata */
		UDI_PCI_CONFIG_SPACE, DEC2114_CFDD, 4,		/* PCI info */
		shrk_nosnooze_pio, NUM_EL(shrk_nosnooze_pio),	/* trans list */
		UDI_PIO_LITTLE_ENDIAN|UDI_PIO_STRICTORDER,	/* flags */
		0, 0 },				/* pace and serialization */
	{ SHRK_PIO_READ_SROM,
		UDI_PCI_BAR_1, 0, SHRK_PIO_LENGTH,
		shrk_srom_read_pio, NUM_EL(shrk_srom_read_pio),
		UDI_PIO_LITTLE_ENDIAN|UDI_PIO_STRICTORDER,
		1, 0 },
	{ SHRK_PIO_MDIO_READ,
		UDI_PCI_BAR_1, 0, SHRK_PIO_LENGTH,
		shrk_mii_read_pio, NUM_EL(shrk_mii_read_pio),
		UDI_PIO_LITTLE_ENDIAN|UDI_PIO_STRICTORDER,
		1, 0 },
	{ SHRK_PIO_MDIO_WRITE,
		UDI_PCI_BAR_1, 0, SHRK_PIO_LENGTH,
		shrk_mii_write_pio, NUM_EL(shrk_mii_write_pio),
		UDI_PIO_LITTLE_ENDIAN|UDI_PIO_STRICTORDER,
		1, 0 },
	{ SHRK_PIO_NIC_RESET,
		UDI_PCI_BAR_1, 0, SHRK_PIO_LENGTH,
		shrk_nic_reset_pio, NUM_EL(shrk_nic_reset_pio),
		UDI_PIO_LITTLE_ENDIAN|UDI_PIO_STRICTORDER,
		1, 0 },
	{ SHRK_PIO_READ_GPREG,
		UDI_PCI_BAR_1, SHRK_CSR12, SHRK_CSR15-SHRK_CSR12,
		shrk_read_gpreg_pio, NUM_EL(shrk_read_gpreg_pio),
		UDI_PIO_LITTLE_ENDIAN|UDI_PIO_STRICTORDER,
		1, 0 },
	{ SHRK_PIO_TX_ENABLE,
		UDI_PCI_BAR_1, 0, SHRK_PIO_LENGTH,
		shrk_tx_enable_pio, NUM_EL(shrk_tx_enable_pio),
		UDI_PIO_LITTLE_ENDIAN|UDI_PIO_STRICTORDER,
		0, 0 },
	{ SHRK_PIO_RX_ENABLE,
		UDI_PCI_BAR_1, 0, SHRK_PIO_LENGTH,
		shrk_rx_enable_pio, NUM_EL(shrk_rx_enable_pio),
		UDI_PIO_LITTLE_ENDIAN|UDI_PIO_STRICTORDER,
		0, 0 },
	{ SHRK_PIO_CSR6_OR32,
		UDI_PCI_BAR_1, SHRK_CSR6, SHRK_CSR7-SHRK_CSR6,
		shrk_or32_pio, NUM_EL(shrk_or32_pio),
		UDI_PIO_LITTLE_ENDIAN|UDI_PIO_STRICTORDER,
		0, 0 },
	{ SHRK_PIO_CSR6_AND32,
		UDI_PCI_BAR_1, SHRK_CSR6, SHRK_CSR7-SHRK_CSR6,
		shrk_and32_pio, NUM_EL(shrk_and32_pio),
		UDI_PIO_LITTLE_ENDIAN|UDI_PIO_STRICTORDER,
		0, 0 },
	{ SHRK_PIO_TX_POLL,
		UDI_PCI_BAR_1, SHRK_CSR1, SHRK_CSR2-SHRK_CSR1,
		shrk_tx_pio, NUM_EL(shrk_tx_pio),
		UDI_PIO_LITTLE_ENDIAN|UDI_PIO_STRICTORDER,
		0, 0 },
	{ SHRK_PIO_ABORT,
		UDI_PCI_BAR_1, SHRK_CSR0, SHRK_CSR1-SHRK_CSR0,
		shrk_abort_pio, NUM_EL(shrk_abort_pio),
		UDI_PIO_LITTLE_ENDIAN|UDI_PIO_STRICTORDER,
		0, 0 }
};

/*
 * possible values of link speed custom attribute in default locale
 *  these must match udiprops.txt
 */
static shrk_option_values_t shrk_link_speeds[] = {
	{ "Autonegotiate", SHRK_MEDIA_10 | SHRK_MEDIA_100 },
	{ "Force_10_Mbit/sec", SHRK_MEDIA_10 },
	{ "Force_100_Mbit/sec", SHRK_MEDIA_100 },
	{ 0, 0 }
};

/* possible values of duplex mode custom attribute in default locale */
static shrk_option_values_t shrk_duplex_modes[] = {
	{ "Autonegotiate", SHRK_MEDIA_HDX | SHRK_MEDIA_FDX },
	{ "Force_Half-Duplex", SHRK_MEDIA_HDX },
	{ "Force_Full-Duplex", SHRK_MEDIA_FDX },
	{ 0, 0 }
};

/* attribute names, types and defaults */
static shrk_instance_attr_name_t shrk_attr_names[SHRK_ATTR_COUNT] = {
	{ SHRK_ATTR_LINK_SPEED, "%link_speed", UDI_ATTR_STRING,
		shrk_link_speeds, SHRK_MEDIA_10 | SHRK_MEDIA_100 },
	{ SHRK_ATTR_DUPLEX_MODE, "%duplex_mode", UDI_ATTR_STRING,
		shrk_duplex_modes, SHRK_MEDIA_HDX | SHRK_MEDIA_FDX },
	{ SHRK_ATTR_RX_BUFFERS, "%rx_buffers", UDI_ATTR_UBIT32,
		0, SHRK_RX_BUFFERS_DEF },
	{ SHRK_ATTR_TX_BUFFERS, "%tx_buffers", UDI_ATTR_UBIT32,
		0, SHRK_TX_BUFFERS_DEF },
	{ SHRK_ATTR_PCI_VENDOR, "pci_vendor_id", UDI_ATTR_UBIT32, 0, 0 },
	{ SHRK_ATTR_PCI_DEVICE, "pci_device_id", UDI_ATTR_UBIT32, 0, 0 },
	{ SHRK_ATTR_PCI_REV, "pci_revision_id", UDI_ATTR_UBIT32, 0, 0 },
	{ SHRK_ATTR_PCI_SUBVEND, "pci_subsystem_vendor_id", UDI_ATTR_UBIT32,
								0, 0 },
	{ SHRK_ATTR_PCI_SUBSYS, "pci_subsystem_id", UDI_ATTR_UBIT32, 0, 0 }
};

/* link negotiation parameters and associated CSR6 and gpreg values */
/* the Osicom SROM says GP_OSI_FDX applies only to 10Mb */
static shrk_link_parms_t shrk_link_parms[] = {
	{ SHRK_MEDIA_100 | SHRK_MEDIA_FDX, CSR6_RESET_100 | OM_FD, GP_OSI_HDX },
	{ SHRK_MEDIA_100 | SHRK_MEDIA_HDX, CSR6_RESET_100, GP_OSI_HDX },
	{ SHRK_MEDIA_10 | SHRK_MEDIA_FDX, CSR6_RESET_10 | OM_FD, GP_OSI_FDX },
	{ SHRK_MEDIA_10 | SHRK_MEDIA_HDX, CSR6_RESET_10, GP_OSI_HDX }
};

/*
 *-----------------------------------------------------------------------------
 * DMA constraints lists
 */
/* shrk_constraints: used for rx buffers and descriptor lists */
static const udi_dma_constraints_attr_spec_t shrk_constraints[] = {
	/* descriptors and rx buffers are in 1 physically contiguous element */ 
	{ UDI_DMA_SCGTH_MAX_ELEMENTS, 1 },
	{ UDI_DMA_SCGTH_MAX_EL_PER_SEG, 1 },

	/* attributes the same after this */
	{ UDI_DMA_SCGTH_FORMAT, UDI_SCGTH_32|UDI_SCGTH_DRIVER_MAPPED },
	{ UDI_DMA_SCGTH_MAX_SEGMENTS, 1 },
	{ UDI_DMA_ADDRESSABLE_BITS, 32 },   /* 32 address lines */
	{ UDI_DMA_NO_PARTIAL, TRUE },       /* no partial dma mappings */
	{ UDI_DMA_ALIGNMENT_BITS, 5 },      /* longword (32-bit) aligned */
	{ UDI_DMA_ELEMENT_LENGTH_BITS, 16 }, /* Note: much larger then maxpdu */
	{ UDI_DMA_SEQUENTIAL, TRUE },        /* data access is sequential */
};

/* shrk_tx_constraints: tx buffer constraints */
static const udi_dma_constraints_attr_spec_t shrk_tx_constraints[] = {
	/* allow transmit buffers to span SHRK_TX_MAXELEM elements */ 
	{ UDI_DMA_SCGTH_MAX_ELEMENTS, SHRK_TX_MAXELEM },
	{ UDI_DMA_SCGTH_MAX_EL_PER_SEG, SHRK_TX_MAXELEM },

	/* attributes the same after this */
	{ UDI_DMA_SCGTH_FORMAT, UDI_SCGTH_32|UDI_SCGTH_DRIVER_MAPPED },
	{ UDI_DMA_SCGTH_MAX_SEGMENTS, 1 },
	{ UDI_DMA_ADDRESSABLE_BITS, 32 },   /* 32 address lines */
	{ UDI_DMA_NO_PARTIAL, TRUE },       /* no partial dma mappings */
	{ UDI_DMA_ALIGNMENT_BITS, 5 },      /* longword (32-bit) aligned */
	{ UDI_DMA_ELEMENT_LENGTH_BITS, 16 }, /* Note: much larger then maxpdu */
	{ UDI_DMA_SEQUENTIAL, TRUE },        /* data access is sequential */
};


/*
 *-----------------------------------------------------------------------------
 * Management Metalanguage Entrypoints
 *
 * The MA handles udi environment control, configuration, and instantiation
 */

/*
 * shrk_usage_ind
 *	- primary region
 *
 *	- first operation on a new region created by MA
 *	- handle tracing output levels and system resource indications
 */
void
shrk_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
	shrktx_rdata_t *dv = UDI_GCB(cb)->context;

	/* TODO2 handle tracing */
	DEBUG01(("shrk_usage_ind(cb %x reslev %x) context %x meta_idx %x\n",
		cb, resource_level, UDI_GCB(cb)->context, cb->meta_idx));

	/* save tracemask and resource levels */
	dv->ml_trace_mask[cb->meta_idx] = cb->trace_mask;
	dv->reslev = resource_level;

	if (!(dv->flags & SHRK_INIT)) {
		dv->flags = SHRK_INIT;
		/* first time through initialize region-specific data */
		dv->nic_state = UNBOUND;

		udi_dma_limits(&dv->dma_limits);

		/* calculate csr0 value based on cache line size */
		dv->csr0 = SHRK_DESC_SKIP_LEN << BM_DSL_BIT_POS;
		dv->csr0 &= BM_DSL_MASK;

		switch (dv->dma_limits.cache_line_size / 4) {
		case 8:
			dv->csr0 |= BM_RLE | BM_RME | BM_CACHE_8;
			break;
		case 16:
			dv->csr0 |= BM_RLE | BM_RME | BM_CACHE_16;
			break;
		case 32:
			dv->csr0 |= BM_RLE | BM_RME | BM_CACHE_32;
			break;
		default:
			/* no cache alignment */ 
			break;
		}
		/* this eventually does the udi_usage_res() */
		udi_cb_alloc(shrk_log_cb, UDI_GCB(cb),
				SHRK_GCB_CB_IDX, UDI_NULL_CHANNEL);
	} else {
		/* TODO3: log */
		DEBUG02(("shrk_usage_ind(cb %x reslev %d): multi calls\n",
			cb, resource_level));
		udi_usage_res(cb);
	}
}

/*
 * shrk_enumerate_req
 *	- primary region
 *
 * request information regarding a child instance
 * see enumeration attributes in the UDI Network Driver spec
 */
void
shrk_enumerate_req(udi_enumerate_cb_t *cb, udi_ubit8_t level)
{
	shrktx_rdata_t *dv = UDI_GCB(cb)->context;
	udi_ubit8_t result;
	udi_instance_attr_list_t *a;

	DEBUG01(("shrk_enumerate_req(cb %x level %x) dv %x\n", cb, level, dv));
	/*
	 * we've requested SHRK_ATTR_LEN in udi_primary_init
	 * so there should be enough room for all six attributes
	 */

	switch (level) {
	case UDI_ENUMERATE_START:
	case UDI_ENUMERATE_START_RESCAN:
		a = cb->attr_list;

		udi_strcpy(a->attr_name, "if_num");
		UDI_ATTR32_SET(a->attr_value, 0);
		a->attr_length = sizeof(udi_ubit32_t);
		a->attr_type = UDI_ATTR_UBIT32;
		a++;

		udi_strcpy(a->attr_name, "if_media");
		if (shrk_link_parms[dv->link_index].flags & SHRK_MEDIA_10) {
			udi_strcpy((char *)a->attr_value, "eth");
		} else {
			udi_strcpy((char *)a->attr_value, "fe");
		}
		a->attr_length = udi_strlen((char *)a->attr_value) + 1;
		a->attr_type = UDI_ATTR_STRING;
		a++;

		udi_strcpy(a->attr_name, "identifier");
		shrk_addrstr(dv->hwaddr, (char *)a->attr_value);
		a->attr_length = udi_strlen((char *)a->attr_value) + 1;
		a->attr_type = UDI_ATTR_STRING;
		a++;

		udi_strcpy(a->attr_name, "address_locator");
		a->attr_value[0] = '0';
		a->attr_value[1] = 0;
		a->attr_length = udi_strlen((char *)a->attr_value) + 1;
		a->attr_type = UDI_ATTR_STRING;
		a++;

		udi_strcpy(a->attr_name, "physical_locator");
		a->attr_value[0] = '0';
		a->attr_value[1] = 0;
		a->attr_length = udi_strlen((char *)a->attr_value) + 1;
		a->attr_type = UDI_ATTR_STRING;
		a++;

		cb->attr_valid_length = SHRK_ATTR_LEN;
		result = UDI_ENUMERATE_OK;
		break;

	case UDI_ENUMERATE_NEXT:
		result = UDI_ENUMERATE_DONE;
		break;
	case UDI_ENUMERATE_NEW:
		/* We don't support new children; just fail it now. */
		result = UDI_ENUMERATE_FAILED;
		break;
	case UDI_ENUMERATE_DIRECTED:
		/* only have one child (NSR); directed enumeration not needed */
		/* could make driver use data from enumeration list (macaddr) */
		result = UDI_ENUMERATE_FAILED;
		break;
	case UDI_ENUMERATE_RELEASE:
		/* TODO2 release resources associated with cb->child_ID */
		result = UDI_ENUMERATE_RELEASED;
		break;
	default:
		result = UDI_ENUMERATE_FAILED;
	}

	udi_enumerate_ack(cb, result, SHRK_ND_CTRL_IDX);
}

/*
 * shrk_addrstr
 *	- primary region
 *
 * generate a string version of the hw mac address
 * ie. turn 0x02608cf0123a into "02608cf0123a"
 */
#define	EASTRINGSIZE	12		/* size of ethernet address string */ 
void
shrk_addrstr(shrkaddr_t ea, char *buf)
{
	udi_ubit32_t i;
	static const char s[] = "0123456789abcdef";
	
	for (i = 0; i < SHRK_MACADDRSIZE; ++i) {
		buf[i*2] = s[(ea[i] >> 4) & 0xf];
		buf[i*2 + 1] = s[ea[i] & 0xf];
	}
	buf[EASTRINGSIZE] = '\0';
}

/*
 * shrk_devmgmt_req
 *	- primary region
 *
 * handle device management operations
 */
void
shrk_devmgmt_req(udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, udi_ubit8_t pid)
{
	shrktx_rdata_t *dv = UDI_GCB(cb)->context;
	udi_ubit8_t flags = 0;

	DEBUG01(("shrk_devmgmt_req(cb %x mgmt_op %x) dv %x\n", cb, mgmt_op, dv));

	switch (mgmt_op) {
	case UDI_DMGMT_PREPARE_TO_SUSPEND:
		DEBUG01(("shrk_devmgmt_req(PREPARE_TO_SUSPEND)\n"));
		flags = UDI_DMGMT_NONTRANSPARENT;
		break;
	case UDI_DMGMT_SUSPEND:
		DEBUG01(("shrk_devmgmt_req(SUSPEND)\n"));
		flags = UDI_DMGMT_NONTRANSPARENT;
		dv->flags |= SHRK_SUSPENDED;
		shrk_clean_ring(dv);
		dv->cdi = dv->pdi = 0;

		/* disable HW activity */
		if (dv->pio_handles[SHRK_PIO_CSR6_AND32]
						!= UDI_NULL_PIO_HANDLE) {
		    *((udi_ubit32_t *)UDI_GCB(cb)->scratch) = ~(OM_SR|OM_ST);
		    udi_pio_trans(shrk_disabled_pio, UDI_GCB(cb),
				dv->pio_handles[SHRK_PIO_CSR6_AND32],
							0, NULL, NULL);
		}
		break;
	case UDI_DMGMT_SHUTDOWN:
		DEBUG01(("shrk_devmgmt_req(SHUTDOWN)\n"));
		dv->flags |= SHRK_REMOVED;
		shrk_clean_ring(dv);
		dv->cdi = dv->pdi = 0;

		/* disable HW activity */
		if (dv->pio_handles[SHRK_PIO_CSR6_AND32]
						!= UDI_NULL_PIO_HANDLE) {
		    *((udi_ubit32_t *)UDI_GCB(cb)->scratch) = ~(OM_SR|OM_ST);
		    udi_pio_trans(shrk_disabled_pio, UDI_GCB(cb),
				dv->pio_handles[SHRK_PIO_CSR6_AND32],
							0, NULL, NULL);
		}
		break;
	case UDI_DMGMT_PARENT_SUSPENDED:
		DEBUG01(("shrk_devmgmt_req(PARENT_SUSPENDED)\n"));
		dv->flags |= SHRK_SUSPENDED;
		break;
	case UDI_DMGMT_RESUME:
		DEBUG01(("shrk_devmgmt_req(RESUME)\n"));
		dv->flags &= ~SHRK_SUSPENDED;
		/* TODO3 restart transmitter, receiver from current position */
		break;
	case UDI_DMGMT_UNBIND:
		DEBUG01(("shrk_devmgmt_req(UNBIND)\n"));
		/*
		 * driver unbinds from bus parent and detaches interrupts
		 * in the secondary region when the gio channel gets closed
		 */
		break;
	}

	udi_devmgmt_ack(cb, flags, UDI_OK);
}

/*
 * shrk_final_cleanup_req
 *	- primary region
 *
 * release resources prior to instance unload
 */
void
shrk_final_cleanup_req(udi_mgmt_cb_t *cb)
{
	shrktx_rdata_t *dv = UDI_GCB(cb)->context;
	udi_ubit16_t i;

	DEBUG01(("shrk_final_cleanup_req(cb %x) dv %x\n", cb, dv));

	/* TODO2 release all resources and region instance context */

	for (i = 0; i < SHRK_PIO_COUNT; i++) {
		if (dv->pio_handles[i] != UDI_NULL_PIO_HANDLE)
			udi_pio_unmap(dv->pio_handles[i]);
	}

#if 0 /* still having problems will un-unmaped buffers */
	for (i = 0; i < dv->max_desc - 1; i++) {
DEBUG15(("shrk_final_cleanup_req: freeing sdmah %d\n", i));
		udi_dma_free(dv->tx_sdmah[i].dmah);
	}
DEBUG15(("shrk_final_cleanup_req: done freeing sdmah\n"));
#endif
	udi_mem_free(dv->tx_sdmah);

	udi_dma_free(dv->txd_dmah);
	udi_dma_free(dv->setup_dmah);

	udi_dma_constraints_free(dv->tx_cnst);
	udi_dma_constraints_free(dv->txd_cnst);

	udi_mem_free(dv->srom_contents);

	if (dv->timer_cb) {
		udi_timer_cancel(dv->timer_cb);
	}
	if (dv->flags & SHRK_SETUP) {
		udi_timer_cancel(dv->setup_cb);
	}

	udi_final_cleanup_ack(cb);
}

/*
 *-----------------------------------------------------------------------------
 * Initialization happening during management meta usage indication
 */

/*
 * log cb allocated
 */
void
shrk_log_cb(udi_cb_t *cb, udi_cb_t *new_cb)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG01(("shrk_log_cb(cb %x new_cb %x) dv %x\n", cb, new_cb, dv));

	dv->log_cb = new_cb;

	/* initial call of recursive callback with bogus attr_type */
	shrk_instance_attr_get(cb, (udi_instance_attr_type_t) -1, 0);
}

/*
 * recursive callback for getting instance attributes
 */
void
shrk_instance_attr_get(udi_cb_t *cb, udi_instance_attr_type_t type,
							udi_size_t length)
{
	shrktx_rdata_t *dv = cb->context;
	shrk_option_values_t *value;

	DEBUG01(("shrk_instance_attr(cb %x type %d len %d) dv %x index %d\n",
					cb, type, length, dv, dv->index));

	if (type == (udi_instance_attr_type_t) -1) {
		dv->index = -1;
	} else {
		if (type != UDI_ATTR_NONE) {
			udi_assert(shrk_attr_names[dv->index].attr_type == type);
		}
		/* set default */
		dv->attr_values[dv->index] = shrk_attr_names[dv->index].defval;
		switch(type) {
		case UDI_ATTR_STRING:
			/* look up value in translation table */
			udi_assert(shrk_attr_names[dv->index].values);
			value = shrk_attr_names[dv->index].values;
			while (value->name != 0) {
				if (udi_strcmp(value->name, 
				    (const char *)dv->attr_scratch) == 0) {
					dv->attr_values[dv->index] =
								value->code;
					break;
				}
				value++;
			}
			if (value->name == 0) {
				/* TODO3: log */
				DEBUG02(("shrk_instance_attr: value %s for %s not recognised\n",
					dv->attr_scratch,
					shrk_attr_names[dv->index].attr_name));
			} else {
				DEBUG07(("shrk_instance_attr: %s = \"%s\" (0x%x)\n",
					shrk_attr_names[dv->index].attr_name, 
					dv->attr_scratch, value->code));
			}
			break;
		case UDI_ATTR_UBIT32:
			dv->attr_values[dv->index]
					= UDI_ATTR32_GET(dv->attr_scratch);
			DEBUG07(("shrk_instance_attr: %s = %d.\n",
				shrk_attr_names[dv->index].attr_name, 
				dv->attr_values[dv->index]));
			break;
		case UDI_ATTR_NONE:
			/* TODO3: log */
			DEBUG02(("shrk_instance_attr: no value for %s\n",
				shrk_attr_names[dv->index].attr_name));
			break;
		}
	}

	if (++dv->index == SHRK_ATTR_COUNT) {
		udi_usage_res(UDI_MCB(cb, udi_usage_cb_t));
	} else {
		udi_instance_attr_get(shrk_instance_attr_get, cb,
			shrk_attr_names[dv->index].attr_name, 0,
			(void *)dv->attr_scratch, sizeof(dv->attr_scratch));
	}
}

/*
 *-----------------------------------------------------------------------------
 * GIO Metalanguage Entrypoints (used for inter-region communication)
 * Primary region
 */

/*
 * shrk_gio_bind_req
 *	- gio provider, primary region
 *
 * request binding to gio provider
 * client calls this routine from secondary region after allocating cb
 */
void
shrk_gio_bind_req(udi_gio_bind_cb_t *cb)
{
	shrktx_rdata_t *dv = UDI_GCB(cb)->context;
	udi_channel_t chan = UDI_GCB(cb)->channel;
	udi_ubit32_t i;

	dv->gio_chan = chan;
	dv->max_desc = SHRK_RING_SIZE;

	/* set the hiwater to half the ring or buffers, whichever is less */
	dv->tx_cleanup = dv->attr_values[SHRK_ATTR_TX_BUFFERS];
	if (dv->tx_cleanup > SHRK_RING_SIZE)
		dv->tx_cleanup = SHRK_RING_SIZE;
	dv->tx_cleanup >>= 1;

	/* calculate mask to test cdi to see when to request a cleanup int */
	/* mask is highest power of 2 less than hiwater, minus 1 */
	/* cleanup interrupts are only requested when ring is over hiwater */
	dv->tx_freemask = 2;		/* mask has to be at least 1 */
	for (i=0; i < 32; i++) {
		dv->tx_freemask <<= 1;
		if (dv->tx_freemask >= dv->tx_cleanup)
			break;
	}
	udi_assert(i != 32);
	dv->tx_freemask = (dv->tx_freemask >> 1) - 1;

	DEBUG01(("shrk_gio_bind_req(cb %x) dv %x chan %x cleanup %d mask %x\n",
		cb, dv, chan, dv->tx_cleanup, dv->tx_freemask));
	
	udi_cb_alloc(shrk_gio_event_cb, UDI_GCB(cb), SHRK_GIO_EVENT_CB_IDX,
		dv->gio_chan);
}

/*
 * shrk_gio_event_cb
 *	- primary region
 *
 * gio event cb allocated callback
 * save event request cb, then issue bind ack
 */
void
shrk_gio_event_cb(udi_cb_t *cb, udi_cb_t *new_cb)
{
	shrktx_rdata_t *dv = cb->context;
	udi_gio_event_cb_t *event_cb = UDI_MCB(new_cb, udi_gio_event_cb_t);

	DEBUG01(("shrk_gio_event_cb(cb %x new_cb %x) dv %x\n", cb, new_cb, dv));
	
	event_cb->event_code = SHRK_GIO_GIO_BIND;

	udi_gio_event_ind(event_cb);
	dv->gio_event_cb = NULL;

	udi_gio_bind_ack(UDI_MCB(cb, udi_gio_bind_cb_t), 0, 0, UDI_OK);
}

/*
 * shrk_gio_xfer_req
 *	- gio provider, primary region
 *
 * gio transfer request
 */
void
shrk_gio_xfer_req(udi_gio_xfer_cb_t *cb)
{
	shrktx_rdata_t *dv = UDI_GCB(cb)->context;
	shrk_constraints_set_scratch_t *scratch = UDI_GCB(cb)->scratch;
	shrk_gio_xfer_params_t *params = cb->tr_params;

	DEBUG01(("shrk_gio_xfer_req(cb %x) dv %x op %x\n", cb, dv, cb->op));

	switch (cb->op) {
	case SHRK_GIO_BUS_BIND:
		DEBUG03(("shrk_gio_xfer_req: GIO_BUS_BIND\n"));
		/* receive descriptor memory from secondary */
		dv->rxpaddr = params->rxpaddr;
		/* bus constraints from secondary region */
		scratch->parent_constraints = params->parent_constraints;
 		/* make a copy; we need one for tx and one for txd */
 		udi_dma_constraints_attr_set(shrk_tx_constraints_set,
  			UDI_GCB(cb), params->parent_constraints,
 			shrk_tx_constraints,
 			sizeof(shrk_tx_constraints) /
 				sizeof(udi_dma_constraints_attr_spec_t),
 			UDI_DMA_CONSTRAINTS_COPY);
		break;
	case SHRK_GIO_TX_COMPLETE:
		DEBUG15(("shrk_gio_xfer_req: GIO_TX_COMPLETE\n"));
		shrk_txcomplete(dv);
		udi_gio_xfer_ack(cb);
		break;
	default:
		udi_assert(0);
	}
}

/*
 * shrk_gio_event_res
 *	- primary region
 *
 * gio event response (from gio client in secondary region)
 */
void
shrk_gio_event_res(udi_gio_event_cb_t *cb)
{
	shrktx_rdata_t *dv = UDI_GCB(cb)->context;
	udi_nic_info_cb_t *icb;
	shrk_gio_event_params_t *evparams;

	DEBUG01(("shrk_gio_event_res(cb %x) dv %x nic_state %x\n",
		cb, dv, dv->nic_state));

	switch (cb->event_code) {
	case SHRK_GIO_GIO_BIND:
		DEBUG03(("shrk_gio_event_res: GIO_GIO_BIND\n"));
		break;
	case SHRK_GIO_NIC_BIND:
		DEBUG03(("shrk_gio_event_res: GIO_NIC_BIND\n"));
		/* all NSR channels spawned/anchored, send bind_ack to NSR */
		shrk_nd_bind_ack(dv, UDI_OK);
		break;
	case SHRK_GIO_NIC_ENABLE:
		DEBUG03(("shrk_gio_event_res: GIO_NIC_ENABLE\n"));
		if (dv->nic_state == ACTIVE) {
			cb->event_code = SHRK_GIO_NIC_LINK_UP;
			udi_gio_event_ind(cb);
			return;
		}
		break;
	case SHRK_GIO_NIC_LINK_UP:
		DEBUG03(("shrk_gio_event_res: GIO_NIC_LINK_UP\n"));
		dv->nic_state = ACTIVE;
		*((udi_ubit32_t *)UDI_GCB(cb)->scratch) = dv->rxpaddr;
		udi_pio_trans(shrk_rxenab_pio, UDI_GCB(cb),
			dv->pio_handles[SHRK_PIO_RX_ENABLE], 0, NULL, NULL);
		return;
	case SHRK_GIO_NIC_LINK_DOWN:
		DEBUG03(("shrk_gio_event_res: GIO_NIC_LINK_DOWN\n"));
		break;
	case SHRK_GIO_NIC_INFO:
		DEBUG03(("shrk_gio_event_res SHRK_GIO_NIC_INFO\n"));
		icb = UDI_MCB(UDI_GCB(cb)->initiator_context, udi_nic_info_cb_t);
		UDI_GCB(cb)->initiator_context = NULL;
		udi_assert(icb);

		evparams = (shrk_gio_event_params_t *)cb->event_params;

		icb->tx_packets = dv->stats.tx_packets;
		icb->tx_underrun = dv->stats.tx_underrun;
		icb->tx_errors = dv->stats.tx_errors;
		icb->collisions = dv->stats.collisions;
		icb->tx_discards = dv->stats.tx_discards;
		icb->rx_packets = evparams->rx_packets;
		icb->rx_errors = evparams->rx_errors;
		icb->rx_discards = evparams->rx_discards;
		icb->rx_overrun = evparams->rx_overrun;

		/* TODO3 read CSR8 to obtain rx_discards and rx_overrun */ 
		udi_nsr_info_ack(icb);

		if (dv->reset_stats) {
			DEBUG03(("shrk_gio_event_res INFO reset_stats\n"));
			udi_memset(&dv->stats, 0, sizeof(udi_nic_info_cb_t));
		}

		break;
	case SHRK_GIO_NIC_DISABLE:
		DEBUG03(("shrk_gio_event_res SHRK_GIO_NIC_DISABLE\n"));
		break;
	case SHRK_GIO_NIC_UNBIND:
		DEBUG03(("shrk_gio_event_res SHRK_GIO_NIC_UNBIND\n"));
		udi_nsr_unbind_ack(UDI_GCB(cb)->initiator_context, UDI_OK);
		UDI_GCB(cb)->initiator_context = NULL;
		dv->nic_state = UNBOUND;
		break;
	}

	dv->gio_event_cb = cb;
}

/*
 * shrk_rxenab_pio
 *	- primary region
 *
 * NIC receive enabled pio transaction completed
 * called after secondary responds to LINK_UP event
 * -- send STATUS_IND
 */
void
shrk_rxenab_pio(udi_cb_t *cb, udi_buf_t *buf, udi_status_t st, udi_ubit16_t res)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG01(("shrk_rxenab_pio(cb %x buf %x st %x res %x) dv %x\n",
		cb, buf, st, res, dv));

	udi_cb_alloc(shrk_status_ind_cb, cb,
		SHRK_NIC_STATUS_CB_IDX, dv->ctl_chan);

	dv->gio_event_cb = UDI_MCB(cb, udi_gio_event_cb_t);
}

/*
 * shrk_status_ind_cb
 *
 * allocated cb for status indication; send it up
 */
void
shrk_status_ind_cb(udi_cb_t *cb, udi_cb_t *new_cb)
{
	shrktx_rdata_t *dv = cb->context;
	udi_nic_status_cb_t *stat_cb = UDI_MCB(new_cb, udi_nic_status_cb_t);

	DEBUG01(("shrk_status_ind_cb(cb %x, new_cb %x)\n", cb, new_cb));

	switch (dv->nic_state) {
	case ACTIVE:
		shrk_log(dv, UDI_OK, 301, "shrk_link_status",
			(dv->flags & SHRK_SPEED_100M) ? 100 : 10,
			(dv->flags & SHRK_DUPLEX_FULL) ? "FDX" : "HDX");
		stat_cb->event = UDI_NIC_LINK_UP;
		break;
	default:
		shrk_log(dv, UDI_STAT_NOT_RESPONDING, 300, "shrk_link_status");
		stat_cb->event = UDI_NIC_LINK_DOWN;
		break;
	}

	udi_nsr_status_ind(stat_cb);
}

/*
 * shrk_gio_unbind_req
 *	- gio provider, primary region
 *
 * request to unbind from gio provider
 * client calls this routine from secondary region to request unbind
 */
void
shrk_gio_unbind_req(udi_gio_bind_cb_t *cb)
{
	shrktx_rdata_t *dv = UDI_GCB(cb)->context;
	
	DEBUG01(("shrk_gio_unbind_req(cb %x) dv %x\n", cb, dv));

	dv->gio_chan = UDI_NULL_CHANNEL;
	udi_gio_unbind_ack(cb);
}

/*
 * shrk_gio_channel_event_ind
 *	- primary region, gio provider
 *
 * handle gio channel events
 */
void
shrk_gio_channel_event_ind(udi_channel_event_cb_t *cb)
{

	DEBUG01(("shrk_gio_channel_event_ind(cb %x) dv %x chan %x event %x\n",
		cb, UDI_GCB(cb)->context, UDI_GCB(cb)->channel, cb->event));

	switch (cb->event) {
	case UDI_CHANNEL_CLOSED:     /* won't happen */
	case UDI_CHANNEL_BOUND:      /* won't happen */
	default:
		UDI_GCB(cb)->initiator_context = cb;
		udi_log_write(shrk_log_channel, UDI_GCB(cb), UDI_TREVENT_LOG,
			UDI_LOG_ERROR, SHRK_NIC_META, UDI_STAT_NOT_UNDERSTOOD,
			100, "shrk_gio_channel_event_ind", cb->event);
		break;
	}
}


/*
 *-----------------------------------------------------------------------------
 * Initialization happening in primary region during bus_bind_ack()
 */

/*
 * shrk_tx_constraints_set
 *	- primary region
 *
 * constraints handle set callback
 * allocate tx descriptor constraints
 */
void
shrk_tx_constraints_set(udi_cb_t *cb, udi_dma_constraints_t cnst, udi_status_t st)
{
	shrktx_rdata_t *dv = cb->context;
	shrk_constraints_set_scratch_t *scratch = cb->scratch;

	DEBUG01(("shrk_tx_constraints_set(cb %x cnst %x st %x) dv %x\n",
		cb, cnst, st, dv));

	if (st != UDI_OK) {
		udi_dma_constraints_free(cnst);
		udi_gio_xfer_nak(UDI_MCB(cb, udi_gio_xfer_cb_t),
			UDI_STAT_RESOURCE_UNAVAIL);
		return;
	}

 	dv->tx_cnst = cnst;	/* save tx data buffer constraints */
  
  	udi_dma_constraints_attr_set(shrk_txd_constraints_set,
  		cb, scratch->parent_constraints, shrk_constraints,
 		sizeof(shrk_constraints) /
 			sizeof(udi_dma_constraints_attr_spec_t), 0);
}

/*
 * shrk_txd_constraints_set
 *	- primary region
 *
 * callback for tx buffer dma constraints handle set
 */
void
shrk_txd_constraints_set(udi_cb_t *cb, udi_dma_constraints_t cnst, udi_status_t st)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG01(("shrk_txd_constraints_set(cb %x cnst %x) dv %x\n",
		cb, cnst, dv));

	dv->txd_cnst = cnst; /* save tx descriptor constraints */

	udi_mem_alloc(shrk_dma_alloc, cb,
			(dv->max_desc - 1) * sizeof(shrk_dma_element_t), 0);
}

/*
 * shrk_dma_alloc
 *
 * dma handle storage allocated
 * allocate dma handles
 */
void
shrk_dma_alloc(udi_cb_t *cb, void *new_mem)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG01(("shrk_dma_alloc(cb %x new_mem %x)\n", cb, new_mem));

	dv->tx_sdmah = dv->next_sdmah = (shrk_dma_element_t *)new_mem;

	dv->index = 0;
	/* get tx data dma handles using tx buffer constraints */
	udi_dma_prepare(shrk_tx_dmah, cb, dv->tx_cnst, UDI_DMA_OUT);
}

/*
 * shrk_tx_dmah
 *	- primary region
 *
 * callback for udi_dma_prepare which allocates dma handles for transmit buffers
 */
void
shrk_tx_dmah(udi_cb_t *cb, udi_dma_handle_t dmah)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG01(("shrk_tx_dmah(cb %x dmah %x) index %d\n",
		cb, dmah, dv->index));

	dv->tx_sdmah[dv->index].dmah = dmah;

	if (dv->index < dv->max_desc - 2) {	/* one descriptor is unused */
		dv->tx_sdmah[dv->index].next = &(dv->tx_sdmah[dv->index + 1]);
		dv->index++;
		udi_dma_prepare(shrk_tx_dmah, cb, dv->tx_cnst, UDI_DMA_OUT);
	} else {
		dv->tx_sdmah[dv->index].next = NULL;
		/* allocate memory for transmit descriptors */
		udi_dma_mem_alloc(shrk_txdesc_alloc, cb,
			dv->txd_cnst, UDI_DMA_OUT|UDI_DMA_LITTLE_ENDIAN,
			dv->max_desc, sizeof(txd_t), 0);
	}
}

/*
 * shrk_txdesc_alloc
 *	- primary region
 *
 * dma'able memory for nic transmit descriptors allocated callback
 */
void
shrk_txdesc_alloc(udi_cb_t *cb, udi_dma_handle_t dmah, void *mem,
	udi_size_t actual_gap, udi_boolean_t single_element,
	udi_scgth_t *scgth, udi_boolean_t must_swap)
{
	shrktx_rdata_t *dv = cb->context;


	DEBUG01(("shrk_txdesc_alloc dv %x txd_cnst %x\n", dv, dv->txd_cnst));

	udi_assert(scgth->scgth_num_elements == 1);
	udi_assert(scgth->scgth_format ==
		(UDI_SCGTH_32 | UDI_SCGTH_DRIVER_MAPPED));

	dv->swap = must_swap;
	dv->txpaddr = SHRK_SWAP_SCGTH(scgth,
		scgth->scgth_elements.el32p[0].block_busaddr);
	dv->desc = mem;
	dv->txd_dmah = dmah;
	/* mark last descriptor end of ring - must be after dv->swap is set */
	dv->desc[dv->max_desc-1].cntrl = SHRK_SWAP_DESC(dv, TX_TER); 

	/* allocate memory for setup frame */
	udi_dma_mem_alloc(shrk_setup_alloc, cb,
		dv->txd_cnst, UDI_DMA_OUT|UDI_DMA_LITTLE_ENDIAN,
		1, SHRK_SET_SZ+sizeof(udi_ubit32_t), 0);
}

/*
 * shrk_setup_alloc
 *	- primary region
 *
 * dma'able memory for nic setup frame allocated callback
 */
void
shrk_setup_alloc(udi_cb_t *cb, udi_dma_handle_t dmah, void *mem,
	udi_size_t actual_gap, udi_boolean_t single_element,
	udi_scgth_t *scgth, udi_boolean_t must_swap)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG01(("shrk_setup_alloc(cb %x dmah %x mem %x s/g %x m/s %x) dv %x\n",
		cb, dmah, mem, scgth, must_swap, dv));

	udi_assert(scgth->scgth_num_elements == 1);
	udi_assert(scgth->scgth_format ==
		(UDI_SCGTH_32|/*UDI_SCGTH_DMA_MAPPED|*/UDI_SCGTH_DRIVER_MAPPED));

	dv->setup_paddr = SHRK_SWAP_SCGTH(scgth,
		scgth->scgth_elements.el32p[0].block_busaddr);
	dv->setup_mem = mem;
	dv->setup_dmah = dmah;
	dv->setup_swap = must_swap;

	shrk_pio_map(cb, UDI_NULL_PIO_HANDLE);
}

/*
 * recursive routine to map primary region pio trans lists
 */
void
shrk_pio_map(udi_cb_t *cb, udi_pio_handle_t pio_handle)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG01(("shrk_pio_map(cb %x pio %x) dv %x index %d\n",
					cb, pio_handle, dv, dv->index));

	if (pio_handle == UDI_NULL_PIO_HANDLE) {
		dv->index = -1;
	} else {
		dv->pio_handles[shrk_pio_args[dv->index].rdata_idx]
								= pio_handle;
	}

	if (++dv->index == SHRK_PIO_COUNT) {
		/* register abort pio with env */
		udi_pio_abort_sequence(dv->pio_handles[SHRK_PIO_ABORT], 0);
		/* env unmaps abort pio */
		dv->pio_handles[SHRK_PIO_ABORT] = UDI_NULL_PIO_HANDLE;

		if (dv->attr_values[SHRK_ATTR_PCI_DEVICE]
						== SHRK_DEV_DEC_21143) {
		    /* clear 21143 sleep mode */
		    udi_pio_trans(shrk_wakeup_done, cb,
			dv->pio_handles[SHRK_PIO_WAKEUP], 0, NULL, NULL);
		} else {
		    /* allocate memory for contents of SROM */
		    udi_mem_alloc(shrk_read_srom, cb, SHRK_SROMSIZE,
							UDI_MEM_MOVABLE);
		}
	} else {
		/* map next trans list */
		shrk_pio_map_args_t *args = &shrk_pio_args[dv->index];
		udi_pio_map(shrk_pio_map, cb, args->regset_idx,
			args->base_offset, args->length, args->trans_list,
			args->list_length, args->pio_attributes, args->pace,
			args->serialization_domain);
	}
}

/*
 * 21143 wakeup done
 */
void
shrk_wakeup_done(udi_cb_t *cb, udi_buf_t *buf, udi_status_t st, udi_ubit16_t res)
{
	DEBUG01(("shrk_wakeup_done(cb %x buf %x st %x res %x)\n",
							cb, buf, st, res));

	/* allocate memory for contents of SROM */
	udi_mem_alloc(shrk_read_srom, cb, SHRK_SROMSIZE, UDI_MEM_MOVABLE);
}

/*
 * shrk_read_srom
 *	- primary region
 *
 * store address of srom contents memory
 * read SROM
 */
void
shrk_read_srom(udi_cb_t *cb, void *new_mem)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG01(("shrk_read_srom(cb %x new_mem %x) dv %x\n",
		cb, new_mem, dv));

	/* wakeup pio handle not needed anymore; unmap it */
	udi_pio_unmap(dv->pio_handles[SHRK_PIO_WAKEUP]);
	dv->pio_handles[SHRK_PIO_WAKEUP] = UDI_NULL_PIO_HANDLE;
	
	dv->srom_contents = new_mem;

	udi_pio_trans(shrk_process_srom, cb,
	    dv->pio_handles[SHRK_PIO_READ_SROM], 0, NULL, dv->srom_contents);
}

/*
 * shrk_process_srom
 *	- primary region
 *
 * callback routine after reading srom
 * store mac address
 */
void
shrk_process_srom(udi_cb_t *cb, udi_buf_t *buf, udi_status_t st, udi_ubit16_t res)
{
	shrktx_rdata_t *dv = cb->context;
	udi_ubit8_t i;
	shrkmacaddr_t sm, *smp;
	udi_ubit16_t ma[SHRK_MACADDRSIZE/2];

	DEBUG01(("shrk_process_srom(cb %x buf %x st %x res %x) dv %x\n",
							cb, buf, st, res, dv));

#ifdef SHRK_DEBUG
	DEBUG07(("SROM contents:\n"));
	for (i = 0; i < SHRK_SROMSIZE; i += 16) {
		DEBUG07(("%04x %04x   %04x %04x   %04x %04x   %04x %04x\n",
			*((udi_ubit16_t *)&(dv->srom_contents[i])),
			*((udi_ubit16_t *)&(dv->srom_contents[i+2])),
			*((udi_ubit16_t *)&(dv->srom_contents[i+4])),
			*((udi_ubit16_t *)&(dv->srom_contents[i+6])),
			*((udi_ubit16_t *)&(dv->srom_contents[i+8])),
			*((udi_ubit16_t *)&(dv->srom_contents[i+10])),
			*((udi_ubit16_t *)&(dv->srom_contents[i+12])),
			*((udi_ubit16_t *)&(dv->srom_contents[i+14]))));
	}
#endif /* SHRK_DEBUG */

	/* srom not needed anymore, unmap it */
	udi_pio_unmap(dv->pio_handles[SHRK_PIO_READ_SROM]);
	dv->pio_handles[SHRK_PIO_READ_SROM] = UDI_NULL_PIO_HANDLE;
	
	/* handle endianness since mac address is stored in host memory */
	smp = &sm;
	udi_memcpy((void *)ma,
		(const void *)(dv->srom_contents + SHRK_SROM_MAC_OFF),
		SHRK_MACADDRSIZE);
	for (i = 0; i < (udi_ubit8_t)(SHRK_MACADDRSIZE/2); i++) {
		UDI_MBSET_2(smp, mac, ma[i]);
		dv->factaddr[i*2] = smp->mac0;
		dv->factaddr[(i*2)+1] = smp->mac1;
	}

	udi_memcpy((void *)dv->hwaddr, (const void *)dv->factaddr,
		SHRK_MACADDRSIZE);

	/* initial call of recursive callback with bogus buf pointer */
	shrk_mdio_read(cb, (udi_buf_t *)-1, 0, 0);
}

/*
 * recursive callback to locate phy
 */
void
shrk_mdio_read(udi_cb_t *cb, udi_buf_t *buf, udi_status_t st, udi_ubit16_t res)
{
	shrktx_rdata_t *dv = cb->context;

	if (buf == (udi_buf_t *)-1) {
	    dv->phy = -1;
	} else if ((*((udi_ubit32_t *)cb->scratch) & 0x10000) == 0) {
		dv->phy_status = *((udi_ubit32_t *)cb->scratch) & 0xffff;

		if (dv->phy_status == 0) {
			DEBUG02(("shrk_mdio_read: bad phy!\n"));

			/* no MII; skip over MDIO */
			dv->link_index = 0;
			if (!shrk_check_link_index(dv))
				shrk_new_link_index(dv);

			shrk_nic_reset(cb);
		} else {
			/* MII read of vendor register high half */
			*((udi_ubit32_t *)cb->scratch) =
			  (SHRK_MII_READ << 10) | (dv->phy << 5) | SHRK_PHYIDR1;
			udi_pio_trans(shrk_mdio_read1, cb,
			    dv->pio_handles[SHRK_PIO_MDIO_READ], 0, NULL, NULL);
		}
		return;
	}

	if (++dv->phy < 32) {
		/* MII read of status register */
		*((udi_ubit32_t *)cb->scratch) =
			(SHRK_MII_READ << 10) | (dv->phy << 5) | SHRK_BMSR;
		udi_pio_trans(shrk_mdio_read, cb,
		    dv->pio_handles[SHRK_PIO_MDIO_READ], 0, NULL, NULL);
	} else {
		DEBUG02(("shrk_mdio_read: no phy!\n"));
		dv->phy_status = 0;

		/* no MII; skip over MDIO */
		dv->link_index = 0;
		if (!shrk_check_link_index(dv))
			shrk_new_link_index(dv);

		shrk_nic_reset(cb);
	}
}

void
shrk_mdio_read1(udi_cb_t *cb, udi_buf_t *buf, udi_status_t st, udi_ubit16_t res)
{
	shrktx_rdata_t *dv = cb->context;

	dv->phy_id = (*((udi_ubit32_t *)cb->scratch) & 0xffff) << 16;

	/* MII read of vendor register low half */
	*((udi_ubit32_t *)cb->scratch) =
	    (SHRK_MII_READ << 10) | (dv->phy << 5) | SHRK_PHYIDR2;
	udi_pio_trans(shrk_mdio_read2, cb,
	    dv->pio_handles[SHRK_PIO_MDIO_READ], 0, NULL, NULL);
}

void
shrk_mdio_read2(udi_cb_t *cb, udi_buf_t *buf, udi_status_t st, udi_ubit16_t res)
{
	shrktx_rdata_t *dv = cb->context;

	dv->phy_id |= *((udi_ubit32_t *)cb->scratch) & 0xffff;

	DEBUG07(("shrk_mdio_read: phy %d status 0x%x ID 0x%x,%d,%d\n",
		dv->phy, dv->phy_status, (dv->phy_id >> 10) & 0xfffff,
		(dv->phy_id >> 4) & 0x3f, (dv->phy_id) & 0xf));

	/* MII write of control register */
	/* just set auto-negotiate for now */
	*((udi_ubit32_t *)cb->scratch) = (((SHRK_MII_WRITE << 10)
				| (dv->phy << 5) | SHRK_BMCR) << 18)
			| 0x20000 /* mii idle */
			| SHRK_BMCR_AUTO;
	udi_pio_trans(shrk_mdio_write, cb,
	    dv->pio_handles[SHRK_PIO_MDIO_WRITE], 0, NULL, NULL);
}

/*
 * mdio write complete
 */
void
shrk_mdio_write(udi_cb_t *cb, udi_buf_t *buf, udi_status_t st, udi_ubit16_t res)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG01(("shrk_mdio_write(cb %x buf %x st %x res %x) dv %x\n",
							cb, buf, st, res, dv));
	dv->link_index = 0;
	if (!shrk_check_link_index(dv))
		shrk_new_link_index(dv);

	shrk_nic_reset(cb);
}

/*
 *-----------------------------------------------------------------------------
 * GIO Metalanguage Entrypoints (used for inter-region communication)
 * Secondary region
 */

/*
 * shrkrx_gio_channel_event_ind
 *	- secondary region gio (client) channel
 *
 * The GIO CHANNEL_BOUND event is the second event the driver receives
 *	(after the management meta usage indication)
 */
void
shrkrx_gio_channel_event_ind(udi_channel_event_cb_t *cb)
{
	shrkrx_rdata_t *rxdv = UDI_GCB(cb)->context;
	udi_channel_t chan = UDI_GCB(cb)->channel;

	DEBUG01(("shrkrx_gio_channel_event_ind(cb %x) rxdv %x chan %x ev %x\n",
		cb, rxdv, chan, cb->event));

	switch (cb->event) {
	case UDI_CHANNEL_BOUND:
		DEBUG03(("shrkrx_gio_channel_event_ind: gio CHANNEL_BOUND\n"));

		rxdv->gio_chan = chan;
		rxdv->nic_state = UNBOUND;

		udi_cb_alloc(shrkrx_gio_bind_cb, UDI_GCB(cb),
			SHRK_GIO_BIND_CB_IDX, chan);

		break;
	case UDI_CHANNEL_CLOSED:
		DEBUG03(("gio_chan CLOSED\n"));
		break;
	default:
		UDI_GCB(cb)->initiator_context = cb;
		udi_log_write(shrk_log_channel, UDI_GCB(cb), UDI_TREVENT_LOG,
			UDI_LOG_ERROR, SHRK_NIC_META, UDI_STAT_NOT_UNDERSTOOD,
			100, "shrkrx_gio_channel_event_ind", cb->event);
		break;
	}
}

/*
 * shrkrx_gio_bind_cb
 *	- secondary region
 *
 * gio bind cb allocated callback - issue gio bind request to gio provider
 * TODO2: have this cb preallocated
 */
void
shrkrx_gio_bind_cb(udi_cb_t *cb, udi_cb_t *new_cb)
{
	DEBUG03(("shrkrx_gio_bind_cb(cb %x new_cb %x)\n", cb, new_cb));

	/*
	 * Stash the originating udi_channel_event_cb_t away for the subsequent
	 * udi_channel_event_complete() call
	 */
	new_cb->initiator_context = cb;

	udi_cb_alloc(shrkrx_log_cb, new_cb,
		SHRK_GCB_CB_IDX, UDI_NULL_CHANNEL);
}

/*
 * log cb allocated
 */
void
shrkrx_log_cb(udi_cb_t *cb, udi_cb_t *new_cb)
{
	shrkrx_rdata_t *rxdv = cb->context;

	DEBUG01(("shrkrx_log_cb(cb %x new_cb %x) rxdv %x\n", cb, new_cb, rxdv));

	rxdv->log_cb = new_cb;

	udi_gio_bind_req(UDI_MCB(cb, udi_gio_bind_cb_t));

}

/*
 * shrkrx_gio_bind_ack
 *	- secondary region
 *
 * acknowlegment of bind to primary
 */
void
shrkrx_gio_bind_ack(udi_gio_bind_cb_t *cb, udi_ubit32_t lo, udi_ubit32_t hi, udi_status_t st)
{
	shrkrx_rdata_t *rxdv = UDI_GCB(cb)->context;

	DEBUG01(("shrkrx_gio_bind_ack(cb %x lo %x hi %x st %x) rxdv %x\n",
		cb, lo, hi, st, rxdv));

	udi_assert(rxdv->gio_chan == UDI_GCB(cb)->channel);

	if (st != UDI_OK) {
		DEBUG02(("shrkrx_gio_bind_ack: bad status %d\n", st));
	}

	rxdv->pdi = rxdv->cdi = 0;

	udi_cb_alloc(shrkrx_gioxfer_cb, UDI_GCB(cb), SHRK_GIO_XFER_CB_IDX,
		rxdv->gio_chan);
}

/*
 * shrkrx_gioxfer_cb
 *	- gio client, secondary region
 *
 * callback routine for gio_xfer cb allocation
 */
void
shrkrx_gioxfer_cb(udi_cb_t *cb, udi_cb_t *new_cb)
{
	shrkrx_rdata_t *rxdv = cb->context;
	udi_channel_event_cb_t *ecb;
	udi_gio_xfer_cb_t *xcb;

	DEBUG01(("shrkrx_gioxfer_cb(cb %x new_cb %x) dv %x\n", cb, new_cb, rxdv));

	xcb = UDI_MCB(new_cb, udi_gio_xfer_cb_t);

	rxdv->gio_xfer_cb = xcb;

	/* complete internal gio channel_event_complete CHANNEL_BOUND */
	ecb = UDI_MCB(cb->initiator_context, udi_channel_event_cb_t);
	DEBUG03(("shrkrx_gioxfer_cb ecb %x\n", ecb));

	udi_channel_event_complete(ecb, UDI_OK);

	udi_cb_free(cb); /* free bind cb */
}

/*
 * shrkrx_gio_event_ind
 *	- secondary region
 *
 * handles gio channel events in rx region
 */
void
shrkrx_gio_event_ind(udi_gio_event_cb_t *cb)
{
	shrkrx_rdata_t *rxdv = UDI_GCB(cb)->context;
	shrk_gio_event_params_t *evparams
				= (shrk_gio_event_params_t *)cb->event_params;

	DEBUG01(("shrkrx_gio_event_ind(cb %x) rxdv %x eventcode %x rflags %x\n",
		cb, rxdv, cb->event_code, rxdv->flags));

	switch (cb->event_code) {
	case SHRK_GIO_GIO_BIND:
		DEBUG03(("shrkrx_gio_event_ind SHRK_GIO_GIO_BIND\n"));
		rxdv->max_desc = SHRK_RING_SIZE;
		break;
	case SHRK_GIO_NIC_BIND:
		DEBUG03(("shrkrx_gio_event_ind SHRK_GIO_NIC_BIND rx_chan: rxdv %x params %x\n",
			rxdv->rx_chan, evparams->rx_chan));
		rxdv->rx_chan = evparams->rx_chan;
		rxdv->nic_state = BOUND;
		/* Anchor rx channel using SHRK_ND_RX_IDX ops */
		udi_channel_anchor(shrkrx_rx_chan_anchored, UDI_GCB(cb), 
			rxdv->rx_chan, SHRK_ND_RX_IDX, (void *)rxdv);
		return;
	case SHRK_GIO_NIC_ENABLE:
		DEBUG03(("shrkrx_gio_event_ind SHRK_GIO_NIC_ENABLE\n"));
		rxdv->nic_state = ENABLED;
		break;
	case SHRK_GIO_NIC_LINK_UP:
		DEBUG03(("shrkrx_gio_event_ind SHRK_GIO_NIC_LINK_UP\n"));
		shrkrx_sked_receive(rxdv);	/* remap buffers */
		rxdv->nic_state = ACTIVE;
		break;
	case SHRK_GIO_NIC_LINK_DOWN:
		DEBUG03(("shrkrx_gio_event_ind SHRK_GIO_NIC_LINK_DOWN\n"));
		shrkrx_clean_ring(rxdv);
		rxdv->pdi = rxdv->cdi = 0;
		rxdv->nic_state = ENABLED;
		break;
	case SHRK_GIO_NIC_DISABLE:
		DEBUG03(("shrkrx_gio_event_ind SHRK_GIO_NIC_DISABLE\n"));
		shrkrx_clean_ring(rxdv);
#if 0 /* NSR doesn't give then back! */
		if (rxdv->rxrdy_cb != NULL) {
			udi_nsr_rx_ind(rxdv->rxrdy_cb);
			rxdv->rxrdy_cb = NULL;
		}
#endif
		rxdv->pdi = rxdv->cdi = 0;
		rxdv->nic_state = BOUND;
		break;
	case SHRK_GIO_NIC_UNBIND:
		DEBUG03(("shrkrx_gio_event_ind SHRK_GIO_NIC_UNBIND\n"));

		/* close rx channel */
		udi_channel_close(rxdv->rx_chan);
		rxdv->rx_chan = UDI_NULL_CHANNEL;

		udi_dma_free(rxdv->rxd_dmah);
		/* Note: environment unmaps interrupt preprocessing pio */
		/* TODO2 free bus resources, rx cbs, unmap rx bufs, rx_dmah's */
		rxdv->flags |= SHRK_BUS_UNBIND;
		rxdv->nic_state = UNBOUND;
		udi_cb_alloc(shrkrx_gio_close_cb, UDI_GCB(cb),
			SHRK_BUS_BIND_CB_IDX, rxdv->bus_chan);
		return;
	case SHRK_GIO_NIC_INFO:
		DEBUG03(("shrkrx_gio_event_ind SHRK_GIO_NIC_INFO\n"));
		evparams = (shrk_gio_event_params_t *)cb->event_params;
		evparams->rx_packets = rxdv->rx_packets;
		evparams->rx_errors = rxdv->rx_errors;
		evparams->rx_discards = rxdv->rx_discards;
		evparams->tx_underrun = rxdv->tx_underrun;
		evparams->rx_overrun = rxdv->rx_overrun;
		evparams->rx_desc_unavail = rxdv->rx_desc_unavail;

		if (evparams->reset_stats) {
			DEBUG03(("shrkrx_gio_event_ind INFO reset_stats\n"));
			rxdv->rx_packets = 0;
			rxdv->rx_errors = 0;
			rxdv->rx_discards = 0;
			rxdv->tx_underrun = 0;
			rxdv->rx_overrun = 0;
			rxdv->rx_desc_unavail = 0;
		}

		break;
	default:
		udi_assert(0);
	}

	udi_gio_event_res(cb);

}

/*
 * shrkrx_rx_chan_anchored
 *	- secondary region
 *
 * callback routine for rx channel anchor
 */
void
shrkrx_rx_chan_anchored(udi_cb_t *cb, udi_channel_t chan)
{
	shrkrx_rdata_t *rxdv = cb->context;
	udi_gio_event_cb_t *ecb = UDI_MCB(cb, udi_gio_event_cb_t);

	DEBUG01(("shrkrx_rx_chan_anchored(cb %x chan %x) rxdv %x flags %x rxd_cnst %x\n",
		cb, chan, rxdv, rxdv->flags, rxdv->rxd_cnst));

	rxdv->rx_chan = chan;

	/* notify primary region rx channel anchored with gioxfer */
	udi_gio_event_res(ecb);
}

/*
 * shrkrx_gio_xfer_ack
 *	- gio client, secondary region
 *
 * gio transfer acknowledement
 */
void
shrkrx_gio_xfer_ack(udi_gio_xfer_cb_t *xcb)
{
	shrkrx_rdata_t *rxdv = UDI_GCB(xcb)->context;

	DEBUG01(("shrkrx_gio_xfer_ack(cb %x) rxdv %x bus_chan %x\n",
		xcb, rxdv, rxdv->bus_chan));

	rxdv->gio_xfer_cb = xcb;

	switch (xcb->op) {
	case SHRK_GIO_BUS_BIND:
		DEBUG03(("shrkrx_gio_xfer_ack GIO_BUS_BIND\n"));

		/* continue rx thread with bus bind cb */
		udi_pio_map(shrkrx_intr_preprocess_map,
			UDI_GCB(xcb)->initiator_context, UDI_PCI_BAR_1, 0,
			SHRK_PIO_LENGTH, shrk_intr_preproc_pio,
			sizeof(shrk_intr_preproc_pio) / sizeof(udi_pio_trans_t),
			UDI_PIO_LITTLE_ENDIAN|UDI_PIO_STRICTORDER, 0, 0);
		UDI_GCB(xcb)->initiator_context = NULL;
		break;
	case SHRK_GIO_TX_COMPLETE:
		DEBUG03(("shrkrx_gio_xfer_ack SHRK_GIO_TX_COMPLETE\n"));
		break;
	default:
		udi_assert(0);
	}
}

/*
 * shrkrx_gio_xfer_nak
 *	- gio client, secondary region
 *
 * gio transfer negative acknowledement
 */
void
shrkrx_gio_xfer_nak(udi_gio_xfer_cb_t *cb, udi_status_t st)
{
	shrkrx_rdata_t *rxdv = UDI_GCB(cb)->context;

	DEBUG03(("shrkrx_gio_xfer_nak(cb %x st %x)\n", cb, st));

	rxdv->gio_xfer_cb = cb;
}

/*
 * shrkrx_gio_unbind_ack
 *	- gio client, secondary region
 *
 * acknowledge a gio unbind request
 * free up gio_bind control block
 */
void
shrkrx_gio_unbind_ack(udi_gio_bind_cb_t *cb)
{
	DEBUG_VAR(shrkrx_rdata_t *rxdv = UDI_GCB(cb)->context;)

	DEBUG01(("shrkrx_gio_unbind_ack(cb %x) rxdv %x\n", cb, rxdv));

	udi_cb_free(UDI_GCB(cb));
}

/*
 *-----------------------------------------------------------------------------
 * Initialization happening in secondary during Bus Bind due to GIO xfer ack
 *  - attach interrupts
 */

/*
 * shrkrx_intr_preprocess_map
 *	- secondary region
 *
 * interrupt preprocessing pio handle is mapped
 */
void
shrkrx_intr_preprocess_map(udi_cb_t *cb, udi_pio_handle_t pio)
{
	shrkrx_rdata_t *rxdv = cb->context;

	DEBUG01(("shrkrx_intr_preprocess_map(cb %x pio %x) rxdv %x\n",
		cb, pio, rxdv));

	rxdv->pio_intr_prep = pio;

	/* spawn our end of the interrupt channel, spawn_idx == interrupt_idx */
	udi_channel_spawn(shrkrx_intr_chan, cb, rxdv->bus_chan, 0,
		SHRK_INTR_HANDLER_IDX, (void *)rxdv);
}

/*
 * shrkrx_intr_chan
 *	- secondary region
 *
 * interrupt channel spawned callback
 */
void
shrkrx_intr_chan(udi_cb_t *cb, udi_channel_t chan)
{
	shrkrx_rdata_t *rxdv = cb->context;

	DEBUG01(("shrkrx_intr_chan(cb %x chan %x) rxdv %x iacb size %x\n",
		cb, chan, rxdv, sizeof(udi_intr_attach_cb_t)));

	rxdv->intr_chan = chan;

	udi_cb_alloc(shrkrx_intr_attach_cb, cb,
		SHRK_BUS_INTR_ATTACH_CB_IDX, rxdv->bus_chan);
}

/*
 * shrkrx_intr_attach_cb
 *	- secondary region
 *
 * interrupt attach cb allocated callback
 */
void
shrkrx_intr_attach_cb(udi_cb_t *cb, udi_cb_t *new_cb)
{
	shrkrx_rdata_t *rxdv = cb->context;
	udi_intr_attach_cb_t *icb;

	DEBUG01(("shrkrx_intr_attach_cb(cb %x new_cb %x) rxdv %x int_prep %x\n",
		cb, new_cb, rxdv, rxdv->pio_intr_prep));

	icb = UDI_MCB(new_cb, udi_intr_attach_cb_t);

	icb->interrupt_idx = 0;  /* one interrupt source */
	icb->min_event_pend = SHRK_MIN_INTR_EVENT_CBS; /* TODO2: custom param */
	icb->preprocessing_handle = rxdv->pio_intr_prep;

	udi_intr_attach_req(icb);
}

/*
 *-----------------------------------------------------------------------------
 * Bus Bridge Metalanguage Entrypoints
 */

/*
 * shrkrx_bus_channel_event_ind
 *	- secondary region
 *
 * handles bus bridge channel events
 *
 * The bus CHANNEL_BOUND event is the third event the driver receives
 *	(after the GIO CHANNEL_BOUND)
 */
void
shrkrx_bus_channel_event_ind(udi_channel_event_cb_t *cb)
{
	shrkrx_rdata_t *rxdv = UDI_GCB(cb)->context;
	udi_channel_t chan = UDI_GCB(cb)->channel;
	udi_bus_bind_cb_t *bus_bind_cb;

	DEBUG01(("shrkrx_bus_channel_event_ind(cb %x) rxdv %x event %x chan %x\n",
		cb, rxdv, cb->event, chan));

	switch (cb->event) {
	case UDI_CHANNEL_BOUND:
		DEBUG03(("shrkrx_bus_channel_event_ind: CHANNEL_BOUND\n"));

		rxdv->bus_chan = chan;
		rxdv->chan_cb = cb;
		bus_bind_cb = UDI_MCB(cb->params.parent_bound.bind_cb,
									udi_bus_bind_cb_t);
		UDI_GCB(bus_bind_cb)->initiator_context = cb;
		udi_bus_bind_req(bus_bind_cb);
		break;
	case UDI_CHANNEL_OP_ABORTED:
		DEBUG03(("shrkrx_bus_channel_event_ind: CHANNEL_OP_ABORTED\n"));
		udi_channel_event_complete(cb, UDI_STAT_NOT_SUPPORTED);
		break;
	case UDI_CHANNEL_CLOSED:
		/* TODO3 free resources, disable hardware */
		DEBUG03(("shrk_bus_channel_event: CLOSED\n"));
		rxdv->bus_chan = UDI_NULL_CHANNEL;
		udi_channel_event_complete(cb, UDI_OK);
		udi_channel_close(chan);
		break;
	default:
		UDI_GCB(cb)->initiator_context = cb;
		udi_log_write(shrk_log_channel, UDI_GCB(cb), UDI_TREVENT_LOG,
			UDI_LOG_ERROR, SHRK_NIC_META, UDI_STAT_NOT_UNDERSTOOD,
			100, "shrkrx_bus_channel_event_ind", cb->event);
		break;
	}
}

/*
 * shrkrx_bus_bind_ack
 *	- secondary region
 *
 * acknowledge bus bridge binding
 */
void
shrkrx_bus_bind_ack(udi_bus_bind_cb_t *cb, 
		udi_dma_constraints_t parent_constraints,
		udi_ubit8_t pref, udi_status_t st)
{
	udi_channel_event_cb_t *ecb;
	shrk_constraints_set_scratch_t *scratch = UDI_GCB(cb)->scratch;

	ecb = UDI_MCB(UDI_GCB(cb)->initiator_context, udi_channel_event_cb_t);

	DEBUG01(("shrkrx_bus_bind_ack(cb %x pref %x st %x) rxdv %x ecb %x\n",
		cb, pref, st, UDI_GCB(cb)->context, ecb));

	if (st == UDI_OK) {
		scratch->parent_constraints = parent_constraints;
 		/* make a copy; send one to tx region; keep the other */
 		udi_dma_constraints_attr_set(shrkrx_rx_constraints_set,
  			UDI_GCB(cb), parent_constraints,
 			shrk_constraints,
 			sizeof(shrk_constraints) /
 				sizeof(udi_dma_constraints_attr_spec_t),
 			UDI_DMA_CONSTRAINTS_COPY);
	} else {
		/* TODO3 log */
		DEBUG02(("shrkrx_bus_bind_ack: bad status %d\n", st));
		udi_channel_event_complete(ecb, UDI_STAT_CANNOT_BIND);
	}
}

/*
 * shrkrx_intr_attach_ack
 *	- secondary region
 *
 * interrupts attached, complete outstanding bus channel_event CHANNEL_BOUND
 * bus bind event complete; nd bind is next
 */
void
shrkrx_intr_attach_ack(udi_intr_attach_cb_t *cb, udi_status_t st)
{
	shrkrx_rdata_t *rxdv = UDI_GCB(cb)->context;

	DEBUG01(("shrkrx_intr_attach_ack(cb %x st %x) rxdv %x intr_prep %x\n",
		cb, st, rxdv, cb->preprocessing_handle));

	udi_assert(rxdv->chan_cb);
	if (st == UDI_OK) {
		udi_channel_event_complete(rxdv->chan_cb, UDI_OK);

		/* allocate intr_event cbs, pass to dispatcher */
		udi_cb_alloc(shrkrx_intr_cb, UDI_GCB(cb),
			SHRK_BUS_INTR_EVENT_CB_IDX, rxdv->intr_chan);
	} else {
		/* TODO3 log */
		udi_cb_free(UDI_GCB(cb));
		DEBUG02(("shrkrx_intr_attach_ack: bad status %d\n", st));
		udi_channel_event_complete(rxdv->chan_cb, UDI_STAT_CANNOT_BIND);
	}

	rxdv->chan_cb = NULL;
}

/*
 * shrkrx_intr_cb
 *	- secondary region
 *
 * interrupts attached, bus channel bound, allocate interrupt event cbs
 */
void
shrkrx_intr_cb(udi_cb_t *cb, udi_cb_t *new_cb)
{
	shrkrx_rdata_t *rxdv = cb->context;
	udi_intr_event_cb_t *iecb = UDI_MCB(new_cb, udi_intr_event_cb_t);

	DEBUG01(("shrkrx_intr_cb(cb %x new_cb %x) rxdv %x\n",
		cb, new_cb, rxdv));

	iecb->event_buf = NULL;
	udi_intr_event_rdy(iecb);
	
	/* TODO2: custom param */
	if (++rxdv->intr_event_cbs < SHRK_INTR_EVENT_CBS) {
		udi_cb_alloc(shrkrx_intr_cb, cb,
			SHRK_BUS_INTR_EVENT_CB_IDX, rxdv->intr_chan);
	} else {
		/* free intr_attach_ack cb */
		udi_cb_free(cb);
	}
}

/*
 * shrkrx_intr_event_ind
 *	- secondary region
 *
 * handles interrupt events
 */
void
shrkrx_intr_event_ind(udi_intr_event_cb_t *cb, udi_ubit8_t flags)
{
	shrkrx_rdata_t *rxdv = UDI_GCB(cb)->context;
	udi_ubit16_t status = cb->intr_result;

	DEBUG13(("shrkrx_intr_event_ind(cb %x flags %x) status %x\n",
		cb, flags, status));

	if (status & ST_RI) {
		shrkrx_rx(rxdv);
	}
	if (status & ST_ERI) {
		DEBUG13(("shrkrx_intr_event_ind: tx ERI\n"));
		udi_assert(!(status & ST_RI)); /* nic clears ERI on RI */
		shrkrx_rx(rxdv);
	}
	if (status & ST_TI) {
		DEBUG13(("shrkrx_intr_event_ind: tx TI\n"));
		if (rxdv->gio_xfer_cb) {
			rxdv->gio_xfer_cb->op = SHRK_GIO_TX_COMPLETE;
			udi_gio_xfer_req(rxdv->gio_xfer_cb);
			rxdv->gio_xfer_cb = NULL;
		}
	}
#ifdef SHRK_DEBUG
	if (status & ST_TU) {
		DEBUG02(("shrkrx_intr_event_ind: tx TU\n"));
	}
	if (status & ST_GTE) {
		/* shouldn't happen - general purpose timer not used */
		DEBUG02(("shrkrx_intr_event_ind: GPT expired\n"));
	}
	if (status & ST_FBE) {
		/* fatal bus error */
		switch (status & ST_EB_MASK) {
		case ST_EB_PARITY:
			DEBUG02(("shrkrx_intr_event_ind: parity error\n"));
			break;
		case ST_EB_MABORT:
			DEBUG02(("shrkrx_intr_event_ind: master abort\n"));
			break;
		case ST_EB_TABORT:
			DEBUG02(("shrkrx_intr_event_ind: target abort\n"));
			break;
		default:
			DEBUG02(("shrkrx_intr_event_ind: Fatal Bus Error %x\n",
				status & ST_EB_MASK));
			break;
		}
	}
#endif /* SHRK_DEBUG */

	if (status & ST_FBE) {
		if ((status & ST_FBE) == ST_EB_PARITY) {
			/* TODO - issue software reset */
		}
	}
	if (status & ST_AIS) {
		if (status & ST_UNF) {
			/* transmit underflow */
			rxdv->tx_underrun++;
			DEBUG13(("shrkrx_intr_event_ind: tx UNF\n"));
		}
		if (status & ST_RU) {
			/* receive buffer unavailable */
			rxdv->rx_desc_unavail++;
			DEBUG13(("shrkrx_intr_event_ind: RU\n"));
#ifdef SHRK_DEBUG
			shrkrxd(rxdv);
#endif /* SHRK_DEBUG */
		}

#ifdef SHRK_DEBUG
		if (status & ST_TPS) {
			/* transmit process stopped */
			DEBUG02(("shrkrx_intr_event_ind: TPS\n"));
		}
		if (status & ST_TJT) {
			/* transmit jabber timeout */
			DEBUG02(("shrkrx_intr_event_ind: TJT\n"));
		}
		if (status & ST_RPS) {
			/* receive process stopped */
			DEBUG02(("shrkrx_intr_event_ind: RPS\n"));
		}
		if (status & ST_RWT) {
			/* receive watchdog timeout */
			DEBUG02(("shrkrx_intr_event_ind: RWT\n"));
		}
		if (status & ST_ETI) {
			/*
			 * Early transmit interrupts are not enabled, but they
			 * may occur when other interrupts are processed when
			 * the transmit packet has been fully transferred into
			 * the chip's transmit FIFOs.
			 */
			DEBUG02(("shrkrx_intr_event_ind: ETI\n"));
		}
#endif /* SHRK_DEBUG */
	}

	udi_intr_event_rdy(cb);
}

/*
 * shrkrx_intr_channel_event_ind
 *	- secondary region
 *
 * handles interrupt channel events
 */
void
shrkrx_intr_channel_event_ind(udi_channel_event_cb_t *cb)
{
	udi_channel_t chan = UDI_GCB(cb)->channel;

	DEBUG01(("shrk_intr_channel_event_ind(cb %x) chan %x event %x\n",
		cb, chan, cb->event));

	switch (cb->event) {
	case UDI_CHANNEL_CLOSED:
		udi_channel_event_complete(cb, UDI_OK);
		udi_channel_close(chan);
		break;
	case UDI_CHANNEL_BOUND:      /* won't happen since bus is initiator */
	case UDI_CHANNEL_OP_ABORTED: /* intr channel ops not abortable */
	default:
		UDI_GCB(cb)->initiator_context = cb;
		udi_log_write(shrk_log_channel, UDI_GCB(cb), UDI_TREVENT_LOG,
			UDI_LOG_ERROR, SHRK_BUS_META, UDI_STAT_NOT_UNDERSTOOD,
			100, "shrk_intr_channel_event_ind", cb->event);
		break;
	}
}

/*
 * shrkrx_bus_unbind_ack
 *	- secondary region
 *
 * acknowledge bus bridge unbinding
 */
void
shrkrx_bus_unbind_ack(udi_bus_bind_cb_t *cb)
{
	shrkrx_rdata_t *rxdv = UDI_GCB(cb)->context;

	DEBUG01(("shrkrx_bus_unbind_ack(cb %x) rxdv %x initiator %x\n",
		cb, rxdv, UDI_GCB(cb)->initiator_context));

	rxdv->flags &= ~SHRK_BUS_UNBIND;
	rxdv->flags |= SHRK_INTR_DETACH;
	/* the gcb will eventually get used for the channel_event_ind */
	udi_cb_alloc(shrkrx_gio_close_cb, UDI_GCB(cb),
		SHRK_BUS_INTR_DETACH_CB_IDX, rxdv->bus_chan);
}

/*
 * shrkrx_gio_close_cb
 *	- secondary region
 *
 * gio close cb allocation handler used during teardown
 */
void
shrkrx_gio_close_cb(udi_cb_t *cb, udi_cb_t *new_cb)
{
	shrkrx_rdata_t *rxdv = cb->context;
	udi_intr_detach_cb_t *dcb;

	DEBUG01(("shrkrx_gio_close_cb(cb %x new_cb %x) rxdv %x\n",
		cb, new_cb, rxdv));

	new_cb->initiator_context = cb;

	switch(rxdv->flags & (SHRK_BUS_UNBIND|SHRK_INTR_DETACH)) {
	case SHRK_BUS_UNBIND:
		DEBUG11(("shrkrx_gio_close_cb: BUS_UNBIND\n"));
		udi_gio_event_res(UDI_MCB(cb, udi_gio_event_cb_t));
		udi_bus_unbind_req(UDI_MCB(new_cb, udi_bus_bind_cb_t));
		break;
	case SHRK_INTR_DETACH:
		DEBUG11(("shrkrx_gio_close_cb: INTR_DETACH\n"));
		dcb = UDI_MCB(new_cb, udi_intr_detach_cb_t);
		dcb->interrupt_idx = 0;
		udi_intr_detach_req(dcb);
		break;
	default:
		udi_assert(0);
	}
}

/*
 * shrkrx_intr_detach_ack
 *	- secondary region
 *
 * interrupt detached callback from bus bridge
 */
void
shrkrx_intr_detach_ack(udi_intr_detach_cb_t *cb)
{
	DEBUG_VAR(udi_channel_event_cb_t *ecb;)

	DEBUG01(("shrk_intr_detach_cb(cb %x) ecb %x\n", cb, ecb));
	udi_assert(cb->interrupt_idx == 0);

	udi_cb_free(UDI_GCB(cb));
}

/*
 *-----------------------------------------------------------------------------
 * Initialization happening in secondary after bus meta bind ack
 *  - build constraints and allocat dma resources
 *  - pass constraints to primary
 */

/*
 * shrkrx_rx_constraints_set
 *	- secondary region
 *
 * constraints handle set callback
 * allocate rx descriptors
 */
void
shrkrx_rx_constraints_set(udi_cb_t *cb, udi_dma_constraints_t cnst,
			  udi_status_t st)
{
	shrkrx_rdata_t *rxdv = cb->context;
	udi_channel_event_cb_t *ecb;

	DEBUG01(("shrkrx_rx_constraints_set(cb %x cnst %x st %x) rxdv %x\n",
		cb, cnst, st, rxdv));

	if (st != UDI_OK) {
		DEBUG02(("shrkrx_rx_constraints_set: bad state %d\n", st));
		udi_dma_constraints_free(cnst);
		ecb = UDI_MCB(cb->initiator_context, udi_channel_event_cb_t);
		/* TODO3: track state */
		udi_channel_event_complete(ecb, UDI_STAT_CANNOT_BIND);
		return;
	}

	rxdv->rxd_cnst = cnst;

	/* allocate receive descriptors list */
	udi_dma_mem_alloc(shrkrx_rxdesc_alloc, cb, cnst,
		UDI_DMA_OUT|UDI_DMA_LITTLE_ENDIAN,
		rxdv->max_desc, sizeof(rxd_t), 0);
}

/*
 * shrkrx_rxdesc_alloc
 *	- secondary region
 *
 * dma'able memory for nic receive descriptors allocated callback
 */
void
shrkrx_rxdesc_alloc(udi_cb_t *cb, udi_dma_handle_t dmah, void *mem,
	udi_size_t actual_gap, udi_boolean_t single_element,
	udi_scgth_t *scgth, udi_boolean_t must_swap)
{
	shrkrx_rdata_t *rxdv = cb->context;
	
	DEBUG01(("shrkrx_rxdesc_alloc rxdv %x\n", rxdv));
	udi_assert(scgth->scgth_num_elements == 1);
	udi_assert(scgth->scgth_format ==
				(UDI_SCGTH_32 | UDI_SCGTH_DRIVER_MAPPED));

	rxdv->desc = mem;
	rxdv->rxd_dmah = dmah;
	rxdv->swap = must_swap;
	/* mark last descriptor end of ring - must be after rxdv->swap is set */
	rxdv->desc[rxdv->max_desc-1].cntrl = SHRK_SWAP_DESC(rxdv, RX_RER);
	rxdv->rxpaddr = SHRK_SWAP_SCGTH(scgth,
		scgth->scgth_elements.el32p[0].block_busaddr);

	udi_mem_alloc(shrkrx_dma_alloc, cb,
			(rxdv->max_desc - 1) * sizeof(shrk_dma_element_t), 0);
}

/*
 * shrkrx_dma_alloc
 *	- secondary region
 *
 * dma handle storage allocated
 * allocate dma handles
 */
void
shrkrx_dma_alloc(udi_cb_t *cb, void *new_mem)
{
	shrkrx_rdata_t *rxdv = cb->context;

	DEBUG01(("shrkrx_dma_alloc(cb %x new_mem %x)\n", cb, new_mem));

	rxdv->rx_sdmah = rxdv->next_sdmah = (shrk_dma_element_t *)new_mem;

	/* allocate pool of rx dma handles */
	rxdv->index = 0;
	udi_dma_prepare(shrkrx_rx_dmah, cb, rxdv->rxd_cnst, UDI_DMA_IN);
}

/*
 * shrkrx_rx_dmah
 *	- secondary region
 *
 * callback for udi_dma_prepare which allocates dma handles for receive buffers
 */
void
shrkrx_rx_dmah(udi_cb_t *cb, udi_dma_handle_t dmah)
{
	shrkrx_rdata_t *rxdv = cb->context;
	udi_gio_xfer_cb_t *xcb;
	shrk_gio_xfer_params_t *params;
	shrk_constraints_set_scratch_t *scratch = cb->scratch;

	DEBUG01(("shrkrx_rx_dmah(cb %x dmah %x) index %d\n",
		cb, dmah, rxdv->index));

	rxdv->rx_sdmah[rxdv->index].dmah = dmah;

	if (rxdv->index < rxdv->max_desc - 2) {	/* one descriptor is unused */
		rxdv->rx_sdmah[rxdv->index].next =
					&(rxdv->rx_sdmah[rxdv->index + 1]);
		rxdv->index++;
		udi_dma_prepare(shrkrx_rx_dmah, cb, rxdv->rxd_cnst, UDI_DMA_IN);
	} else {
		rxdv->rx_sdmah[rxdv->index].next = NULL;
		/*
		 * now pass the parent constraints to the primary region so that
		 * it can set up the tx and txd constraints
		 */
		udi_assert(rxdv->gio_xfer_cb);
		xcb = rxdv->gio_xfer_cb;
		rxdv->gio_xfer_cb = NULL;

		params = xcb->tr_params;
		params->parent_constraints = scratch->parent_constraints;
		params->rxpaddr = rxdv->rxpaddr;

		/* save bus bind context for ack */
		UDI_GCB(xcb)->initiator_context =
			 (void*)UDI_MCB(cb, udi_bus_bind_cb_t);

		xcb->op = SHRK_GIO_BUS_BIND;
		udi_gio_xfer_req(xcb);
	}
}

/*
 *-----------------------------------------------------------------------------
 * Network Interface Metalanguage Entrypoints
 */

/*
 * shrk_nd_bind_req
 *	-primary region
 *
 * first operation sent to the ND from the NSR over newly created bind channel
 *	- associate (bind) a channel between the NSR and ND
 */
void
shrk_nd_bind_req(udi_nic_bind_cb_t *cb, udi_index_t tx_idx, udi_index_t rx_idx)
{
	shrktx_rdata_t *dv = UDI_GCB(cb)->context;

	DEBUG01(("shrk_nd_bind_req(cb %x tx_idx %x rx_idx %x) dv %x nic_state %x\n",
		cb, tx_idx, rx_idx, dv, dv->nic_state));

	dv->bind_cb = cb;

	if ((!(dv->flags & SHRK_RUNNING)) || (dv->nic_state != UNBOUND)) {
		shrk_nd_bind_ack(dv, UDI_STAT_INVALID_STATE);
		return;
	}

	dv->nic_state = BINDING;
	dv->ctl_chan = UDI_GCB(cb)->channel;
	dv->tx_idx = tx_idx;
	dv->rx_idx = rx_idx;

	/* spawn tx channel */
	udi_channel_spawn(shrk_tx_chan_spawned, UDI_GCB(cb),
		dv->ctl_chan, tx_idx, SHRK_ND_TX_IDX, (void *)dv);
}

/*
 * shrk_nd_unbind_req
 *	- primary region
 *
 * handle network unbind request
 */
void
shrk_nd_unbind_req(udi_nic_cb_t *cb)
{
	shrktx_rdata_t *dv = UDI_GCB(cb)->context;

	DEBUG01(("shrk_nd_unbind_req(cb %x) dv %x\n", cb, dv));

	if ((dv->nic_state != BOUND) || (dv->gio_chan == UDI_NULL_CHANNEL)) {
		udi_nsr_unbind_ack(cb, UDI_STAT_INVALID_STATE);
		return;
	}

	if (dv->tx_chan != UDI_NULL_CHANNEL) {
		udi_channel_close(dv->tx_chan);
		dv->tx_chan = UDI_NULL_CHANNEL;
	}

	/* notify secondary region of unbind */
	udi_assert(dv->gio_event_cb);
	UDI_GCB(dv->gio_event_cb)->initiator_context = cb;
	dv->gio_event_cb->event_code = SHRK_GIO_NIC_UNBIND;
	udi_gio_event_ind(dv->gio_event_cb);
	dv->gio_event_cb = NULL;
}

/*
 * shrk_nd_enable_req
 *	- primary region
 *
 * handle network link enable request
 */
void
shrk_nd_enable_req(udi_nic_cb_t *ncb)
{
	shrktx_rdata_t *dv = UDI_GCB(ncb)->context;

	DEBUG01(("shrk_nd_enable_req(ncb %x) dv %x tx_chan %x\n",
						ncb, dv, dv->tx_chan));

	if (dv->nic_state != BOUND) {
		udi_nsr_enable_ack(ncb, UDI_STAT_INVALID_STATE);
		return;
	}
	if (!(dv->flags & SHRK_RUNNING)) {
		udi_nsr_enable_ack(ncb, UDI_STAT_HW_PROBLEM);
		return;
	}
	if (!udi_memcmp(dv->factaddr, shrk_broad, SHRK_MACADDRSIZE)) {
		DEBUG02(("shrk_nd_enable_req: bad mac address\n"));
		udi_nsr_enable_ack(ncb, UDI_STAT_HW_PROBLEM);
		return;
	}


	udi_cb_alloc(shrk_probe_cb, UDI_GCB(ncb),
		SHRK_PROBE_CB_IDX, UDI_NULL_CHANNEL);
}

/*
 * shrk_nd_disable_req
 *	- primary region
 *
 * handle network link disable request operation from NSR
 */
void
shrk_nd_disable_req(udi_nic_cb_t *cb)
{
	shrktx_rdata_t *dv = UDI_GCB(cb)->context;

	DEBUG01(("shrk_nd_disable_req(cb %x) dv %x\n", cb, dv));

	dv->nic_state = BOUND;

	if (dv->timer_cb) {
		udi_timer_cancel(dv->timer_cb);
		dv->timer_cb = NULL;
	}

	/* disable HW activity */
	*((udi_ubit32_t *)UDI_GCB(cb)->scratch) = ~(OM_SR|OM_ST);
	udi_pio_trans(shrk_disabled_pio, UDI_GCB(cb),
		dv->pio_handles[SHRK_PIO_CSR6_AND32], 0, NULL, NULL);

	/*
	 * TODO3 cancel pending async service calls such as udi_dma_buf_map
	 * by storing state in cb scratch, if state is pending, cancel
	 */
}

/*
 * shrk_disabled_pio
 *	- primary region
 *
 * transmit and receive have been disabled, send gio event to secondary region
 */
void
shrk_disabled_pio(udi_cb_t *cb, udi_buf_t *buf, udi_status_t st, udi_ubit16_t r)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG01(("shrk_disabled_pio(cb %x buf %x st %x r %x) dv %x\n",
		cb, buf, st, r, dv));
	udi_assert(dv->gio_event_cb);

	/* nd_disable_req is unacknowledged - free cb */
	udi_cb_free(cb);

	shrk_clean_ring(dv);
	dv->cdi = dv->pdi = 0;

	udi_assert(dv->gio_event_cb);

	dv->gio_event_cb->event_code = SHRK_GIO_NIC_DISABLE;
	udi_gio_event_ind(dv->gio_event_cb);
	dv->gio_event_cb = NULL;
}

/*
 * shrk_nd_info_req
 *	- primary region
 *
 * handle network information request from NSR
 */
void
shrk_nd_info_req(udi_nic_info_cb_t *cb, udi_boolean_t reset_statistics)
{
	shrktx_rdata_t *dv = UDI_GCB(cb)->context;
	shrk_gio_event_params_t *evparams;

	DEBUG01(("shrk_nd_info_req(cb %x reset %x)\n", cb, reset_statistics));

	cb->interface_is_active = FALSE;
	if (dv->flags & SHRK_RUNNING) {
		cb->interface_is_active = TRUE;
	}
	if (dv->nic_state == ACTIVE) {
		cb->link_is_active = TRUE;
		cb->link_mbps = 100;
		if (shrk_link_parms[dv->link_index].flags & SHRK_MEDIA_10) {
			cb->link_mbps = 10;
		}
		cb->link_bps = cb->link_mbps * 1000000;

		cb->is_full_duplex = TRUE;
		if (shrk_link_parms[dv->link_index].flags & SHRK_MEDIA_HDX) {
			cb->is_full_duplex = FALSE;
		}
	} else {
		cb->link_is_active = FALSE;
		cb->link_mbps = 0;
		cb->link_bps = 0;
		cb->is_full_duplex = FALSE;
	}

	udi_assert(dv->gio_event_cb);

	dv->gio_event_cb->event_code = SHRK_GIO_NIC_INFO;
	UDI_GCB(dv->gio_event_cb)->initiator_context = cb;

	evparams = (shrk_gio_event_params_t *)dv->gio_event_cb->event_params;
	dv->reset_stats = evparams->reset_stats = reset_statistics;

	udi_gio_event_ind(dv->gio_event_cb);
	dv->gio_event_cb = NULL;
}

/*
 * shrk_nd_ctrl_req
 *	- primary region
 *
 * handle network control request operation from NSR
 */
void
shrk_nd_ctrl_req(udi_nic_ctrl_cb_t *cb)
{
	shrktx_rdata_t *dv = UDI_GCB(cb)->context;

	DEBUG01(("shrk_nd_ctrl_req(cb %x) dv %x command %x\n",
						cb, dv, cb->command));
	
	switch(cb->command) {
	case UDI_NIC_GET_CURR_MAC:
		DEBUG08(("shrk_nd_ctrl_req: UDI_NIC_GET_CURR_MAC\n"));
		udi_buf_write(shrk_ctrl_macaddr_written, UDI_GCB(cb),
			dv->hwaddr, SHRK_MACADDRSIZE, cb->data_buf, 0,
			SHRK_MACADDRSIZE, UDI_NULL_BUF_PATH);
		break;
	case UDI_NIC_GET_FACT_MAC:
		DEBUG08(("shrk_nd_ctrl_req: UDI_NIC_GET_FACT_MAC\n"));
		udi_buf_write(shrk_ctrl_macaddr_written, UDI_GCB(cb),
			dv->factaddr, SHRK_MACADDRSIZE, cb->data_buf, 0,
			SHRK_MACADDRSIZE, UDI_NULL_BUF_PATH);
		break;
	case UDI_NIC_ADD_MULTI:
		DEBUG08(("shrk_nd_ctrl_req: UDI_NIC_ADD_MULTI\n"));
		shrk_build_setup_frame(dv, cb);
		if (dv->nic_state == ACTIVE)
			shrk_send_setup_frame(UDI_GCB(cb), 0);
		udi_nsr_ctrl_ack(cb, UDI_OK);
		break;
	case UDI_NIC_DEL_MULTI:
		DEBUG08(("shrk_nd_ctrl_req: UDI_NIC_DEL_MULTI\n"));
		shrk_build_setup_frame(dv, cb);
		if (dv->nic_state == ACTIVE)
			shrk_send_setup_frame(UDI_GCB(cb), 0);
		udi_nsr_ctrl_ack(cb, UDI_OK);
		break;
	case UDI_NIC_SET_CURR_MAC:
		DEBUG08(("shrk_nd_ctrl_req: UDI_NIC_SET_CURR_MAC\n"));
		shrk_build_setup_frame(dv, cb);
		if (dv->nic_state == ACTIVE)
			shrk_send_setup_frame(UDI_GCB(cb), 0);
		udi_nsr_ctrl_ack(cb, UDI_OK);
		break;
	case UDI_NIC_PROMISC_ON:
		*((udi_ubit32_t *)UDI_GCB(cb)->scratch) = OM_PR;
		udi_pio_trans(shrk_nsr_ctrl_ack, UDI_GCB(cb),
			dv->pio_handles[SHRK_PIO_CSR6_OR32], 0, NULL, NULL);
		break;
	case UDI_NIC_PROMISC_OFF:
		*((udi_ubit32_t *)UDI_GCB(cb)->scratch) = ~OM_PR;
		udi_pio_trans(shrk_nsr_ctrl_ack, UDI_GCB(cb),
			dv->pio_handles[SHRK_PIO_CSR6_AND32], 0, NULL, NULL);
		break;
	case UDI_NIC_ALLMULTI_ON:
		*((udi_ubit32_t *)UDI_GCB(cb)->scratch) = OM_PM;
		udi_pio_trans(shrk_nsr_ctrl_ack, UDI_GCB(cb),
			dv->pio_handles[SHRK_PIO_CSR6_OR32], 0, NULL, NULL);
		break;
	case UDI_NIC_ALLMULTI_OFF:
		*((udi_ubit32_t *)UDI_GCB(cb)->scratch) = ~OM_PM;
		udi_pio_trans(shrk_nsr_ctrl_ack, UDI_GCB(cb),
			dv->pio_handles[SHRK_PIO_CSR6_AND32], 0, NULL, NULL);
		break;
	case UDI_NIC_HW_RESET:
	case UDI_NIC_BAD_RXPKT:
		/* TODO2 handle these ctrl requests */
		udi_nsr_ctrl_ack(cb, UDI_STAT_NOT_SUPPORTED);
		break;
	default:
		/* TODO3 log error */
		DEBUG02(("shrk_nd_ctrl_req: bad command %x\n", cb->command));
		udi_nsr_ctrl_ack(cb, UDI_STAT_NOT_UNDERSTOOD);
		break;
	}
}

/*
 * shrk_promisc_done
 *	- primary region
 *
 * promiscuous pio transaction completed
 */
void
shrk_nsr_ctrl_ack(udi_cb_t *cb, udi_buf_t *buf, udi_status_t st, udi_ubit16_t res)
{
	udi_nic_ctrl_cb_t *ccb = UDI_MCB(cb, udi_nic_ctrl_cb_t);

	DEBUG01(("shrk_nsr_ctrl_ack(cb %x buf %x st %x res %x)\n",
		cb, buf, st, res));

	udi_nsr_ctrl_ack(ccb, UDI_OK);
}

/*
 * shrk_ctrl_macaddr_written
 *	- primary region
 *
 * udi_buf_write callback - mac address has been written into new_buf
 */
void
shrk_ctrl_macaddr_written(udi_cb_t *cb, udi_buf_t *new_buf)
{
	udi_nic_ctrl_cb_t *ccb = UDI_MCB(cb, udi_nic_ctrl_cb_t);

	DEBUG03(("shrk_ctrl_macaddr_written(cb %x new_buf %x) dv %x\n", cb, new_buf, cb->context));

	ccb->data_buf = new_buf;
	ccb->indicator = SHRK_MACADDRSIZE;

	udi_nsr_ctrl_ack(ccb, UDI_OK);
}

/*
 * shrk_nd_txexp_req
 *	- primary region
 *
 * expedited transmit packet request from NSR 
 * TODO3 send expedited packets before non-expedited; use 2 tx queues
 */
void
shrk_nd_txexp_req(udi_nic_tx_cb_t *cb)
{
	DEBUG01(("shrk_txexp_req(cb %x) dv %x\n", cb, UDI_GCB(cb)->context));

	shrk_nd_tx_req(cb);
}

/*
 * shrk_nd_tx_req
 *	- primary region
 *
 * transmit packet request from NSR 
 */
void
shrk_nd_tx_req(udi_nic_tx_cb_t *txcb)
{
	shrktx_rdata_t *dv = UDI_GCB(txcb)->context;
	udi_nic_tx_cb_t *lastcb;
	udi_ubit32_t j = 0;
#ifdef SHRK_DEBUG
	udi_ubit32_t i = 0;

	for (lastcb = txcb; lastcb; lastcb = lastcb->chain) {
		if (lastcb->tx_buf && lastcb->tx_buf->buf_size < 14) {
			DEBUG02(("shrk_nd_tx_req: runt\n"));
		}
		i++;
	}
#endif /* SHRK_DEBUG */

	if (dv->nic_state != ENABLED && dv->nic_state != ACTIVE) {
		/* NSR returning cbs; add to ready list */
		if (dv->txrdy_cb != NULL) {
		    for (j = 1, lastcb = dv->txrdy_cb; lastcb->chain != NULL;
							lastcb = lastcb->chain)
			j++;
		    lastcb->chain = txcb;
		} else {
		    dv->txrdy_cb = txcb;
		}

		DEBUG15(("shrk_nd_tx_req(cb %x) dv %x %d cbs returned; had %d\n",
			txcb, dv, i, j));
		return;
	}

	if (dv->tx_cb != NULL) {
		for (j = 1, lastcb = dv->tx_cb; lastcb->chain != NULL;
							lastcb = lastcb->chain)
			j++;
		lastcb->chain = txcb;
	} else {
		dv->tx_cb = txcb;
	}

	DEBUG15(("shrk_nd_tx_req(cb %x) %d cbs received, had %d; "
		"cdi %d pdi %d\n", txcb, i, j, dv->cdi, dv->pdi));

	udi_assert(dv->txd_dmah != UDI_NULL_DMA_HANDLE);
	udi_assert(dv->pio_handles[SHRK_PIO_TX_POLL] != UDI_NULL_PIO_HANDLE);

	if (dv->nic_state == ACTIVE) {
		shrk_sked_xmit(dv);
	} else {
		DEBUG15(("shrk_nd_tx_req: not ACTIVE\n"));
	}
}

/*
 * shrkrx_nd_rx_rdy
 *	- secondary region
 *
 * receive packet response from NSR
 * used to pass rx cbs/bufs from NSR to ND
 */
void
shrkrx_nd_rx_rdy(udi_nic_rx_cb_t *rxcb)
{
	shrkrx_rdata_t *rxdv = UDI_GCB(rxcb)->context;
	udi_nic_rx_cb_t *lastcb;
	udi_ubit32_t j = 0;

#ifdef SHRK_DEBUG
	udi_ubit32_t i = 0;

	for (lastcb = rxcb; lastcb; lastcb = lastcb->chain)
		i++;
#endif /* SHRK_DEBUG */

	if (rxdv->rxrdy_cb != NULL) {
	    for (j = 1, lastcb = rxdv->rxrdy_cb; lastcb->chain != NULL;
							lastcb = lastcb->chain)
		j++;
	}

	DEBUG13(("shrkrx_nd_rx_rdy(cb %x) received %d holding %d cbs "
		"cdi %d pdi %d\n", rxcb, i, j, rxdv->cdi, rxdv->pdi));

	/* enqueue cbs at end of list */
	if (rxdv->rxrdy_cb == NULL) {
		rxdv->rxrdy_cb = rxcb;
	} else {
		lastcb->chain = rxcb;
	}

	shrkrx_sked_receive(rxdv);
}

/*
 * shrk_nd_ctrl_channel_event_ind
 *	-primary region
 *
 * handles NSR control channel events
 */
void
shrk_nd_ctrl_channel_event_ind(udi_channel_event_cb_t *cb)
{
	shrktx_rdata_t *dv = UDI_GCB(cb)->context;
	udi_channel_t chan = UDI_GCB(cb)->channel;

	DEBUG01(("shrk_nd_ctrl_channel_event_ind(cb %x) chan %x event %x\n", cb, chan, cb->event));

	switch (cb->event) {
	case UDI_CHANNEL_CLOSED:
		dv->ctl_chan = UDI_NULL_CHANNEL;
		/* TODO3 free resources, disable hardware */
		udi_channel_event_complete(cb, UDI_OK);
		udi_channel_close(chan);
		break;
	case UDI_CHANNEL_BOUND:      /* won't happen since NSR is initiator */
	case UDI_CHANNEL_OP_ABORTED: /* nsr ctrl channel ops not abortable */
	default:
		UDI_GCB(cb)->initiator_context = cb;
		udi_log_write(shrk_log_channel, UDI_GCB(cb), UDI_TREVENT_LOG,
			UDI_LOG_ERROR, SHRK_NIC_META, UDI_STAT_NOT_UNDERSTOOD,
			100, "shrk_nd_ctrl_channel_event_ind", cb->event);
		break;
	}
}

/*
 * shrk_nd_tx_channel_event_ind
 *	- primary region
 *
 * handles NSR transmit channel events
 */
void
shrk_nd_tx_channel_event_ind(udi_channel_event_cb_t *cb)
{
	shrktx_rdata_t *dv = UDI_GCB(cb)->context;
	udi_channel_t chan = UDI_GCB(cb)->channel;

	DEBUG01(("shrk_nd_tx_channel_event_ind(cb %x) chan %x event %x\n", cb, chan, cb->event));

	switch (cb->event) {
	case UDI_CHANNEL_CLOSED:
		dv->tx_chan = UDI_NULL_CHANNEL;
		/* TODO3 free resources, disable hardware */
		udi_channel_event_complete(cb, UDI_OK);
		udi_channel_close(chan);
		break;
	case UDI_CHANNEL_BOUND:      /* won't happen since NSR is initiator */
	case UDI_CHANNEL_OP_ABORTED: /* nsr tx channel ops not abortable */
	default:
		UDI_GCB(cb)->initiator_context = cb;
		udi_log_write(shrk_log_channel, UDI_GCB(cb), UDI_TREVENT_LOG,
			UDI_LOG_ERROR, SHRK_NIC_META, UDI_STAT_NOT_UNDERSTOOD,
			100, "shrk_nd_tx_channel_event_ind", cb->event);
		break;
	}
}

/*
 * shrkrx_nd_channel_event_ind
 *	- secondary region
 *
 * handles NSR receive channel events
 */
void
shrkrx_nd_channel_event_ind(udi_channel_event_cb_t *cb)
{
	shrkrx_rdata_t *rxdv = UDI_GCB(cb)->context;
	udi_channel_t chan = UDI_GCB(cb)->channel;

	DEBUG01(("shrkrx_nd_channel_event_ind(cb %x) chan %x event %x\n", cb, chan, cb->event));

	switch (cb->event) {
	case UDI_CHANNEL_CLOSED:
		rxdv->rx_chan = UDI_NULL_CHANNEL;
		/* TODO3 free resources, disable hardware */
		udi_channel_event_complete(cb, UDI_OK);
		udi_channel_close(chan);
		break;
	case UDI_CHANNEL_BOUND:      /* won't happen since NSR is initiator */
	case UDI_CHANNEL_OP_ABORTED: /* nsr rx channel ops not abortable */
	default:
		UDI_GCB(cb)->initiator_context = cb;
		udi_log_write(shrk_log_channel, UDI_GCB(cb), UDI_TREVENT_LOG,
			UDI_LOG_ERROR, SHRK_NIC_META, UDI_STAT_NOT_UNDERSTOOD,
			100, "shrkrx_nd_channel_event_ind", cb->event);
		break;
	}
}


/*
 *-----------------------------------------------------------------------------
 * Initialization happening during NIC meta bind request
 */

/*
 * shrk_tx_chan_spawned
 *	- primary region
 *
 * handles tx channel spawn callbacks
 *	gets tx channel spawned/anchored callback, spawns loose end rx channel
 *	gets rx channel spawned callback, passes loose end rx channel to
 *		secondary region over gio channel
 */
void
shrk_tx_chan_spawned(udi_cb_t *cb, udi_channel_t chan)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG01(("shrk_tx_chan_spawned(cb %x chan %x)\n", cb, chan));

	dv->tx_chan = chan;
	/* spawn receive channel, pass loose end to rx region later */
	udi_channel_spawn(shrk_rx_chan_spawned, cb,
		dv->ctl_chan, dv->rx_idx, 0, (void *)dv);
}

/*
 * shrk_rx_chan_spawned
 *	- primary region
 *
 *	gets rx channel spawned callback, passes loose end rx channel to
 *		secondary region over gio channel
 */
void
shrk_rx_chan_spawned(udi_cb_t *cb, udi_channel_t chan)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG01(("shrk_rx_chan_spawned(cb %x chan %x)\n", cb, chan));

	dv->rx_chan = chan;

#if 0 /* alloc_batch causes crash */
	/* allocate transmit cbs */
	udi_cb_alloc_batch(shrk_tx_cblist, cb, SHRK_NIC_TX_CB_IDX,
		dv->attr_values[SHRK_ATTR_TX_BUFFERS], 0, 0, UDI_NULL_BUF_PATH);
}
#else
	/* allocate transmit cbs */
	udi_cb_alloc(shrk_tx_cb, cb, SHRK_NIC_TX_CB_IDX, dv->tx_chan);
}

/*
 * shrk_tx_cb
 *	- primary region
 *
 * allocates transmit control blocks, maps tx_enable code when done
 */
void
shrk_tx_cb(udi_cb_t *cb, udi_cb_t *new_cb)
{
	shrktx_rdata_t *dv = cb->context;
	udi_nic_tx_cb_t *tx_cb = UDI_MCB(new_cb, udi_nic_tx_cb_t );
	udi_nic_tx_cb_t *tmpcb;
	udi_index_t i = 1; /* starts with one because first goes to dv */

	DEBUG01(("shrk_tx_cb(cb %x new_cb %x) dv %x flags %x\n", cb, new_cb, dv, dv->flags));

	tx_cb->chain = NULL;
	tx_cb->tx_buf = NULL;

	if (dv->txrdy_cb == NULL) {
		dv->txrdy_cb = tx_cb;
	} else {
		for (tmpcb = dv->txrdy_cb; tmpcb != NULL; tmpcb = tmpcb->chain) {
			i++;
			if (tmpcb->chain == NULL) {
				tmpcb->chain = tx_cb;
				break;
			}
		}
	}

	/* keep one free tx descriptor to simplify txd producer code */
	if (i < dv->attr_values[SHRK_ATTR_TX_BUFFERS]) {
		udi_cb_alloc(shrk_tx_cb, cb, SHRK_NIC_TX_CB_IDX, dv->tx_chan);
	} else {
		shrk_tx_cblist(cb, UDI_GCB(dv->txrdy_cb));
	}
}
#endif

/*
 * shrk_tx_cblist
 *	- primary region
 *
 * -- save cbs for enable, tell secondary about bind and pass channel
 */
void
shrk_tx_cblist(udi_cb_t *cb, udi_cb_t *new_cblist)
{
	shrktx_rdata_t *dv = cb->context;
	shrk_gio_event_params_t *params;

	DEBUG01(("shrk_tx_cblist(cb %x new_cblist %x) dv %x flags %x\n",
					cb, new_cblist, dv, dv->flags));

	dv->txrdy_cb = UDI_MCB(new_cblist, udi_nic_tx_cb_t);

	/* pass the newly spawned channel to the rx region to anchor */
	udi_assert(dv->gio_event_cb);
	dv->gio_event_cb->event_code = SHRK_GIO_NIC_BIND;
	params = (shrk_gio_event_params_t *)dv->gio_event_cb->event_params;
	params->rx_chan = dv->rx_chan;

	udi_gio_event_ind(dv->gio_event_cb);
	dv->gio_event_cb = NULL;

	/* initialize setup frame */
	shrk_build_setup_frame(dv, NULL);
}

/*
 * shrk_nd_bind_log
 *	- primary region
 *
 * called after log is written after an nd bind failure
 */
void
shrk_nd_bind_log(udi_cb_t *cb, udi_status_t correlated_status)
{
	udi_nic_bind_cb_t *bcb = UDI_MCB(cb, udi_nic_bind_cb_t);

	udi_nsr_bind_ack(bcb, correlated_status);
}

/*
 * shrk_nd_bind_ack
 *	- primary region
 *
 * issue udi_nsr_bind_ack to NSR
 */
void
shrk_nd_bind_ack(shrktx_rdata_t *dv, udi_status_t st)
{
	udi_nic_bind_cb_t *bcb;

	DEBUG01(("shrk_nd_bind_ack(dv %x st %x)\n", dv, st));

	bcb = dv->bind_cb; /* TODO3: use initiator_context */

	if (st != UDI_OK) {
		udi_log_write(shrk_nd_bind_log, UDI_GCB(bcb), UDI_TREVENT_LOG,
			UDI_LOG_ERROR, SHRK_NIC_META, st, 200,
			"shrk_nd_bind_ack");
		dv->bind_cb = NULL;
		return;
	}

	bcb->media_type = UDI_NIC_FASTETHER;
	if (shrk_link_parms[dv->link_index].flags & SHRK_MEDIA_10) {
		bcb->media_type = UDI_NIC_ETHER;
	}
	bcb->min_pdu_size = 0; /* default */
	bcb->max_pdu_size = SHRK_TXMAXSZ;
	bcb->rx_hw_threshold = dv->attr_values[SHRK_ATTR_RX_BUFFERS];
	bcb->capabilities = 0;
	bcb->max_perfect_multicast = SHRK_SET_MAXPERF;
	bcb->max_total_multicast = -1;

	bcb->mac_addr_len = SHRK_MACADDRSIZE;
	udi_memcpy(bcb->mac_addr, dv->hwaddr, SHRK_MACADDRSIZE);

	udi_nsr_bind_ack(bcb, UDI_OK);
	dv->nic_state = BOUND;
}

/*
 *-----------------------------------------------------------------------------
 * Initialization happening during NIC meta enable request
 */

/*
 * shrk_probe_cb
 *
 * allocated cb to use for pio_trans's during timeouts
 * return enable ack, continue thread with new cb
 */
void
shrk_probe_cb(udi_cb_t *cb, udi_cb_t *new_cb)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG01(("shrk_probe_cb(cb %x new_cb %x) dv %x\n", cb, new_cb, dv));

	dv->probe_cb = new_cb;

	udi_nsr_enable_ack(UDI_MCB(cb, udi_nic_cb_t), UDI_OK);

	/* notify secondary region of enable */
	udi_assert(dv->gio_event_cb);
	dv->gio_event_cb->event_code = SHRK_GIO_NIC_ENABLE;
	udi_gio_event_ind(dv->gio_event_cb);
	dv->gio_event_cb = NULL;

	dv->nic_state = ENABLED;

	/* pass transmit control blocks to NSR */
	udi_nsr_tx_rdy(dv->txrdy_cb);
	dv->txrdy_cb = NULL;

	udi_cb_alloc(shrk_setup_timer_cb, new_cb,
			SHRK_GCB_CB_IDX, UDI_NULL_CHANNEL);
}

/*
 * shrk_setup_timer_cb
 *
 */
void
shrk_setup_timer_cb(udi_cb_t *cb, udi_cb_t *new_cb)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG01(("shrk_setup_timer_cb(cb %x new_cb %x) dv %x\n", cb, new_cb, dv));

	dv->setup_cb = new_cb;

	udi_cb_alloc(shrk_timer_cb, new_cb,
			SHRK_GCB_CB_IDX, UDI_NULL_CHANNEL);
}


/*
 * shrk_timer_cb
 *
 * start link probe timer
 */
void
shrk_timer_cb(udi_cb_t *cb, udi_cb_t *new_cb)
{
	shrktx_rdata_t *dv = cb->context;
	udi_time_t time;

	DEBUG01(("shrk_timer_cb(cb %x new_cb %x) dv %x\n", cb, new_cb, dv));

	dv->timer_cb = new_cb;		/* save cb so we can cancel timer */

	time.seconds = SHRK_TIMER_INTERVAL;
	time.nanoseconds = 0;
	udi_timer_start_repeating(shrk_timer, new_cb, time);

	/* see if we have link already */
	dv->flags |= SHRK_ENABLING;
	if (dv->phy_status) {
		/* MII read of status register */
		/* this cb is a nic standard cb with scratch */
		*((udi_ubit32_t *)cb->scratch) =
			(SHRK_MII_READ << 10) | (dv->phy << 5) | SHRK_BMSR;
		udi_pio_trans(shrk_mii_link, dv->probe_cb,
		    dv->pio_handles[SHRK_PIO_MDIO_READ], 0, NULL, NULL);
	} else {
		udi_pio_trans(shrk_gpreg_link, dv->probe_cb,
			dv->pio_handles[SHRK_PIO_READ_GPREG], 0, NULL, NULL);
	}
}

/*
 * shrk_timer
 *
 * timer for link probe
 */
void
shrk_timer(void *context, udi_ubit32_t nmissed)
{
	shrktx_rdata_t *dv = context;

	DEBUG12(("shrk_timer(dv %x)\n", dv));

	dv->flags &= ~SHRK_ENABLING;

	if (dv->phy_status) {
		/* MII read of status register */
		*((udi_ubit32_t *)dv->probe_cb->scratch) =
			(SHRK_MII_READ << 10) | (dv->phy << 5) | SHRK_BMSR;
		udi_pio_trans(shrk_mii_link, dv->probe_cb,
		    dv->pio_handles[SHRK_PIO_MDIO_READ], 0, NULL, NULL);
	} else {
		udi_pio_trans(shrk_gpreg_link, dv->probe_cb,
			dv->pio_handles[SHRK_PIO_READ_GPREG], 0, NULL, NULL);
	}
}


/*
 *-----------------------------------------------------------------------------
 * Driver-Specific Functions
 */

/*
 * shrk_nic_reset
 *	- primary region
 *
 * reset hardware to current mode
 */
void
shrk_nic_reset(udi_cb_t *cb)
{
	shrktx_rdata_t *dv = cb->context;
	shrk_nic_reset_scratch_t *scratch =
			(shrk_nic_reset_scratch_t *)(cb->scratch);

	DEBUG01(("shrk_nic_reset(cb %x) dv %x link_index %d\n",
						cb, dv, dv->link_index));

	scratch->csr0 = dv->csr0;
	scratch->csr6 = shrk_link_parms[dv->link_index].csr6;

	/* don't use gpreg or pcs on 21143 */
	if (dv->attr_values[SHRK_ATTR_PCI_DEVICE] == SHRK_DEV_DEC_21143) {
		scratch->csr6 &= ~(OM_PCS|OM_SCR|OM_FD);
		scratch->csr6 |= OM_PS;
		scratch->gpreg = 0;
	} else {
		scratch->gpreg = shrk_link_parms[dv->link_index].gpreg;
	}

	shrk_clean_ring(dv);
	dv->cdi = dv->pdi = 0;

	udi_pio_trans(shrk_reset_complete, cb,
		dv->pio_handles[SHRK_PIO_NIC_RESET], 0, NULL, NULL);
}

/*
 * shrk_reset_complete
 *	- primary region
 *
 * nic is reset - callback for shrk_nic_reset_pio pio transaction
 * start timer to check link status if we're at gio bind time
 */
void
shrk_reset_complete(udi_cb_t *cb, udi_buf_t *buf, udi_status_t st, udi_ubit16_t res)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG01(("shrk_reset_complete(cb %x buf %x st %x res %x) dv %x flags %x\n", cb, buf, st, res, dv, dv->flags));

	if (res) {
		/* no device present */
		dv->flags &= ~SHRK_RUNNING;
	} else {
		dv->flags |= SHRK_RUNNING;
	}

	/* check if called from bus_bind_ack() */
	if (dv->nic_state == UNBOUND) {
		udi_gio_xfer_ack(UDI_MCB(cb, udi_gio_xfer_cb_t));
	}
}

/*
 * shrk_mii_link
 *	- primary region, mdio supported
 */
void
shrk_mii_link(udi_cb_t *cb, udi_buf_t *buf, udi_status_t st, udi_ubit16_t res)
{
	shrktx_rdata_t *dv = cb->context;

	dv->phy_status = *((udi_ubit32_t *)cb->scratch) & 0xffff;

	DEBUG12(("shrk_mii_link(cb %x buf %x st %x res %x) dv %x status %x\n",
					cb, buf, st, res, dv, dv->phy_status));

	if ((dv->phy_status & SHRK_BMSR_LINK) == 0) {
		if (dv->nic_state == ACTIVE) {
			DEBUG03(("shrk_mii_link: no link; status 0x%x\n",
							dv->phy_status));
			dv->nic_state = ENABLED;
			udi_cb_alloc(shrk_status_ind_cb, cb,
				SHRK_NIC_STATUS_CB_IDX, dv->ctl_chan);
			if (dv->flags & SHRK_ENABLING) {
				/* don't reset if timer hasn't fired */
				dv->flags &= ~SHRK_ENABLING;
			} else {
				/* shrk_new_link_index(dv); */
				shrk_nic_reset(cb);
			}
		}
	} else {
		if (dv->nic_state == ENABLED) {
			/* MII read of status register */
			*((udi_ubit32_t *)cb->scratch) = (SHRK_MII_READ << 10)
						| (dv->phy << 5) | SHRK_ANLPAR;
			udi_pio_trans(shrk_mii_anlpar, cb,
			    dv->pio_handles[SHRK_PIO_MDIO_READ], 0, NULL, NULL);
		}
	}
}

/*
 * shrk_mii_anlpar
 *	- primary region, mdio supported
 */
void
shrk_mii_anlpar(udi_cb_t *cb, udi_buf_t *buf, udi_status_t st, udi_ubit16_t res)
{
	shrktx_rdata_t *dv = cb->context;
	udi_ubit32_t anlpar = *((udi_ubit32_t *)cb->scratch) & 0xffff;
	shrk_txenab_scratch_t *scratch = cb->scratch;

	DEBUG12(("shrk_mii_anlpar(cb %x buf %x st %x res %x) dv %x anlpar %x\n",
					cb, buf, st, res, dv, anlpar));

	scratch->txpaddr = dv->txpaddr;
	scratch->csr6_bits = OM_ST;

	if ((anlpar & SHRK_ANLPAR_TX_FD) || (anlpar & SHRK_ANLPAR_TX)) {
		dv->flags |= SHRK_SPEED_100M;
	} else {
		dv->flags &= ~SHRK_SPEED_100M;
		scratch->csr6_bits |= OM_TTM;
	}

	if (((anlpar & SHRK_ANLPAR_TX_FD) || (anlpar & SHRK_ANLPAR_10_FD))
		&& (shrk_link_parms[dv->link_index].flags & SHRK_MEDIA_FDX)) {
		dv->flags |= SHRK_DUPLEX_FULL;
		scratch->csr6_bits |= OM_FD;
	} else {
		dv->flags &= ~SHRK_DUPLEX_FULL;
	}

	udi_pio_trans(shrk_txenab_pio, cb,
		dv->pio_handles[SHRK_PIO_TX_ENABLE],
					0, NULL, NULL);
}

/*
 * shrk_gpreg_link
 *	- primary region, mdio not supported
 *
 * - check result of GP reg poll
 * - send indication if warranted
 * - if link not detected, try new mode
 *
 * (this can be fooled half the time by unplugging 10M and plugging in 100M)
 */
void
shrk_gpreg_link(udi_cb_t *cb, udi_buf_t *buf, udi_status_t st, udi_ubit16_t res)
{
	shrktx_rdata_t *dv = cb->context;
	shrk_txenab_scratch_t *scratch = cb->scratch;

	DEBUG12(("shrk_gpreg_link(cb %x buf %x st %x res %x) dv %x flags %x\n", cb, buf, st, res, dv, dv->flags));

	switch (res) {
	case 0:
		DEBUG03(("shrk_gpreg_link: no link\n"));
		if (dv->nic_state == ACTIVE) {
			dv->nic_state = ENABLED;
			udi_cb_alloc(shrk_status_ind_cb, cb,
				SHRK_NIC_STATUS_CB_IDX, dv->ctl_chan);
		}
		if (dv->flags & SHRK_ENABLING) {
			/* don't reset if timer hasn't fired */
			dv->flags &= ~SHRK_ENABLING;
		} else {
			shrk_new_link_index(dv);
			shrk_nic_reset(cb);
		}
		return;
	case GP_OSI_LINK10M_L:
		dv->flags &= ~SHRK_SPEED_100M;
		if (shrk_link_parms[dv->link_index].flags & SHRK_MEDIA_FDX) {
			dv->flags |= SHRK_DUPLEX_FULL;
			DEBUG12(("shrk_gpreg_link: 10Mbps, FDX\n"));
		} else {
			dv->flags &= ~SHRK_DUPLEX_FULL;
			DEBUG12(("shrk_gpreg_link: 10Mbps, HDX\n"));
		}
		if (dv->nic_state == ENABLED) {
			scratch->txpaddr = dv->txpaddr;
			scratch->csr6_bits = OM_ST;
			udi_pio_trans(shrk_txenab_pio, cb,
				dv->pio_handles[SHRK_PIO_TX_ENABLE],
							0, NULL, NULL);
		}
		break;
	case GP_OSI_SIGDET:
		dv->flags |= SHRK_SPEED_100M;
		if (shrk_link_parms[dv->link_index].flags & SHRK_MEDIA_FDX) {
			dv->flags |= SHRK_DUPLEX_FULL;
			DEBUG12(("shrk_gpreg_link: 100Mbps, FDX\n"));
		} else {
			dv->flags &= ~SHRK_DUPLEX_FULL;
			DEBUG12(("shrk_gpreg_link: 100Mbps, HDX\n"));
		}
		if (dv->nic_state == ENABLED) {
			scratch->txpaddr = dv->txpaddr;
			scratch->csr6_bits = OM_ST;
			udi_pio_trans(shrk_txenab_pio, cb,
				dv->pio_handles[SHRK_PIO_TX_ENABLE],
							0, NULL, NULL);
		}
		break;
	default:
		udi_assert(0);
		break;
	}
}

/*
 * shrk_check_link_index
 *
 * check if a link parms table entry matches the custom params
 */
udi_boolean_t
shrk_check_link_index(shrktx_rdata_t *dv)
{
	if (!(shrk_link_parms[dv->link_index].flags
		& dv->attr_values[SHRK_ATTR_LINK_SPEED]))
			return FALSE;

	if (!(shrk_link_parms[dv->link_index].flags
		& dv->attr_values[SHRK_ATTR_DUPLEX_MODE]))
			return FALSE;

	return TRUE;
}

/*
 * shrk_new_link_index
 *
 * scan the link parms table for a new one matching the custom params
 */
void
shrk_new_link_index(shrktx_rdata_t *dv)
{
	udi_ubit16_t orig_link_index = dv->link_index;
	udi_ubit32_t link_type_count
		= sizeof(shrk_link_parms) / sizeof(shrk_link_parms_t);

	DEBUG12(("shrk_new_link_index(dv %x) orig_index %d\n",
							dv, dv->link_index));

	for (dv->link_index = SHRK_INC_MOD(dv->link_index, link_type_count);
	    dv->link_index != orig_link_index;
	    dv->link_index = SHRK_INC_MOD(dv->link_index, link_type_count)) {
		if (shrk_check_link_index(dv))
			break;
	}
}

/*
 * shrk_txenab_pio
 *	- primary region
 *
 * transmit enabled pio transaction completed callback
 */
void
shrk_txenab_pio(udi_cb_t *cb, udi_buf_t *buf, udi_status_t st, udi_ubit16_t res)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG01(("shrk_txenab_pio(cb %x buf %x st %x res %x) dv %x\n", cb, buf, st, res, dv));

	shrk_sked_xmit(dv);		   /* send any waiting packets */

	shrk_send_setup_frame(cb, TX_IC);  /* this thread enables receive */
}

/*
 * shrk_build_setup_frame
 *	- primary region
 *
 * build setup frame
 * -- if ccb is non-null, it contains a multicast filter request
 */
void
shrk_build_setup_frame(shrktx_rdata_t *dv, udi_nic_ctrl_cb_t *ccb)
{
	udi_ubit32_t cnt, i;
	shrkaddr_t *fill;
	shrkaddr_t mca;

	DEBUG01(("shrk_build_setup_frame(dv %x ccb %x)\n", dv, ccb));

	cnt = SHRK_SET_MINADDR;  /* two addrs: broadcast and unicast */
	if (ccb != NULL) {
		switch (ccb->command) {
		case UDI_NIC_ADD_MULTI:
		case UDI_NIC_ALLMULTI_OFF:
		case UDI_NIC_DEL_MULTI:
			cnt += SHRK_MCA_CNT(ccb);
			break;
		case UDI_NIC_SET_CURR_MAC:
			udi_assert(ccb->indicator == SHRK_MACADDRSIZE);
			udi_buf_read(ccb->data_buf, 0, SHRK_MACADDRSIZE,
				&dv->hwaddr);
			break;
		default:
			udi_assert(0);
		}
	}

	/*
	 * Use perfect address filtering if 16 addresses or less (including
	 * unicast MAC and broadcast addresses) are being set.  A setup frame
	 * is exactly 192 bytes and the format is as follows for ethernet
	 * address 00:01:02:03:04:05 where XX is don't care.  Any valid
	 * address can be used to fill out the frame:
	 *   XXXXFFFF
	 *   XXXXFFFF
	 *   XXXXFFFF
	 *   XXXX0100
	 *   XXXX0302
	 *   XXXX0504
	 *   ...last 3 lines repeated until end of setup frame...
	 */
	if (cnt <= SHRK_SET_MAXPERF) {
		DEBUG15(("shrk_build_setup_frame: perfect filtering cnt %d\n",
			cnt));

		fill = (shrkaddr_t *)dv->setup_mem;

		/* clear imperfect filtering flag */
		dv->flags &= ~SHRK_MCA_HASH;

		/* add broadcast address */
		shrk_set_perfaddr(dv, (udi_ubit8_t *)fill, shrk_broad);
		fill += 2;  /* skip 6 unused bytes per address */

		/* add unicast mac address */
		shrk_set_perfaddr(dv, (udi_ubit8_t *)fill, dv->hwaddr);
		fill += 2;  /* skip 6 unused bytes per address */

		/* handle multicast addresses */
		for (i = SHRK_SET_MINADDR; i < cnt; i++) {
			udi_assert(ccb);
			udi_buf_read(ccb->data_buf,
				(ccb->indicator + i - SHRK_SET_MINADDR)
				* SHRK_MACADDRSIZE, SHRK_MACADDRSIZE, mca);
			DEBUG15(("mca off %d: %02x:%02x:%02x:%02x:%02x:%02x\n",
				(ccb->indicator + i - SHRK_SET_MINADDR),
				mca[0], mca[1], mca[2], mca[3], mca[4],
				mca[5]));
			shrk_set_perfaddr(dv, (udi_ubit8_t *)fill, mca);
			fill += 2;  /* skip 6 unused bytes per address */
		}

		/* fill out setup frame with the unicast mac address */
		while (cnt++ < SHRK_SET_MAXPERF) {
			shrk_set_perfaddr(dv, (udi_ubit8_t *)fill, dv->hwaddr);
			fill += 2;  /* skip 6 unused bytes per address */
		}
	} else {
		DEBUG02(("shrk_build_setup_frame: imperfect filtering\n"));
		dv->flags |= SHRK_MCA_HASH;

		/* TODO2 handle imperfect multicasting */

		return;
	}
}

#define CRC32POLY        0xEDB88320UL  /* CRC-32 Poly - Little Endian */
#define SHRK_HASH_BITS   9             /* Number of bits in hash */

/*
 * shrk_crc32_mchash
 *	- primary region
 *
 * hash mac address for imperfect multicast addressing, return result
 */
udi_ubit32_t
shrk_crc32_mchash(udi_ubit8_t *mca)
{
	udi_ubit32_t idx, bit, data, crc=0xffffffffUL;

	DEBUG01(("shrk_crc32_mchash(mca %08x)\n", mca));

	for (idx = 0; idx < 6; idx++)
		for (data = *mca++, bit = 0; bit < 8; bit++, data >>= 1)
			crc = (crc >> 1) ^ (((crc ^ data) & 1) ? CRC32POLY : 0);

	return(crc & ((1 << SHRK_HASH_BITS) - 1));  /* low bits for hash */
}

/*
 * shrk_set_perfaddr
 *	- primary region
 *
 * write mac address into setup frame buffer, represented by *s
 */
void
shrk_set_perfaddr(shrktx_rdata_t *dv, udi_ubit8_t *s, const shrkaddr_t addr)
{
	udi_ubit32_t i;

	DEBUG16(("shrk_perfaddr(s %x addr %x %02x:%02x:%02x:%02x:%02x:%02x) setup_swap %x\n",
		s, addr, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
		dv->setup_swap));

	for (i = 0; i < sizeof(shrkaddr_t); i += 2) {
		/*
		 * we don't have to do endianness swapping since 
		 * the setup_frame is an array of bytes
		 */
		*s++ = addr[i];
		*s++ = addr[i + 1];
#ifdef SHRK_DEBUG
		*s++ = 0xAA;
		*s++ = 0xAA;
#else
		s += 2;
#endif /* SHRK_DEBUG */
	}
}

/*
 * shrk_send_setup
 *
 * send previously created setup frame
 */
void
shrk_send_setup_frame(udi_cb_t *cb, udi_ubit32_t txflags)
{
	shrktx_rdata_t *dv = cb->context;
	txd_t *txd, *stxd;

	DEBUG01(("shrk_send_setup_frame(cb %x txflags %x) cdi %d pdi %d\n",
		cb, txflags, dv->cdi, dv->pdi));

	if ((dv->max_desc - DSC_NAVAIL(dv)) >= dv->tx_cleanup) {
		shrk_txcomplete(dv);  /* free up some transmit descriptors */
	}
	if (DSC_NAVAIL(dv) < SHRK_SET_NUMTXDS) {
		DEBUG02(("shrk_setup_frame: no descriptors (cdi %d pdi %d)\n",
							dv->cdi, dv->pdi));
		/* TODO3 log this failure */
		return;
	}

	/* save pointer to setup descriptor index */
	dv->setup_cdi = dv->cdi;

	/* skip 1 transmit descriptor; ensures correct setup frame processing */
	stxd = DSC_CONS(dv);
	stxd->cntrl &= SHRK_SWAP_DESC(dv, TX_TER);
	stxd->cntrl |= SHRK_SWAP_DESC(dv, TX_SET);
	stxd->paddr1 = stxd->paddr2 = 0;
	stxd->tx_cb = NULL;
	stxd->tx_sdmah = NULL;

	/* increment txdesc consumer index past skipped transmit descriptor */
	dv->cdi = DSC_NEXT_CDI(dv);

	/* initialize setup transmit descriptor */
	txd = DSC_CONS(dv);
	DEBUG15(("shrk_setup_frame: txd %x paddr %x\n", txd, dv->setup_paddr));

	if (dv->flags & SHRK_MCA_HASH) {
		/* hash filtering plus one perfect address */
		txflags |= TX_FT0;
	}
	txd->cntrl &= SHRK_SWAP_DESC(dv, TX_TER);
	txd->cntrl |= SHRK_SWAP_DESC(dv, (txflags | TX_SET | SHRK_SET_SZ));
	txd->paddr1 = SHRK_SWAP_DESC(dv, dv->setup_paddr);
	txd->paddr2 = 0;
	txd->tx_cb = NULL;
	txd->tx_sdmah = NULL;

	txd->status = SHRK_SWAP_DESC(dv, TX_OWN);
	/* set first OWN after second */
	stxd->status = SHRK_SWAP_DESC(dv, TX_OWN);

	/* increment txdesc consumer index past setup frame */
	dv->cdi = DSC_NEXT_CDI(dv);

#ifdef SHRK_DEBUG
	shrktxd(dv);
#endif /* SHRK_DEBUG */

	/* set backup timer for interrupt */
	if (!(dv->flags & SHRK_SETUP)) {
		udi_time_t time;

		time.seconds = SHRK_SETUP_TIMEOUT;
		time.nanoseconds = 0;
		udi_timer_start(shrk_setup_timer, dv->setup_cb, time);
		dv->flags |= SHRK_SETUP;
	} else {
		/* timer already running */
	}

	/* issue transmit poll to NIC */
	udi_pio_trans(shrk_setup_poll_done, cb,
		dv->pio_handles[SHRK_PIO_TX_POLL], 0, NULL, NULL);
}

/*
 * shrk_setup_poll_done
 *
 * no-op; waiting for either tx interrupt or timer fire
 */
void
shrk_setup_poll_done(udi_cb_t *cb, udi_buf_t *buf, udi_status_t st, udi_ubit16_t r)
{
	DEBUG_VAR(shrktx_rdata_t *dv = cb->context;)
	DEBUG01(("shrk_setup_poll_done(cb %x buf %x st %x res %x) dv %x\n",
		cb, buf, st, r, dv));
}

/*
 * shrk_setup_timer
 *
 * timeout waiting for tx complete interrupt for setup frame
 * -- check transmit ring
 */
void
shrk_setup_timer(udi_cb_t *cb)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG01(("shrk_setup_timer(cb %x) setup_cdi %d\n", cb, dv->setup_cdi));

	dv->flags &= ~SHRK_SETUP;

	shrk_txcomplete(dv);
}
/*
 * shrk_sked_xmit
 *
 * start sending any waiting packets
 */
void
shrk_sked_xmit(shrktx_rdata_t *dv)
{
	udi_nic_tx_cb_t *txcb;
	shrk_dma_scratch_t *scratch;

	DEBUG01(("shrk_sked_xmit(dv %x) state %d navail %d tx_cb %x sdmah %x\n",
		dv, dv->nic_state, DSC_NAVAIL(dv), dv->tx_cb, dv->next_sdmah));

	if (dv->tx_cb == NULL || dv->next_sdmah == NULL
			|| DSC_NAVAIL(dv) == 0 || dv->nic_state != ACTIVE)
		return;

	/* get first cb */
	txcb = dv->tx_cb;
	dv->tx_cb = txcb->chain;
	txcb->chain = NULL;

	/* store dma handle in scratch */
	scratch = UDI_GCB(txcb)->scratch;
	scratch->sdmah = dv->next_sdmah;
	dv->next_sdmah = scratch->sdmah->next;
	scratch->sdmah->next = NULL;

	/* map first data buffer */
	udi_dma_buf_map(shrk_tx_buf_map, UDI_GCB(txcb),
		scratch->sdmah->dmah, txcb->tx_buf, 0,
		txcb->tx_buf->buf_size, UDI_DMA_OUT);
}

/*
 * shrk_tx_buf_map
 *	- primary region
 *
 * transmit buffer mapped for dma callback
 */
void
shrk_tx_buf_map(udi_cb_t *cb, udi_scgth_t *scgth, udi_boolean_t done, udi_status_t st)
{
	shrktx_rdata_t *dv = cb->context;
	txd_t *txd, *first_txd;
	udi_index_t i;
	udi_ubit32_t num_el;
	udi_nic_tx_cb_t *txcb = UDI_MCB(cb, udi_nic_tx_cb_t);
	shrk_dma_scratch_t *scratch = cb->scratch;

 	DEBUG15(("shrk_tx_buf_map(cb %x scgth %x done %x st %x) urg %d\n",
 		cb, scgth, done, st, txcb->completion_urgent));
 	DEBUG15(("    buf %x dmah %x cdi %d pdi %d\n",
 		txcb->tx_buf, scratch->sdmah, dv->cdi, dv->pdi));

	udi_assert(done);	/* must be full mapping of packet */
	txcb->tx_buf = NULL;	/* buffer is now held by dma handle */

	num_el = scgth->scgth_num_elements;
	if ((dv->max_desc - DSC_NAVAIL(dv)) >= dv->tx_cleanup) {
		shrk_txcomplete(dv);  /* free up some transmit descriptors */
	}
	if (DSC_NAVAIL(dv) < ((num_el + 1) / 2)) {
		DEBUG02(("shrk_tx_buf_map: navail FALSE\n"));

		/* no transmit descriptors available: requeue cb */
		txcb->tx_buf = udi_dma_buf_unmap(scratch->sdmah->dmah,
							txcb->tx_buf->buf_size);
		udi_assert(txcb->chain == NULL);
		txcb->chain = dv->tx_cb;
		dv->tx_cb = txcb;

		/* return dma handle to pool */
		scratch->sdmah->next = dv->next_sdmah;
		dv->next_sdmah = scratch->sdmah;
		scratch->sdmah = NULL;

		return;
	}

	for (i = 0; i < num_el; i++) {
		txd = DSC_CONS(dv);
		dv->cdi = DSC_NEXT_CDI(dv);
		/* clear everything but Transmit End of Ring */ 
		txd->cntrl &= SHRK_SWAP_DESC(dv, TX_TER);
		if (i == 0) {
			/* set First Segment */
			txd->cntrl |= SHRK_SWAP_DESC(dv, TX_FS);
			/* don't give frame to the 21140 yet; set OWN later  */
			txd->status = 0;
			first_txd = txd;
		} else {
			txd->status = SHRK_SWAP_DESC(dv, TX_OWN);
		}
		txd->cntrl |= SHRK_SWAP_DESC(dv, SHRK_SWAP_SCGTH(scgth,
			scgth->scgth_elements.el32p[i].block_length));
		txd->paddr1 = SHRK_SWAP_DESC(dv, SHRK_SWAP_SCGTH(scgth,
			scgth->scgth_elements.el32p[i].block_busaddr));

		if (++i < num_el) {
			txd->cntrl |= SHRK_SWAP_DESC(dv, ((SHRK_SWAP_SCGTH(scgth,
				scgth->scgth_elements.el32p[i].block_length))
							<< TX_TBS2_BIT_POS));
			txd->paddr2 = SHRK_SWAP_DESC(dv, SHRK_SWAP_SCGTH(scgth,
				scgth->scgth_elements.el32p[i].block_busaddr));
		} else {
			txd->paddr2 = 0;
		}
		txd->tx_cb = NULL;
		txd->tx_sdmah = NULL;
	}
	txd->tx_cb = txcb;
	txd->tx_sdmah = scratch->sdmah;
	txd->cntrl |= SHRK_SWAP_DESC(dv, TX_LS);
	if (txcb->completion_urgent) {
		txd->cntrl |= SHRK_SWAP_DESC(dv, TX_IC);
	}
	/* set interrupt periodically when over hiwater */
	if ( (dv->cdi & dv->tx_freemask) == 0
		&& (dv->max_desc - DSC_NAVAIL(dv)) >= dv->tx_cleanup) {
			txd->cntrl |= SHRK_SWAP_DESC(dv, TX_IC);
	}

	/* TODO3 udi_dma_sync before give buffer to NIC */
	/* give frame to the 21140 */
	first_txd->status = SHRK_SWAP_DESC(dv, TX_OWN);

	dv->stats.tx_packets++;

	udi_dma_sync(shrk_tx, cb, txd->tx_sdmah->dmah, 0, 0, UDI_DMA_OUT);
}


/*
 * shrk_tx
 *	- primary region
 *
 * transmit descriptor list sync'd, issue transmit poll to nic
 */
void
shrk_tx(udi_cb_t *cb)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG15(("shrk_tx(cb %x) dv %x\n", cb, dv));

	/* issue transmit poll to NIC */
	udi_pio_trans(shrk_tx_done, cb,
		dv->pio_handles[SHRK_PIO_TX_POLL], 0, NULL, NULL);
}

/*
 * shrk_tx_done
 *	- primary region
 *
 * NIC transmit poll pio transaction completed
 */
void
shrk_tx_done(udi_cb_t *cb, udi_buf_t *buf, udi_status_t st, udi_ubit16_t res)
{
	shrktx_rdata_t *dv = cb->context;

	DEBUG15(("shrk_tx_done(cb %x buf %x st %x res %x) tx_cb %x\n",
						cb, buf, st, res, dv->tx_cb));

	shrk_sked_xmit(dv);
}

/*
 * shrk_txcomplete
 *	- primary region
 *
 * clean up after frame(s) have been transmited
 * producer of free transmit descriptors
 */
void
shrk_txcomplete(shrktx_rdata_t *dv)
{
	txd_t *txd;
	udi_nic_tx_cb_t *ncb, *lcb;   /* NSR transmit control block list */

	DEBUG15(("shrk_txcomplete(%x) cdi %d pdi %d\n", dv, dv->cdi, dv->pdi));
#ifdef SHRK_DEBUG
	shrktxd(dv);
#endif /* SHRK_DEBUG */

	if (dv->desc == NULL)
		return;

	ncb = lcb = NULL;
	
	/* loop below relies on the hardware to let us know which are done */
	txd = DSC_PROD(dv);
	while (!(txd->status & SHRK_SWAP_DESC(dv, TX_OWN))
					&& ! DSC_ALLCLEAN(dv)) {
		/* transmit descriptor belongs to the host */ 
		DEBUG15(("shrk_txcomplete pdi %d status %08x cntrl %08x\n",
				dv->pdi, txd->status, txd->cntrl));

		if (txd->cntrl & SHRK_SWAP_DESC(dv, TX_LS)) {
			shrk_tx_free(dv, txd, &ncb, &lcb);
		}
		if (txd->cntrl & SHRK_SWAP_DESC(dv, TX_SET)) {
			shrk_setup_complete(dv, txd);
		}
		txd->paddr1 = 0;
		txd->paddr2 = 0;
		/* preserve Transmit End of Ring */
		txd->cntrl &= SHRK_SWAP_DESC(dv, TX_TER);

		dv->pdi = DSC_NEXT_PDI(dv);
		txd = DSC_PROD(dv);
	}

	if (ncb != NULL) {
		if (dv->nic_state == ENABLED || dv->nic_state == ACTIVE) {
			udi_nsr_tx_rdy(ncb);
		} else {
			lcb->chain = dv->txrdy_cb;
			dv->txrdy_cb = ncb;
		}
	}

#ifdef SHRK_DEBUG
	shrktxd(dv);
#endif /* SHRK_DEBUG */

	shrk_sked_xmit(dv);	/* check for waiting packets */
}

void
shrk_setup_complete(shrktx_rdata_t *dv, txd_t *txd)
{
	DEBUG15(("shrk_setup_complete(txd %x)\n", txd));

	if ((txd->paddr1 != 0) && (txd->cntrl & SHRK_SWAP_DESC(dv, TX_IC))) {
		/* IC == initialization */
		if (dv->gio_event_cb) {
			dv->gio_event_cb->event_code = SHRK_GIO_NIC_LINK_UP;
			udi_gio_event_ind(dv->gio_event_cb);
			dv->gio_event_cb = NULL;
		}
	}
}

/* free buffer and give tx cb to NSR */
void
shrk_tx_free(shrktx_rdata_t *dv, txd_t *txd,
			udi_nic_tx_cb_t **ncb, udi_nic_tx_cb_t **lcb)
{
	udi_nic_info_cb_t *nst = &dv->stats;       /* NIC statistics */
	udi_ubit32_t status;

	DEBUG15(("shrk_tx_free cb %x sdmah %x\n", txd->tx_cb, txd->tx_sdmah));

	udi_assert(txd->tx_cb != NULL);
	udi_assert(txd->tx_sdmah != NULL);

	udi_buf_free(udi_dma_buf_unmap(txd->tx_sdmah->dmah, 0));
	txd->tx_cb->tx_buf = NULL;

	/* return dma handle to pool */
	txd->tx_sdmah->next = dv->next_sdmah;
	dv->next_sdmah = txd->tx_sdmah;
	txd->tx_sdmah = NULL;

	if (*ncb == NULL) {
		(*ncb) = (*lcb) = txd->tx_cb;
	} else {
		(*lcb)->chain = txd->tx_cb;
		(*lcb) = (*lcb)->chain;
	}
	txd->tx_cb = NULL;

	/* gather transmit statistics */
	status = SHRK_SWAP_DESC(dv, txd->status);
	if (status & TX_CC) {
		nst->collisions += (status & TX_CC) >> TX_CC_BIT_POS;
	}
	if (status & TX_LF) {
		DEBUG15(("shrk_tx_free: TX_LF (link failed)\n"));
		/* TODO2 udi_nsr_status_ind, LINK_DOWN */
	}
	if (status & TX_ES) {
		nst->tx_errors++;
		if (status & TX_TO) {
			DEBUG15(("shrk_tx_free: TX_TO (transmit jabber)\n"));
		}
		if ((status & TX_NC) || (status & TX_LO)) {
			/* no carrier */
			/* TODO2 udi_nsr_status_ind, LINK_DOWN */
			DEBUG15(("shrk_tx_free: TX_NC (no carrier)\n"));
		}
		if (status & TX_LC) {
			DEBUG15(("shrk_tx_free: TX_LC (late collision)\n"));
			nst->collisions++;
		}
		if (status & TX_EC) {
			DEBUG15(("shrk_tx_free: TX_EC (excessive collisions)\n"));
			nst->collisions += 16;
		}
		if (status & TX_UF) {
			DEBUG15(("shrk_tx_free: TX_UF (transmit underflow)\n"));
			nst->tx_underrun++;
		}
	}
}

/*
 * shrk_clean_ring
 *
 * remove all cbs from ring, freeing buffers
 * leave cb's on dv->txrdy_cb
 */
void
shrk_clean_ring(shrktx_rdata_t *dv)
{
	udi_nic_tx_cb_t *txcb;
	txd_t *txd;
	udi_ubit32_t i = 0, j = 0, k = 0;

	if (dv->txrdy_cb != NULL) {
	    i = 1;
	    for (txcb = dv->txrdy_cb; txcb->chain != NULL; txcb = txcb->chain)
		i++;
	}

	while (! DSC_ALLCLEAN(dv)) {
		txd = DSC_PROD(dv);

		txd->paddr1 = txd->paddr2 = 0;

		if ( ! (txd->cntrl & SHRK_SWAP_DESC(dv, TX_LS))) {
			udi_assert(txd->tx_cb == NULL);
			udi_assert(txd->tx_sdmah == NULL);
			dv->cdi = DSC_NEXT_CDI(dv);
			continue;
		}
		udi_assert(txd->tx_cb != NULL);
		udi_assert(txd->tx_sdmah != NULL);

		if (dv->txrdy_cb == NULL) {
			dv->txrdy_cb = txcb = txd->tx_cb;
		} else {
			txcb->chain = txd->tx_cb;
			txcb = txcb->chain;
		}
		txcb->chain = NULL;

		udi_assert(txcb->tx_buf == NULL);
		udi_buf_free(udi_dma_buf_unmap(txd->tx_sdmah->dmah, 0));

		/* return dma handle to pool */
		txd->tx_sdmah->next = dv->next_sdmah;
		dv->next_sdmah = txd->tx_sdmah;
		txd->tx_sdmah = NULL;

		txd->status = 0;
		txd->cntrl &= SHRK_SWAP_DESC(dv, TX_TER);
		txd->tx_cb = 0;

		dv->cdi = DSC_NEXT_PDI(dv);
		j++;
	}

	/* free up waiting transmit requests */
	while (dv->tx_cb != NULL) {
		if (dv->txrdy_cb == NULL) {
			dv->txrdy_cb = txcb = dv->tx_cb;
		} else {
			txcb->chain = dv->tx_cb;
			txcb = txcb->chain;
		}
		dv->tx_cb = txcb->chain;
		txcb->chain = NULL;

		udi_buf_free(txcb->tx_buf);
		txcb->tx_buf = NULL;
		k++;
	}

	DEBUG01(("shrk_clean_ring(dv %x) txrdy %d ring %d waiting %d\n",
						dv, i, j, k));
}

/*
 * shrkrx_sked_receive
 *
 * map buffers back onto receive ring
 */
void
shrkrx_sked_receive(shrkrx_rdata_t *rxdv)
{
	shrk_dma_scratch_t *scratch;
	udi_nic_rx_cb_t *rxcb;

	DEBUG13(("shrkrx_sked_receive(rxdv %x) rxrdy_cb %x next_sdmah %x\n",
				rxdv, rxdv->rxrdy_cb, rxdv->next_sdmah));

	/* don't need to explicity check ring because dmah's are 1-for-1 */
 	if (rxdv->rxrdy_cb == NULL || rxdv->next_sdmah == NULL)
			return;

	rxcb = rxdv->rxrdy_cb;
	rxdv->rxrdy_cb = rxcb->chain;
	rxcb->chain = NULL;

	udi_assert(rxcb->rx_buf != NULL);

	/* store dma handle in scratch */
	scratch = (shrk_dma_scratch_t *)UDI_GCB(rxcb)->scratch;
	scratch->sdmah = rxdv->next_sdmah;
	rxdv->next_sdmah = scratch->sdmah->next;
	scratch->sdmah->next = NULL;

	/* map receive data buffer */
	udi_dma_buf_map(shrkrx_rx_buf_map, UDI_GCB(rxcb),
		scratch->sdmah->dmah, rxcb->rx_buf, 0,
		SHRK_RXMAXSZ, UDI_DMA_IN);
}

/*
 * shrkrx_rx_buf_map
 *	- secondary region
 *
 * producer of receive descriptors with dmaable memory mapped buffers
 */
void
shrkrx_rx_buf_map(udi_cb_t *cb, udi_scgth_t *scgth, udi_boolean_t done, udi_status_t st)
{
	shrkrx_rdata_t *rxdv = cb->context;
	shrk_dma_scratch_t *scratch = cb->scratch;
	udi_nic_rx_cb_t *rxcb = UDI_MCB(cb, udi_nic_rx_cb_t);
	rxd_t *rxd;

 	DEBUG13(("shrkrx_rx_buf_map(cb %x scgth %x done %x st %x)\n",
 		cb, scgth, done, st));
 	DEBUG13(("    shrkrx_rx_buf_map: buf %x sdmah %x cdi %d pdi %d\n",
 		rxcb->rx_buf, scratch->sdmah, rxdv->cdi, rxdv->pdi));
	udi_assert(done);

#ifdef SHRK_DEBUG
 	if ( ! (DSC_ALLCLEAN(rxdv) || DSC_NAVAIL(rxdv) < rxdv->max_desc - 2)) {
		udi_assert(0);	/* should never happen due to limited dmah's */
	}
#endif

	rxcb->rx_buf = NULL;	/* buffer now held by dma handle */

	/* setup next available receive descriptor */
	rxd = DSC_PROD(rxdv);
	rxdv->pdi = DSC_NEXT_PDI(rxdv);
	/* preserve end of ring */
	rxd->cntrl &= SHRK_SWAP_DESC(rxdv, RX_RER);
	rxd->cntrl |= SHRK_SWAP_DESC(rxdv, SHRK_MAXPACK);
	rxd->paddr1 = SHRK_SWAP_DESC(rxdv, SHRK_SWAP_SCGTH(scgth,
		scgth->scgth_elements.el32p[0].block_busaddr));
	rxd->paddr2 = 0;
	rxd->rx_cb = rxcb;
	rxd->rx_sdmah = scratch->sdmah;

	/* TODO3 udi_dma_sync before give buffer to NIC */
	/* buffer belongs to 21140A */
	rxd->status = SHRK_SWAP_DESC(rxdv, RX_OWN);
	/* TODO3 udi_dma_sync after give buffer to NIC */

	shrkrx_sked_receive(rxdv);
}

/*
 * shrkrx_check_packet
 *
 * check to see if packet is valid, update stats, and return status
 */
udi_boolean_t
shrkrx_check_packet(udi_ubit32_t shrk_status,
			udi_ubit8_t *nic_status, shrkrx_rdata_t *rxdv)
{
	rxdv->rx_packets++;

	if ((shrk_status & RX_ES) || !(shrk_status & RX_LS)) {
		shrkrx_log(rxdv, UDI_STAT_DATA_ERROR, 400,
			shrk_status & 0xffff);
		rxdv->rx_errors++;
	}
	*nic_status = 0;
	return TRUE;
}

/*
 * shrkrx_rx
 *	- secondary region
 *
 * process received frames, pass them to NSR over rx channel
 * the hardware (represented by shrkrx_rx) is the consumer of rx descriptors
 *
 * there are no asyncronous service calls in shrkrx_rx
 */
void
shrkrx_rx(shrkrx_rdata_t *rxdv)
{
	udi_nic_rx_cb_t *rxcb;
	rxd_t *rxd;
	udi_ubit32_t status, size;
#ifdef SHRK_DEBUG
	udi_ubit8_t rx_debug_buf[60];
	udi_ubit32_t i;
	
	DEBUG14(("shrkrx_rx:\n"));
	shrkrxd(rxdv);
#endif

	if (rxdv->rxcb != NULL) {
	    for (rxcb = rxdv->rxcb; rxcb->chain != NULL; rxcb = rxcb->chain)
		;
	}

	rxd = DSC_CONS(rxdv);
	while (!(rxd->status & SHRK_SWAP_DESC(rxdv, RX_OWN))
						&& ! DSC_ALLCLEAN(rxdv)) {
		/* receive complete - driver owns this descriptor */

		udi_assert(rxd->rx_cb != NULL);

		DEBUG13(("shrkrx_rx: rxd %x cdi %d pdi %d status %x rx_cb %x\n",
			rxd, rxdv->cdi, rxdv->pdi, rxd->status, rxd->rx_cb));

		if (rxdv->rxcb == NULL) {
			rxdv->rxcb = rxcb = rxd->rx_cb;
		} else {
			rxcb->chain = rxd->rx_cb;
			rxcb = rxcb->chain;
		}
		rxcb->chain = NULL;

		status = SHRK_SWAP_DESC(rxdv, rxd->status);
		if (shrkrx_check_packet(status, &(rxcb->rx_status), rxdv)) {
			/* data length doesn't include the CRC */
			size = ((status & RX_FL) >> RX_FL_BIT_POS) - RX_CRC;
		} else {
			size = 0;
		}
		rxcb->rx_buf = udi_dma_buf_unmap(rxd->rx_sdmah->dmah, size);

		/* return dma handle to pool */
		rxd->rx_sdmah->next = rxdv->next_sdmah;
		rxdv->next_sdmah = rxd->rx_sdmah;
		rxd->rx_sdmah = NULL;

		if (status & RX_MF) {
			/* could be either broadcast or multicast */
			rxcb->addr_match = UDI_NIC_RX_UNKNOWN;
		} else {
			/* unicast is an exact match */
			rxcb->addr_match = UDI_NIC_RX_EXACT;
		}
		rxcb->rx_valid = 0;	/* no checksum validation */

		rxd->rx_cb = NULL;
		rxd->paddr1 = NULL;

		rxdv->cdi = DSC_NEXT_CDI(rxdv);
		rxd = DSC_CONS(rxdv);
	}

	if (rxdv->rxcb != NULL) {
#if 0 && defined(SHRK_DEBUG)
		udi_buf_read(rxdv->rxcb->rx_buf, 0, sizeof(rx_debug_buf),
			rx_debug_buf);
		DEBUG09(("shrkrx_rx buf: <"));
		for (i = 0; i < sizeof(rx_debug_buf); i++) {
			DEBUG09(("%02x", rx_debug_buf[i]));
		}
		DEBUG09((">\n"));
#endif
		if (rxdv->nic_state == ACTIVE) {
			udi_nsr_rx_ind(rxdv->rxcb);
			rxdv->rxcb = NULL;
		} else {
			DEBUG02(("shrkrx_rx: not ACTIVE\n")); /* log? */
		}
	}

	shrkrx_sked_receive(rxdv);

#ifdef SHRK_DEBUG
	shrkrxd(rxdv);
#endif /* SHRK_DEBUG */
}

/*
 * shrkrx_clean_ring
 *
 * remove all cbs from ring, setting buffers to 0 size
 * pass all cbs (with buffers) back up to NSR
 */
void
shrkrx_clean_ring(shrkrx_rdata_t *rxdv)
{
	udi_nic_rx_cb_t *rxcb;
	rxd_t *rxd;
	udi_ubit32_t i = 0, j = 0;

	if (rxdv->rxrdy_cb != NULL) {
		i = 1;
		for (rxcb = rxdv->rxrdy_cb; rxcb->chain != NULL;
							rxcb = rxcb->chain) {
			rxcb->rx_buf->buf_size = 0;
			i++;
		}
		rxcb->rx_buf->buf_size = 0;
	}

	while (! DSC_ALLCLEAN(rxdv)) {
		rxd = DSC_CONS(rxdv);

		udi_assert(rxd->rx_cb != NULL);
		udi_assert(rxd->rx_sdmah != NULL);

		if (rxdv->rxrdy_cb == NULL) {
			rxdv->rxrdy_cb = rxcb = rxd->rx_cb;
		} else {
			rxcb->chain = rxd->rx_cb;
			rxcb = rxcb->chain;
		}
		rxcb->chain = NULL;

		rxcb->rx_buf = udi_dma_buf_unmap(rxd->rx_sdmah->dmah, 0);

		/* return dma handle to pool */
		rxd->rx_sdmah->next = rxdv->next_sdmah;
		rxdv->next_sdmah = rxd->rx_sdmah;
		rxd->rx_sdmah = NULL;

		rxcb->rx_status = 0;
		rxcb->rx_valid = 0;

		rxd->rx_cb = 0;
		rxd->status = 0;
		rxd->cntrl &= SHRK_SWAP_DESC(rxdv, RX_RER);
		rxd->paddr1 = rxd->paddr2 = 0;

		rxdv->cdi = DSC_NEXT_CDI(rxdv);
		j++;
	}

	DEBUG01(("shrkrx_clean_ring(rxdv %x) rxrdy %d ring %d\n",
						rxdv, i, j));
}

/*
 *-----------------------------------------------------------------------------
 * Log functions
 */

void
shrk_log(shrktx_rdata_t *dv, udi_status_t status, udi_ubit32_t msgnum, ...)
{
	udi_cb_t *tmpcb;
	va_list args;
	void *arg1, *arg2, *arg3;

	if (dv->log_cb != NULL) {
		tmpcb = dv->log_cb;
		dv->log_cb = NULL;
		va_start(args, msgnum);
		arg1 = va_arg(args, void *);
		arg2 = va_arg(args, void *);
		arg3 = va_arg(args, void *);
		udi_log_write(shrk_log_complete, tmpcb, UDI_TREVENT_LOG,
			UDI_LOG_INFORMATION, SHRK_NIC_META, status,
			msgnum, arg1, arg2, arg3);
		va_end(args);
	}
} 

void
shrk_log_complete(udi_cb_t *cb, udi_status_t correlated_status)
{
	shrktx_rdata_t *dv = cb->context;

	udi_assert(dv->log_cb == NULL);
	dv->log_cb = cb;
}

void
shrkrx_log(shrkrx_rdata_t *rxdv, udi_status_t status, udi_ubit32_t msgnum, ...)
{
	udi_cb_t *tmpcb;
	va_list args;
	void *arg1, *arg2, *arg3;

	if (rxdv->log_cb != NULL) {
		tmpcb = rxdv->log_cb;
		rxdv->log_cb = NULL;
		va_start(args, msgnum);
		arg1 = va_arg(args, void *);
		arg2 = va_arg(args, void *);
		arg3 = va_arg(args, void *);
		udi_log_write(shrkrx_log_complete, tmpcb, UDI_TREVENT_LOG,
			UDI_LOG_INFORMATION, SHRK_NIC_META, status,
			msgnum, arg1, arg2, arg3);
		va_end(args);
	}
} 

void
shrkrx_log_complete(udi_cb_t *cb, udi_status_t correlated_status)
{
	shrkrx_rdata_t *rxdv = cb->context;

	udi_assert(rxdv->log_cb == NULL);
	rxdv->log_cb = cb;
}

/*
 * shrk_log_channel
 *	- any region
 *
 * called after log is written during an unknown channel event
 */
void
shrk_log_channel(udi_cb_t *cb, udi_status_t correlated_status)
{
	udi_channel_event_cb_t *ccb;
	
	ccb = UDI_MCB(cb->initiator_context, udi_channel_event_cb_t);
	udi_channel_event_complete(ccb, correlated_status);
}


#ifdef SHRK_DEBUG
/*
 *-----------------------------------------------------------------------------
 * Debug print functions
 */

/*
 * shrktxset
 *	- primary region, debug only
 *
 * dump data in setup frame
 */
void
shrktxset(void *mem)
{
	udi_ubit32_t i;
	udi_ubit16_t *data = mem;

	DEBUG16(("shrktxset SETUP DATA:\n"));

	for (i = 0; i < SHRK_SET_SZ / 2; i += 8) {
		DEBUG16(("%04x %04x   %04x %04x   %04x %04x   %04x %04x\n",
			data[i], data[i+1], data[i+2], data[i+3],
			data[i+4], data[i+5], data[i+6], data[i+7]));
	}
}

/*
 * shrktxd
 *	- primary region, debug only
 *
 * dump transmit descriptors
 */
void
shrktxd(shrktx_rdata_t *dv)
{
	udi_ubit32_t i;
	txd_t *txd;

	DEBUG16(("shrktxd(dv %x): cdi %d pdi %d\n", dv, dv->cdi, dv->pdi));

	for (i = 0, txd = dv->desc; i < dv->max_desc; i++, txd++) {
		DEBUG16(("%02d txd %08x tdes0/status %08x tdes1/cntrl %08x\n",
			i, txd, txd->status, txd->cntrl));
		DEBUG16(("    pa1 %08x pa2 %08x tx_cb %08x tx_sdmah %x\n",
			txd->paddr1, txd->paddr2, txd->tx_cb, txd->tx_sdmah));
		if ((txd->paddr1 != 0)
		    && (txd->cntrl & SHRK_SWAP_DESC(dv, TX_SET))) {
			shrktxset(dv->setup_mem);
		}
	}
}

/*
 * shrkrxd
 *	- secondary region, debug only
 *
 * dump receive descriptors
 */
void
shrkrxd(shrkrx_rdata_t *rxdv)
{
	udi_ubit32_t i;
	rxd_t *rxd;

	DEBUG14(("shrkrxd cdi %d pdi %d\n", rxdv->cdi, rxdv->pdi));

	for (i = 0, rxd = rxdv->desc; i < rxdv->max_desc; i++, rxd++) {
		DEBUG14(("%02d rxd %08x rdes0/status %08x rdes1/cntrl %08x\n",
			i, rxd, rxd->status, rxd->cntrl));
		DEBUG14(("    pa1 %08x pa2 %08x rx_cb %08x sdmah %08x\n",
			rxd->paddr1, rxd->paddr2, rxd->rx_cb,
			rxd->rx_sdmah));
	}
}

/*
 * shrktxcb
 *	- primary region
 *
 * dump transmit control blocks
 */
void
shrktxcb(shrktx_rdata_t *dv)
{
	udi_ubit32_t i = 0;
	udi_nic_tx_cb_t *txcb;

	DEBUG08(("shrktxcb(dv %x) tx_cb %x\n", dv, dv->tx_cb));

	for (txcb = dv->tx_cb; txcb != NULL; txcb = txcb->chain) {
		DEBUG08(("%02d txcb %x gcb %x\n", i++, txcb, txcb->gcb));
		DEBUG08(("   chain %x\n", txcb->chain));
		DEBUG08(("   tx_buf %x\n", txcb->tx_buf));
		DEBUG08(("   urgent %x\n", txcb->completion_urgent));
	}
}

#endif /* SHRK_DEBUG */
