/*
 * File:  meta/mgmt/udi_mgmt.c
 *
 * UDI Management Metalanguage Library
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
 * ========================================================================
 * ------------------------------------------------------------------------
 */
#define UDI_VERSION 0x101
#include <udi.h>
#include <udi_mgmt_MA.h>

/* 
 * ---------------------------------------------------------------------
 * Back-end stubs.
 * ---------------------------------------------------------------------
 */
/* Function prototypes for unmarshalling ops. */
static udi_mei_backend_stub_t udi_usage_ind_backend;
static udi_mei_backend_stub_t udi_usage_res_backend;
static udi_mei_backend_stub_t udi_enumerate_req_backend;
static udi_mei_backend_stub_t udi_enumerate_ack_backend;
static udi_mei_backend_stub_t udi_devmgmt_req_backend;
static udi_mei_backend_stub_t udi_devmgmt_ack_backend;
static udi_mei_backend_stub_t udi_final_cleanup_req_backend;
static udi_mei_backend_stub_t udi_final_cleanup_ack_backend;

/* 
 * ---------------------------------------------------------------------
 * Front-end stubs.
 * ---------------------------------------------------------------------
 */
static udi_mei_direct_stub_t udi_usage_ind_direct;
static udi_mei_direct_stub_t udi_usage_res_direct;
static udi_mei_direct_stub_t udi_enumerate_req_direct;
static udi_mei_direct_stub_t udi_enumerate_ack_direct;
static udi_mei_direct_stub_t udi_devmgmt_req_direct;
static udi_mei_direct_stub_t udi_devmgmt_ack_direct;
static udi_mei_direct_stub_t udi_final_cleanup_req_direct;
static udi_mei_direct_stub_t udi_final_cleanup_ack_direct;

/*
 * ---------------------------------------------------------------------
 * Op Templates and associated op_idx's.
 * ---------------------------------------------------------------------
 */

/*
 * ------------------------------------
 * Ops Templates and associated op_idx's.
 * ------------------------------------
 */
#define UDI_USAGE_IND			0
#define UDI_ENUMERATE_REQ		1
#define UDI_DEVMGMT_REQ			2
#define UDI_FINAL_CLEANUP_REQ		3


/* 
 * ------------------------------------------------------------------------
 * Parameter Marshalling layout descriptors.
 * ------------------------------------------------------------------------
 */

static udi_layout_t udi_usage_ind_marshal_layout[] = {
	UDI_DL_UBIT8_T,
	UDI_DL_END
};

static udi_layout_t udi_enumerate_req_marshal_layout[] = {
	UDI_DL_UBIT8_T,
	UDI_DL_END
};

static udi_layout_t udi_enumerate_ack_marshal_layout[] = {
	UDI_DL_UBIT8_T,
	UDI_DL_INDEX_T,
	UDI_DL_END
};

static udi_layout_t udi_devmgmt_req_marshal_layout[] = {
	UDI_DL_UBIT8_T,
	UDI_DL_UBIT8_T,
	UDI_DL_END
};

static udi_layout_t udi_devmgmt_ack_marshal_layout[] = {
	UDI_DL_UBIT8_T,
	UDI_DL_STATUS_T,
	UDI_DL_END
};

/*
 * ------------------------------------------------------------------------
 * Control block layouts
 * ------------------------------------------------------------------------
 */

static udi_layout_t udi_mgmt_visible_layout[] = {
	UDI_DL_END
};

static udi_layout_t udi_usage_visible_layout[] = {
	UDI_DL_UBIT32_T, /* FIXME: Figure out what a trevent_t is */
	UDI_DL_INDEX_T,
	UDI_DL_END
};

static udi_layout_t udi_enumerate_visible_layout[] = {
	UDI_DL_UBIT8_T,
	UDI_DL_UBIT8_T,
	UDI_DL_UBIT8_T,
	UDI_DL_END
};

/* Although this requires us to know what a udi_MA_cb_t looks like, we
 * just create an array of the right size since these CB's will never
 * be marshalled or tranferred
 */
static udi_layout_t udi_MA_visible_layout[] = {
	UDI_DL_ARRAY,
		sizeof(udi_MA_cb_t),
		UDI_DL_UBIT8_T,
		UDI_DL_END, 
	UDI_DL_END };
static udi_layout_t udi_mgmt_empty_layout[] = { UDI_DL_END };

/* MA ops templates */
static udi_mei_op_template_t udi_mgmt_MA_op_templates[] = {
	{ "udi_usage_res", UDI_MEI_OPCAT_RES, 0, UDI_MGMT_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_usage_res_direct, udi_usage_res_backend, 
		udi_MA_visible_layout, udi_usage_ind_marshal_layout }, 
	{ "udi_enumerate_ack", UDI_MEI_OPCAT_ACK, 0, UDI_MGMT_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_enumerate_ack_direct, udi_enumerate_ack_backend, 
		udi_MA_visible_layout, udi_enumerate_ack_marshal_layout },
	{ "udi_devmgmt_ack", UDI_MEI_OPCAT_ACK, 0, UDI_MGMT_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_devmgmt_ack_direct, udi_devmgmt_ack_backend, 
		udi_MA_visible_layout, udi_devmgmt_ack_marshal_layout },
	{ "udi_final_cleanup_ack", UDI_MEI_OPCAT_ACK, 0, UDI_MGMT_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_final_cleanup_ack_direct, udi_final_cleanup_ack_backend, 
		udi_MA_visible_layout, udi_mgmt_empty_layout },
	{ NULL }
};


/* Driver op_idx's */
#define UDI_USAGE_RES			0
#define UDI_ENUMERATE_ACK		1
#define UDI_DEVMGMT_ACK			2
#define UDI_FINAL_CLEANUP_ACK		3


