
/*
 * File: env/common/udi_mgmt_MA.h
 *
 * UDI Management Metalanguage Definitions for Management Agents (MA)
 * These definitions are NOT needed by UDI drivers and are NOT part of
 * the specified programming interfaces.
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

#ifndef _UDI_MGMT_MA_H
#define _UDI_MGMT_MA_H

/*
 * ----------------------------------------------
 * Interfaces used by the Management Agent (MA).  
 * ----------------------------------------------
 */

/* ops_num and cb_num definitions */
#define UDI_MGMT_MA_OPS_NUM	1
#define UDI_MGMT_DRV_OPS_NUM	2
#define UDI_MGMT_CB_NUM		0

/* ops_idx and cb_idx values used by the MA */
#define UDI_MA_MGMT_OPS_IDX	1
#define UDI_MA_MGMT_CB_IDX	1

/*
 * ----------------------------------------------------------------------
 * Internal version of "visible" control blocks that contain extra fields
 * for MA use. These are all union'ed into a single control block group
 * for simplicity and to allow the "event" field to be at the same offset
 * in all control blocks.
 * ----------------------------------------------------------------------
 */

typedef struct udi_MA_cb {
	union {
		udi_mgmt_cb_t m;
		udi_usage_cb_t usg;
		udi_enumerate_cb_t en;
		udi_channel_event_cb_t ce;
	} v;
	udi_queue_t cb_queue;
} udi_MA_cb_t;

/*
 * Private fields used by the MA:
 *	event is used to block waiting for a response from the driver.
 *	The remaining fields are used to hold response parameters.
 * These fields are accessed by indirection through the initiator_context
 * member of the udi_cb_t overlay so that they get propagated through
 * _udi_cb_realloc().
 */

/* _udi_MA_cb_t, _udi_MA_cb_alloc, and _udi_MA_cb_free moved to udi_env.h */

typedef struct _udi_MA_logcb_scratch {
	udi_cb_t *logcb_cb;
	udi_queue_t logcb_q;
} _udi_MA_logcb_t;

typedef struct {
	udi_ubit8_t enum_level;
	void *op;			/* _udi_MA_op_q_entry_t */
} _udi_MA_enum_scratch_t;

typedef struct {
	udi_ubit8_t callback_idx;
	union {
		_udi_MA_logcb_t log;
		_udi_MA_enum_scratch_t enumerate;
	} m;
} _udi_MA_scratch_t;



/*
 * -------------------------------
 * MA Interface Operations types
 * -------------------------------
 */
typedef void udi_usage_res_op_t(udi_usage_cb_t *cb);
typedef void udi_enumerate_ack_op_t(udi_enumerate_cb_t *cb,
				    udi_ubit8_t enumeration_result,
				    udi_index_t ops_idx);
typedef void udi_devmgmt_ack_op_t(udi_mgmt_cb_t *cb,
				  udi_ubit8_t flags,
				  udi_status_t status);
typedef void udi_final_cleanup_ack_op_t(udi_mgmt_cb_t *cb);

/* 
 * ---------------
 * MA ops vector
 * ---------------
 */
typedef const struct {
	udi_usage_res_op_t *usage_res_op;
	udi_enumerate_ack_op_t *enumerate_ack_op;
	udi_devmgmt_ack_op_t *devmgmt_ack_op;
	udi_final_cleanup_ack_op_t *final_cleanup_ack_op;
} udi_MA_ops_t;

/*
 * Function prototypes.
 * These are issued BY the Management Agent (TO the driver).
 */
void udi_usage_ind(udi_usage_cb_t *cb,
		   udi_ubit8_t resource_level);
void udi_enumerate_req(udi_enumerate_cb_t *cb,
		       udi_ubit8_t enumeration_level);
void udi_devmgmt_req(udi_mgmt_cb_t *cb,
		     udi_ubit8_t mgmt_op,
		     udi_ubit8_t parent_ID);
void udi_final_cleanup_req(udi_mgmt_cb_t *cb);


#endif /* _UDI_MGMT_MA_H */
