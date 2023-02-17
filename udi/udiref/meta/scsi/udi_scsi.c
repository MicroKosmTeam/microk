/*
 * File: meta/scsi/udi_scsi.c
 *
 * UDI SCSI Metalanguage Library
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

#define UDI_VERSION 0x101
#define UDI_SCSI_VERSION 0x101
#include <udi.h>	   /* Public Core Environment Definitions */	
#include <udi_scsi.h>	   /* Public Driver-to-Metalang Definitions */

/* 
 * ---------------------------------------------------------------------
 * Front-end stubs.
 * ---------------------------------------------------------------------
 */

static udi_mei_direct_stub_t udi_scsi_bind_req_direct;
static udi_mei_direct_stub_t udi_scsi_bind_ack_direct;
static udi_mei_direct_stub_t udi_scsi_unbind_req_direct;
static udi_mei_direct_stub_t udi_scsi_unbind_ack_direct;
static udi_mei_direct_stub_t udi_scsi_io_req_direct;
static udi_mei_direct_stub_t udi_scsi_io_ack_direct;
static udi_mei_direct_stub_t udi_scsi_io_nak_direct;
static udi_mei_direct_stub_t udi_scsi_ctl_req_direct;
static udi_mei_direct_stub_t udi_scsi_ctl_ack_direct;
static udi_mei_direct_stub_t udi_scsi_event_ind_direct;
static udi_mei_direct_stub_t udi_scsi_event_res_direct;

/* 
 * ---------------------------------------------------------------------
 * Back-end stubs.
 * ---------------------------------------------------------------------
 */

static udi_mei_backend_stub_t udi_scsi_bind_req_backend;
static udi_mei_backend_stub_t udi_scsi_bind_ack_backend;
static udi_mei_backend_stub_t udi_scsi_unbind_req_backend;
static udi_mei_backend_stub_t udi_scsi_unbind_ack_backend;
static udi_mei_backend_stub_t udi_scsi_io_req_backend;
static udi_mei_backend_stub_t udi_scsi_io_ack_backend;
static udi_mei_backend_stub_t udi_scsi_io_nak_backend;
static udi_mei_backend_stub_t udi_scsi_ctl_req_backend;
static udi_mei_backend_stub_t udi_scsi_ctl_ack_backend;
static udi_mei_backend_stub_t udi_scsi_event_ind_backend;
static udi_mei_backend_stub_t udi_scsi_event_res_backend;

/* 
 * ------------------------------------
 * Ops Templates and associated op_idx's.
 * ------------------------------------
 */

/* HD op_idx's */
#define UDI_SCSI_BIND_REQ	1
#define UDI_SCSI_UNBIND_REQ	2
#define UDI_SCSI_IO_REQ		3
#define UDI_SCSI_CTL_REQ	4
#define UDI_SCSI_EVENT_RES	5

/* PD op_idx's */
#define UDI_SCSI_BIND_ACK	1
#define UDI_SCSI_UNBIND_ACK	2
#define UDI_SCSI_IO_ACK		3
#define UDI_SCSI_IO_NAK		4
#define UDI_SCSI_CTL_ACK	5
#define UDI_SCSI_EVENT_IND	6

/*
 * Control block layouts
 */
static udi_layout_t udi_scsi_bind_visible_layout[] = {
	UDI_DL_UBIT16_T,
	UDI_DL_END
};

static udi_layout_t udi_scsi_io_req_visible_layout[] = {
	UDI_DL_BUF,	/* preserve if UDI_SCSI_DATA_OUT flag set in flags */
		offsetof(udi_scsi_io_cb_t, flags),
		UDI_SCSI_DATA_OUT,
		UDI_SCSI_DATA_OUT,
	UDI_DL_UBIT32_T,
	UDI_DL_UBIT16_T,
	UDI_DL_UBIT8_T,
	UDI_DL_UBIT8_T,
	UDI_DL_INLINE_UNTYPED,
	UDI_DL_END
};

