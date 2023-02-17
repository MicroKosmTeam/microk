/*
 * File: meta/bridge/udi_bridge.c
 *
 * UDI Bus Bridge Metalanguage Library
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
#define UDI_PHYSIO_VERSION 0x101
#include <udi.h>			/* Public Core Spec Definitions */
#include <udi_physio.h>			/* Physical I/O Extensions */

/* 
 * ---------------------------------------------------------------------
 * Back-end stubs.
 * ---------------------------------------------------------------------
 */

static udi_mei_direct_stub_t udi_bus_bind_req_direct;
static udi_mei_backend_stub_t udi_bus_bind_req_backend;
static udi_mei_direct_stub_t udi_bus_bind_ack_direct;
static udi_mei_backend_stub_t udi_bus_bind_ack_backend;
static udi_mei_direct_stub_t udi_bus_unbind_req_direct;
static udi_mei_backend_stub_t udi_bus_unbind_req_backend;
static udi_mei_direct_stub_t udi_bus_unbind_ack_direct;
static udi_mei_backend_stub_t udi_bus_unbind_ack_backend;
static udi_mei_direct_stub_t udi_intr_attach_req_direct;
static udi_mei_backend_stub_t udi_intr_attach_req_backend;
static udi_mei_direct_stub_t udi_intr_attach_ack_direct;
static udi_mei_backend_stub_t udi_intr_attach_ack_backend;
static udi_mei_direct_stub_t udi_intr_detach_req_direct;
static udi_mei_backend_stub_t udi_intr_detach_req_backend;
static udi_mei_direct_stub_t udi_intr_detach_ack_direct;
static udi_mei_backend_stub_t udi_intr_detach_ack_backend;
static udi_mei_direct_stub_t udi_intr_event_rdy_direct;
static udi_mei_backend_stub_t udi_intr_event_rdy_backend;
static udi_mei_direct_stub_t udi_intr_event_ind_direct;
static udi_mei_backend_stub_t udi_intr_event_ind_backend;

/*
 * ---------------------------------------------------------------------
 * Op Templates and associated op_idx's.
 * ---------------------------------------------------------------------
 */

/* Bus Bridge op_idx's */
#define UDI_BUS_BIND_REQ	1
#define UDI_BUS_UNBIND_REQ	2
#define UDI_INTR_ATTACH_REQ	3
#define UDI_INTR_DETACH_REQ	4

static udi_layout_t udi_bus_bind_visible_layout[] = {
	UDI_DL_END
};

static udi_layout_t udi_intr_attach_visible_layout[] = {
	UDI_DL_INDEX_T,
	UDI_DL_UBIT8_T,
	UDI_DL_UBIT16_T,
	UDI_DL_PIO_HANDLE_T,
	UDI_DL_END
};

static udi_layout_t udi_intr_detach_visible_layout[] = {
	UDI_DL_INDEX_T,
	UDI_DL_END
};

static udi_layout_t empty_marshal_layout[] = {
	UDI_DL_END
};

/* Bus Bridge ops templates */
static udi_mei_op_template_t udi_bus_bridge_op_templates[] = {
	{"udi_bus_bind_req", UDI_MEI_OPCAT_REQ,
	 0, UDI_BUS_BIND_CB_NUM,
	 0,				/* completion_ops_num */
	 0,				/* completion_vec_index */
	 0,				/* exception_ops_num */
	 0,				/* exception_vec_idx */
	 udi_bus_bind_req_direct, udi_bus_bind_req_backend,
	 udi_bus_bind_visible_layout, empty_marshal_layout},
	{"udi_bus_unbind_req", UDI_MEI_OPCAT_REQ,
	 0, UDI_BUS_BIND_CB_NUM,
	 0,				/* completion_ops_num */
	 0,				/* completion_vec_index */
	 0,				/* exception_ops_num */
	 0,				/* exception_vec_idx */
	 udi_bus_unbind_req_direct, udi_bus_unbind_req_backend,
	 udi_bus_bind_visible_layout, empty_marshal_layout},
	{"udi_intr_attach_req", UDI_MEI_OPCAT_REQ,
	 0, UDI_BUS_INTR_ATTACH_CB_NUM,
	 0,				/* completion_ops_num */
	 0,				/* completion_vec_index */
	 0,				/* exception_ops_num */
	 0,				/* exception_vec_idx */
	 udi_intr_attach_req_direct, udi_intr_attach_req_backend,
	 udi_intr_attach_visible_layout, empty_marshal_layout},
	{"udi_intr_detach_req", UDI_MEI_OPCAT_REQ,
	 0, UDI_BUS_INTR_DETACH_CB_NUM,
	 0,				/* completion_ops_num */
	 0,				/* completion_vec_index */
	 0,				/* exception_ops_num */
	 0,				/* exception_vec_idx */
	 udi_intr_detach_req_direct, udi_intr_detach_req_backend,
	 udi_intr_detach_visible_layout, empty_marshal_layout},
	{NULL}
};

/* Bus Device op_idx's */
#define UDI_BUS_BIND_ACK	1
#define UDI_BUS_UNBIND_ACK	2
#define UDI_INTR_ATTACH_ACK	3
#define UDI_INTR_DETACH_ACK	4