/* Driver ops templates */
static udi_mei_op_template_t udi_mgmt_drv_op_templates[] = {
	{ "udi_usage_ind", UDI_MEI_OPCAT_IND, 0, UDI_MGMT_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_usage_ind_direct, udi_usage_ind_backend, 
		udi_usage_visible_layout, udi_usage_ind_marshal_layout },
	{ "udi_enumerate_req", UDI_MEI_OPCAT_REQ, 0, UDI_MGMT_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_enumerate_req_direct, udi_enumerate_req_backend, 
		udi_enumerate_visible_layout, udi_enumerate_req_marshal_layout},
	{ "udi_devmgmt_req", UDI_MEI_OPCAT_REQ, 0, UDI_MGMT_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_devmgmt_req_direct, udi_devmgmt_req_backend, 
		udi_mgmt_visible_layout, udi_devmgmt_req_marshal_layout },
	{ "udi_final_cleanup_req", UDI_MEI_OPCAT_ACK, 0, UDI_MGMT_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_final_cleanup_req_direct, udi_final_cleanup_req_backend, 
		udi_mgmt_visible_layout, udi_mgmt_empty_layout },
	{ NULL }
};

/*
 * ========================================================================
 * 
 * Set-Interface-Ops Interfaces
 * 
 * ------------------------------------------------------------------------
 */

static 
udi_mei_ops_vec_template_t mgmt_ops_vec_templates_list[] = {
	{ 
		UDI_MGMT_MA_OPS_NUM,
		UDI_MEI_REL_INTERNAL | UDI_MEI_REL_BIND,
		udi_mgmt_MA_op_templates
	},
	{ 
		UDI_MGMT_DRV_OPS_NUM,
		UDI_MEI_REL_INTERNAL | UDI_MEI_REL_BIND,
		udi_mgmt_drv_op_templates
	},
	{
		0, 0, NULL
	}
	
};

static udi_mei_enumeration_rank_func_t udi_mgmt_rank_func;

static udi_ubit8_t udi_mgmt_rank_func(udi_ubit32_t attr_device_match,
                void **attr_value_list)
{
        udi_assert(0);
	return 0;
}


udi_mei_init_t udi_meta_info = {
	mgmt_ops_vec_templates_list,
	udi_mgmt_rank_func
};

/*
 * ------------------------------------------------------------------------
 * "Proxies" 
 * ------------------------------------------------------------------------
 */

void
udi_enumerate_no_children(
	udi_enumerate_cb_t *cb,
	udi_ubit8_t enumeration_level)
{
	udi_enumerate_ack(cb, UDI_ENUMERATE_LEAF, 0);
}

void
udi_static_usage(
	udi_usage_cb_t *cb,
	udi_ubit8_t resource_level)
{
	cb->trace_mask = 0;
	udi_usage_res(cb);
}

/*
 *  Indicate desired resource usage and trace levels.
 */
UDI_MEI_STUBS(udi_usage_ind, udi_usage_cb_t, 
	      1, (resource_level), (udi_ubit8_t), (UDI_VA_UBIT8_T),
	      UDI_MGMT_DRV_OPS_NUM, UDI_USAGE_IND)

/*
 * Resource usage and trace level response operation
 */
UDI_MEI_STUBS(udi_usage_res, udi_usage_cb_t, 
	      0, (), (), (),
	      UDI_MGMT_MA_OPS_NUM, UDI_USAGE_RES)


/*
 * Request information regarding a child instance.
 */
UDI_MEI_STUBS(udi_enumerate_req, udi_enumerate_cb_t, 
	      1, (enumeration_level), (udi_ubit8_t), (UDI_VA_UBIT8_T),
	      UDI_MGMT_DRV_OPS_NUM, UDI_ENUMERATE_REQ)

/*
 * Provide child instance information
 */
UDI_MEI_STUBS(udi_enumerate_ack, udi_enumerate_cb_t, 
	      2, (enumeration_level, ops_idx),
		 (udi_ubit8_t, udi_index_t),
		 (UDI_VA_UBIT8_T, UDI_VA_INDEX_T),
	      UDI_MGMT_MA_OPS_NUM, UDI_ENUMERATE_ACK)

/*
 * Device Management request.
 */     

UDI_MEI_STUBS(udi_devmgmt_req, udi_mgmt_cb_t, 
	      2, (mgmt_op, parent_id),
		 (udi_ubit8_t, udi_ubit8_t),
		 (UDI_VA_UBIT8_T, UDI_VA_UBIT8_T),
	      UDI_MGMT_DRV_OPS_NUM, UDI_DEVMGMT_REQ)

/*
 * Acknowledge a device management request.
 */

UDI_MEI_STUBS(udi_devmgmt_ack, udi_mgmt_cb_t,
	      2, (flags, status),
		 (udi_ubit8_t, udi_status_t),
		 (UDI_VA_UBIT8_T, UDI_VA_STATUS_T),
	      UDI_MGMT_MA_OPS_NUM, UDI_DEVMGMT_ACK)

/*
 * Request driver to release all resources and context prior
 * to instance unload.
 */

UDI_MEI_STUBS(udi_final_cleanup_req, udi_mgmt_cb_t, 
	      0, (), (), (),
	      UDI_MGMT_DRV_OPS_NUM, UDI_FINAL_CLEANUP_REQ)

/*
 *  Acknowledge completion of a final cleanup request.
 */

UDI_MEI_STUBS(udi_final_cleanup_ack, udi_mgmt_cb_t,
	      0, (), (), (),
	      UDI_MGMT_MA_OPS_NUM, UDI_FINAL_CLEANUP_ACK)