static udi_layout_t udi_scsi_io_ack_visible_layout[] = {
	UDI_DL_BUF,	/* preserve if UDI_SCSI_DATA_IN flag set in flags */
		offsetof(udi_scsi_io_cb_t, flags),
		UDI_SCSI_DATA_IN,
		UDI_SCSI_DATA_IN,
	UDI_DL_UBIT32_T,
	UDI_DL_UBIT16_T,
	UDI_DL_UBIT8_T,
	UDI_DL_UBIT8_T,
	UDI_DL_INLINE_UNTYPED,
	UDI_DL_END
};

static udi_layout_t udi_scsi_io_nak_visible_layout[] = {
	UDI_DL_BUF,	/* preserve if UDI_SCSI_DATA_OUT flag set in flags */
		offsetof(udi_scsi_io_cb_t, flags),
		UDI_SCSI_DATA_OUT,
		UDI_SCSI_DATA_OUT,
	UDI_DL_UBIT32_T,
	UDI_DL_UBIT16_T,
	UDI_DL_UBIT8_T,
	UDI_DL_UBIT8_T,
	UDI_DL_INLINE_UNTYPED,
	UDI_DL_END
};

static udi_layout_t udi_scsi_ctl_visible_layout[] = {
	UDI_DL_UBIT8_T,
	UDI_DL_UBIT16_T,
	UDI_DL_END
};

static udi_layout_t udi_scsi_event_ind_visible_layout[] = {
	UDI_DL_UBIT8_T,
	UDI_DL_BUF, 0, 0, 0,	/* always preserve */
	UDI_DL_END
};

static udi_layout_t udi_scsi_event_res_visible_layout[] = {
	UDI_DL_UBIT8_T,
	UDI_DL_BUF, 0, 0, 1,	/* never preserve */
	UDI_DL_END
};


/*
 *-----------------------------------------------------------------------------
 * udi_scsi_bind_req
 *-----------------------------------------------------------------------------
 */
static udi_layout_t udi_scsi_bind_req_marshal_layout[] = {
	UDI_DL_UBIT16_T,
	UDI_DL_UBIT16_T,
	UDI_DL_UBIT32_T,	/* udi_time_t */
	UDI_DL_UBIT32_T,	/* udi_time_t */
	UDI_DL_UBIT16_T,
	UDI_DL_UBIT16_T,
	UDI_DL_END
};

/*
 *-----------------------------------------------------------------------------
 * udi_scsi_bind_ack
 *-----------------------------------------------------------------------------
 */

static udi_layout_t udi_scsi_bind_ack_marshal_layout[] = {
	UDI_DL_UBIT32_T,
	UDI_DL_STATUS_T,
	UDI_DL_END
};

/*
 *-----------------------------------------------------------------------------
 * udi_scsi_io_nak
 *-----------------------------------------------------------------------------
 */

static udi_layout_t udi_scsi_io_nak_marshal_layout[] = {
	UDI_DL_STATUS_T,
	UDI_DL_BUF, 0, 0, 0,	/* always preserve */
	UDI_DL_END
};

/*
 *-----------------------------------------------------------------------------
 * udi_scsi_ctl_ack
 *-----------------------------------------------------------------------------
 */

static udi_layout_t udi_scsi_ctl_ack_marshal_layout[] = {
	UDI_DL_STATUS_T,
	UDI_DL_END
};