static udi_layout_t udi_bus_bind_ack_marshal_layout[] = {
	UDI_DL_DMA_CONSTRAINTS_T,
	UDI_DL_UBIT8_T,
	UDI_DL_STATUS_T,
	UDI_DL_END
};

static udi_layout_t udi_intr_attach_ack_marshal_layout[] = {
	UDI_DL_STATUS_T,
	UDI_DL_END
};

/* Bus Device ops templates */
static udi_mei_op_template_t udi_bus_device_op_templates[] = {
	{"udi_bus_bind_ack", UDI_MEI_OPCAT_ACK,
	 0, UDI_BUS_BIND_CB_NUM,
	 0,				/* completion_ops_num */
	 0,				/* completion_vec_index */
	 0,				/* exception_ops_num */
	 0,				/* exception_vec_idx */
	 udi_bus_bind_ack_direct, udi_bus_bind_ack_backend,
	 udi_bus_bind_visible_layout, udi_bus_bind_ack_marshal_layout},
	{"udi_bus_unbind_ack", UDI_MEI_OPCAT_ACK,
	 0, UDI_BUS_BIND_CB_NUM,
	 0,				/* completion_ops_num */
	 0,				/* completion_vec_index */
	 0,				/* exception_ops_num */
	 0,				/* exception_vec_idx */
	 udi_bus_unbind_ack_direct, udi_bus_unbind_ack_backend,
	 udi_bus_bind_visible_layout, empty_marshal_layout},
	{"udi_intr_attach_ack", UDI_MEI_OPCAT_ACK,
	 0, UDI_BUS_INTR_ATTACH_CB_NUM,
	 0,				/* completion_ops_num */
	 0,				/* completion_vec_index */
	 0,				/* exception_ops_num */
	 0,				/* exception_vec_idx */
	 udi_intr_attach_ack_direct, udi_intr_attach_ack_backend,
	 udi_intr_attach_visible_layout, udi_intr_attach_ack_marshal_layout},
	{"udi_intr_detach_ack", UDI_MEI_OPCAT_ACK,
	 0, UDI_BUS_INTR_DETACH_CB_NUM,
	 0,				/* completion_ops_num */
	 0,				/* completion_vec_index */
	 0,				/* exception_ops_num */
	 0,				/* exception_vec_idx */
	 udi_intr_detach_ack_direct, udi_intr_detach_ack_backend,
	 udi_intr_detach_visible_layout,
	 empty_marshal_layout},
	{NULL}
};

/* Interrupt Dispatcher op_idx's */
#define UDI_INTR_EVENT_RES	1

static udi_layout_t udi_intr_event_ind_visible_layout[] = {
	UDI_DL_BUF, 0, 0, 0,		/* always preserve */
	UDI_DL_UBIT16_T,
	UDI_DL_END
};

static udi_layout_t udi_intr_event_rdy_visible_layout[] = {
	UDI_DL_BUF, 0, 0, 1,		/* never preserve */
	UDI_DL_UBIT16_T,
	UDI_DL_END
};

/* Interrupt Dispatcher ops templates */
static udi_mei_op_template_t udi_intr_dispatcher_op_templates[] = {
	{"udi_intr_event_rdy", UDI_MEI_OPCAT_RES,
	 0, UDI_BUS_INTR_EVENT_CB_NUM,
	 0,				/* completion_ops_num */
	 0,				/* completion_vec_index */
	 0,				/* exception_ops_num */
	 0,				/* exception_vec_idx */
	 udi_intr_event_rdy_direct, udi_intr_event_rdy_backend,
	 udi_intr_event_rdy_visible_layout,
	 empty_marshal_layout},
	{NULL}
};

/* Interrupt Handler op_idx's */
#define UDI_INTR_EVENT_IND	1

static udi_layout_t udi_intr_event_ind_marshal_layout[] = {
	UDI_DL_UBIT8_T,
	UDI_DL_END
};

/* Interrupt Handler ops templates */
static udi_mei_op_template_t udi_intr_handler_op_templates[] = {
	{"udi_intr_event_ind", UDI_MEI_OPCAT_IND,
	 0, UDI_BUS_INTR_EVENT_CB_NUM,
	 0,				/* completion_ops_num */
	 0,				/* completion_vec_index */
	 0,				/* exception_ops_num */
	 0,				/* exception_vec_idx */
	 udi_intr_event_ind_direct, udi_intr_event_ind_backend,
	 udi_intr_event_ind_visible_layout,
	 udi_intr_event_ind_marshal_layout},
	{NULL}

};


/*
 * ========================================================================
 * 
 * Ops-Init Interfaces
 * 
 * ------------------------------------------------------------------------
 */