/* PD ops templates */
static udi_mei_op_template_t udi_scsi_pd_op_templates[] = {
	/* udi_scsi_bind_ack_template */
	{
		"udi_scsi_bind_ack",
		UDI_MEI_OPCAT_ACK,
		0,	/* op_flags */
		UDI_SCSI_BIND_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_scsi_bind_ack_direct,
		udi_scsi_bind_ack_backend,
		udi_scsi_bind_visible_layout,
		udi_scsi_bind_ack_marshal_layout
	},

	/* udi_scsi_unbind_ack_template */
	{
		"udi_scsi_unbind_ack",
		UDI_MEI_OPCAT_ACK,
		0,	/* op_flags */
		UDI_SCSI_BIND_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_scsi_unbind_ack_direct,
		udi_scsi_unbind_ack_backend,
		udi_scsi_bind_visible_layout
	},

	/* udi_scsi_io_ack_template */
	{
		"udi_scsi_io_ack",
		UDI_MEI_OPCAT_ACK,
		0,	/* op_flags */
		UDI_SCSI_IO_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_scsi_io_ack_direct,
		udi_scsi_io_ack_backend,
		udi_scsi_io_ack_visible_layout
	},

	/* udi_scsi_io_nak_template */
	{
		"udi_scsi_io_nak",
		UDI_MEI_OPCAT_ACK,
		0,	/* op_flags */
		UDI_SCSI_IO_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_scsi_io_nak_direct, 
		udi_scsi_io_nak_backend,
		udi_scsi_io_nak_visible_layout,
		udi_scsi_io_nak_marshal_layout
	},

	/* udi_scsi_ctl_ack_template */
	{
		"udi_scsi_ctl_ack",
		UDI_MEI_OPCAT_ACK,
		0,	/* op_flags */
		UDI_SCSI_CTL_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_scsi_ctl_ack_direct,
		udi_scsi_ctl_ack_backend,
		udi_scsi_ctl_visible_layout
	},

	/* udi_scsi_event_ind_template */
	{
		"udi_scsi_event_ind",
		UDI_MEI_OPCAT_ACK,
		0,	/* op_flags */
		UDI_SCSI_EVENT_CB_NUM,
		UDI_SCSI_PD_OPS_NUM,	/* completion_ops_num */
		UDI_SCSI_EVENT_RES,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_scsi_event_ind_direct,
		udi_scsi_event_ind_backend,
		udi_scsi_event_ind_visible_layout
	},
	{
		0, /* terminate the list */
	}
};

/* HD op templates */
static udi_mei_op_template_t udi_scsi_hd_op_templates[] = {
	/* udi_scsi_bind_req_template */
	{
		"udi_scsi_bind_req",
		UDI_MEI_OPCAT_REQ,
		0,	/* op_flags */
		UDI_SCSI_BIND_CB_NUM,
		UDI_SCSI_PD_OPS_NUM,	/* completion_ops_num */
		UDI_SCSI_BIND_ACK,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_scsi_bind_req_direct,
		udi_scsi_bind_req_backend,
		udi_scsi_bind_visible_layout,
		udi_scsi_bind_req_marshal_layout
	},

	/* udi_scsi_unbind_req_template */
	{
		"udi_scsi_unbind_req",
		UDI_MEI_OPCAT_REQ,
		0,	/* op_flags */
		UDI_SCSI_BIND_CB_NUM,
		UDI_SCSI_PD_OPS_NUM,	/* completion_ops_num */
		UDI_SCSI_UNBIND_ACK,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_scsi_unbind_req_direct,
		udi_scsi_unbind_req_backend,
		udi_scsi_bind_visible_layout
	},

	/* udi_scsi_io_req_template */
	{
		"udi_scsi_io_req",
		UDI_MEI_OPCAT_REQ,
		UDI_MEI_OP_ABORTABLE,	/* op_flags */
		UDI_SCSI_IO_CB_NUM,
		UDI_SCSI_PD_OPS_NUM,	/* completion_ops_num */
		UDI_SCSI_IO_ACK,	/* completion_vec_index */
		UDI_SCSI_PD_OPS_NUM,	/* exception_ops_num */
		UDI_SCSI_IO_NAK,	/* exception_vec_idx */
		udi_scsi_io_req_direct,
		udi_scsi_io_req_backend,
		udi_scsi_io_req_visible_layout
	},

	/* udi_scsi_ctl_req_template */
	{
		"udi_scsi_ctl_req",
		UDI_MEI_OPCAT_REQ,
		0,	/* op_flags */
		UDI_SCSI_CTL_CB_NUM,
		UDI_SCSI_PD_OPS_NUM,	/* completion_ops_num */
		UDI_SCSI_CTL_ACK,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_scsi_ctl_req_direct,
		udi_scsi_ctl_req_backend,
		udi_scsi_ctl_visible_layout
	},

	/* udi_scsi_event_res_template */
	{
		"udi_scsi_event_res",
		UDI_MEI_OPCAT_RES,
		0,	/* op_flags */
		UDI_SCSI_EVENT_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_scsi_event_res_direct,
		udi_scsi_event_res_backend,
		udi_scsi_event_res_visible_layout
	},
	{
		0, /* terminate the list */
	}
};