static udi_mei_ops_vec_template_t bridge_ops_vec_template_list[] = {
	{
		UDI_BUS_DEVICE_OPS_NUM,
		UDI_MEI_REL_BIND | UDI_MEI_REL_EXTERNAL | UDI_MEI_REL_INITIATOR,
		udi_bus_device_op_templates
	},
	{
		UDI_BUS_BRIDGE_OPS_NUM,
		UDI_MEI_REL_BIND | UDI_MEI_REL_EXTERNAL,
		udi_bus_bridge_op_templates
	},
	{
		UDI_BUS_INTR_HANDLER_OPS_NUM,
		UDI_MEI_REL_EXTERNAL | UDI_MEI_REL_INITIATOR,
		udi_intr_handler_op_templates
	},
	{
		UDI_BUS_INTR_DISPATCH_OPS_NUM,
		UDI_MEI_REL_EXTERNAL,
		udi_intr_dispatcher_op_templates
	},
	{
		0, 0, NULL
	}
};

static udi_mei_enumeration_rank_func_t udi_bus_rank_func;

static udi_ubit8_t udi_bus_rank_func(udi_ubit32_t attr_device_match,
		void **attr_value_list)
{
	udi_assert(0);
	return 0;
}

udi_mei_init_t udi_meta_info = {
	bridge_ops_vec_template_list,
	udi_bus_rank_func
};

/*
 * ========================================================================
 * 
 * Metalanguage Stubs
 *
 * ------------------------------------------------------------------------
 */

/*
 *  udi_bus_bind_req - Request a binding to a bus bridge driver
 */
UDI_MEI_STUBS(udi_bus_bind_req, udi_bus_bind_cb_t,
	      0, (), (), (), UDI_BUS_DEVICE_OPS_NUM, UDI_BUS_BIND_REQ)

/*
 *  udi_bus_bind_ack - Acknowledge a bus binding
 */
UDI_MEI_STUBS(udi_bus_bind_ack, udi_bus_bind_cb_t,
	      3, (dma_constraints, preferred_endianness, status),
	         (udi_dma_constraints_t, udi_ubit8_t, udi_status_t),
	         (UDI_VA_DMA_CONSTRAINTS_T, UDI_VA_UBIT8_T, UDI_VA_STATUS_T),
	      UDI_BUS_BRIDGE_OPS_NUM, UDI_BUS_BIND_ACK)

/*
 *  udi_bus_unbind_req - Request to unbind from a bus bridge driver
 */
UDI_MEI_STUBS(udi_bus_unbind_req, udi_bus_bind_cb_t,
	      0, (), (), (), UDI_BUS_DEVICE_OPS_NUM, UDI_BUS_UNBIND_REQ)

/*
 *  udi_bus_unbind_ack - Acknowledge a bus unbind request
 */
UDI_MEI_STUBS(udi_bus_unbind_ack, udi_bus_bind_cb_t,
	      0, (), (), (), UDI_BUS_BRIDGE_OPS_NUM, UDI_BUS_UNBIND_ACK)

/*
 *  udi_intr_attach_req - Request an interrupt attachment
 */
UDI_MEI_STUBS(udi_intr_attach_req, udi_intr_attach_cb_t,
	      0, (), (), (), UDI_BUS_DEVICE_OPS_NUM, UDI_INTR_ATTACH_REQ)

/*
 *  udi_intr_attach_ack - Acknowledge an interrupt attachment operation
 */
UDI_MEI_STUBS(udi_intr_attach_ack, udi_intr_attach_cb_t,
	      1, (status), (udi_status_t), (UDI_VA_STATUS_T),
	      UDI_BUS_BRIDGE_OPS_NUM, UDI_INTR_ATTACH_ACK)

/*
 *  udi_intr_detach_req - Request an interrupt detachment
 */
UDI_MEI_STUBS(udi_intr_detach_req, udi_intr_detach_cb_t,
	      0, (), (), (), UDI_BUS_DEVICE_OPS_NUM, UDI_INTR_DETACH_REQ)

/*
 *  udi_intr_detach_ack - Acknowledge an interrupt detachment operation
 */
UDI_MEI_STUBS(udi_intr_detach_ack, udi_intr_detach_cb_t,
	      0, (), (), (), UDI_BUS_BRIDGE_OPS_NUM, UDI_INTR_DETACH_ACK)

/*
 *  udi_intr_event_ind - Interrupt event indication
 */
UDI_MEI_STUBS(udi_intr_event_ind, udi_intr_event_cb_t,
	      1, (flags), (udi_ubit8_t), (UDI_VA_UBIT8_T),
	      UDI_BUS_INTR_HANDLER_OPS_NUM, UDI_INTR_EVENT_IND)

/*
 *  udi_intr_event_rdy - Interrupt event response
 */
UDI_MEI_STUBS(udi_intr_event_rdy, udi_intr_event_cb_t,
	      0, (), (), (), UDI_BUS_INTR_DISPATCH_OPS_NUM, UDI_INTR_EVENT_RES)

/*
 * ========================================================================
 * 
 * Proxy functions
 *
 * ------------------------------------------------------------------------
 */
void
udi_intr_attach_ack_unused(udi_intr_attach_cb_t * cb,
				     udi_status_t status)
{
	udi_assert(0);
}

void
udi_intr_detach_ack_unused(udi_intr_detach_cb_t * cb)
{
	udi_assert(0);
}