/*
 *----------------------------------------------------------------------
 * ops vector template 
 *----------------------------------------------------------------------
 */
static udi_mei_ops_vec_template_t scsi_ops_vec_template_list[] = {
	{
		UDI_SCSI_PD_OPS_NUM,
		UDI_MEI_REL_INITIATOR|UDI_MEI_REL_EXTERNAL|UDI_MEI_REL_BIND,
		udi_scsi_pd_op_templates
	},
	{
		UDI_SCSI_HD_OPS_NUM,
		UDI_MEI_REL_EXTERNAL|UDI_MEI_REL_BIND,
		udi_scsi_hd_op_templates
	},
	{
		0, 0, NULL	/* Terminate list */
	}
};

static udi_mei_enumeration_rank_func_t udi_scsi_rank_func;

static udi_ubit8_t udi_scsi_rank_func(udi_ubit32_t attr_device_match,
                void **attr_value_list)
{
        udi_assert(0);
	return 0;
}

udi_mei_init_t udi_meta_info = {
	scsi_ops_vec_template_list,
	udi_scsi_rank_func
};

/*
 * ========================================================================
 * 
 * Channel Operations
 * 
 * ------------------------------------------------------------------------
 */

/*
 * ========================================================================
 * 
 * Unmarshalling Routines (aka "Metalanguage Stubs")
 *  
 * After dequeuing a control block from the region queue the environment
 * calls the corresponding metalanguage-specific unmarshalling routine to 
 * unmarshal the extra parameters from the control block and make the 
 * associated call to the driver.
 * ------------------------------------------------------------------------
 */


/*
 *  udi_scsi_bind_req - Request a SCSI binding (PD-to-HD)
 */

UDI_MEI_STUBS(udi_scsi_bind_req, udi_scsi_bind_cb_t,
	      4, (bind_flags, queue_depth, max_sense_len, aen_buf_size),
		 (udi_ubit16_t, udi_ubit16_t, udi_ubit16_t, udi_ubit16_t),
		 (UDI_VA_UBIT16_T, UDI_VA_UBIT16_T, UDI_VA_UBIT16_T,
		  UDI_VA_UBIT16_T),
	      UDI_SCSI_HD_OPS_NUM, UDI_SCSI_BIND_REQ)

/*
 *  udi_scsi_bind_ack - Acknowledge a SCSI binding (HD-to-PD)
 */
UDI_MEI_STUBS(udi_scsi_bind_ack, udi_scsi_bind_cb_t,
	      2, (hd_timeout_increase, status),
		 (udi_ubit32_t, udi_status_t),
		 (UDI_VA_UBIT32_T, UDI_VA_STATUS_T),
	      UDI_SCSI_PD_OPS_NUM, UDI_SCSI_BIND_ACK)

/*
 *  udi_scsi_unbind_req - Request a SCSI unbind (PD-to-HD)
 */

UDI_MEI_STUBS(udi_scsi_unbind_req, udi_scsi_bind_cb_t,
	      0, (), (), (),
	      UDI_SCSI_HD_OPS_NUM, UDI_SCSI_UNBIND_REQ)

/*
 *  udi_scsi_unbind_ack - Acknowledge a SCSI unbind (HD-to-PD)
 */

UDI_MEI_STUBS(udi_scsi_unbind_ack, udi_scsi_bind_cb_t,
	      0, (), (), (),
	      UDI_SCSI_PD_OPS_NUM, UDI_SCSI_UNBIND_ACK)

/*
 *  udi_scsi_io_req - Request a SCSI IO operation (PD-to-HD)
 */

UDI_MEI_STUBS(udi_scsi_io_req, udi_scsi_io_cb_t,
	      0, (), (), (),
	      UDI_SCSI_HD_OPS_NUM, UDI_SCSI_IO_REQ)

/*
 *  udi_scsi_io_ack - Acknowledge normal completion of SCSI IO operation
 * 			 (HD-to-PD)
 */

UDI_MEI_STUBS(udi_scsi_io_ack, udi_scsi_io_cb_t,
	      0, (), (), (),
	      UDI_SCSI_PD_OPS_NUM, UDI_SCSI_IO_ACK)

/*
 *  udi_scsi_io_nak - Acknowledge abnormal completion of SCSI IO operation
 * 			 (HD-to-PD)
 */

UDI_MEI_STUBS(udi_scsi_io_nak, udi_scsi_io_cb_t,
	      4, (req_status, scsi_status, sense_status, sense_buf),
		 (udi_status_t, udi_ubit8_t, udi_ubit8_t, udi_buf_t *),
		 (UDI_VA_STATUS_T, UDI_VA_UBIT8_T, UDI_VA_UBIT8_T,
		  UDI_VA_POINTER),
	      UDI_SCSI_PD_OPS_NUM, UDI_SCSI_IO_NAK)

/*
 *  udi_scsi_ctl_req - Request a SCSI control operation (PD-to-HD)
 */
UDI_MEI_STUBS(udi_scsi_ctl_req, udi_scsi_ctl_cb_t,
	      0, (), (), (),
	      UDI_SCSI_HD_OPS_NUM, UDI_SCSI_CTL_REQ)

/*
 *  udi_scsi_ctl_ack - Acknowledge a SCSI control operation (HD-to-PD)
 */
UDI_MEI_STUBS(udi_scsi_ctl_ack, udi_scsi_ctl_cb_t,
	      1, (status), (udi_status_t), (UDI_VA_STATUS_T),
	      UDI_SCSI_PD_OPS_NUM, UDI_SCSI_CTL_ACK)

/*
 *  udi_scsi_event_ind - SCSI event notification (HD-to-PD)
 */
UDI_MEI_STUBS(udi_scsi_event_ind, udi_scsi_event_cb_t,
	      0, (), (), (),
	      UDI_SCSI_PD_OPS_NUM, UDI_SCSI_EVENT_IND)

/*
 *  udi_scsi_event_res - Acknowledge a SCSI scsi event (PD-to-HD)
 */
UDI_MEI_STUBS(udi_scsi_event_res, udi_scsi_event_cb_t,
	      0, (), (), (),
	      UDI_SCSI_HD_OPS_NUM, UDI_SCSI_EVENT_RES)

/*
 * "Proxy" Functions
 */

void
udi_scsi_event_ind_unused(
	udi_scsi_event_cb_t	*event_cb )
{
	/* Should never be called */
	udi_assert(0);
}

/*
 *---------------------------------------------------------------------
 * Utility Functions
 *---------------------------------------------------------------------
 */

void
udi_scsi_inquiry_to_string (
	const udi_ubit8_t	*inquiry_data,
	udi_size_t		inquiry_len,
	char			*str)
{
	udi_ubit8_t	i;
	const udi_ubit8_t	*dp = inquiry_data;
	static const udi_ubit8_t Hex[]="0123456789ABCDEF";

	for (i=0; i < 8 && i < inquiry_len; i++, dp++) {
		*str++ = Hex[(*dp) >> 4];
		*str++ = Hex[(*dp) & 0x0F];
	}
	for (; i < 36 && i < inquiry_len; i++, str++, dp++) {
		*str = ((*dp < 33) || (*dp > 126)) ? '.' : *dp;
	}
}
