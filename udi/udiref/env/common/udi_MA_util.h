
/*
 * File: env/common/udi_MA_util.c
 *
 * Various utility functions that the MA uses.
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

#ifndef _UDI_MA_UTIL_H
#define _UDI_MA_UTIL_H

#define _UDI_MA_USAGE_OP 1
#define _UDI_MA_EV_OP 2
#define _UDI_MA_ENUM_OP 3
#define _UDI_MA_DEVMGMT_OP 4
#define _UDI_MA_CLEANUP_OP 5
#define _UDI_MA_CB_ALLOC_OP 6
#define _UDI_MA_MEM_ALLOC_OP 7
#define _UDI_MA_CB_ALLOC_BATCH_OP 8


typedef struct {
	udi_queue_t link;		/* link to the _udi_MA_op_q */
	udi_index_t op_type;		/* as defined above */
	udi_size_t scratch_size;	/* size of scratch */
	union {
		udi_mgmt_cb_t mgmt;
		udi_usage_cb_t usage;
		udi_enumerate_cb_t enu;
		udi_channel_event_cb_t chan_ev;
	} op_cb;
	union {
		struct {
			udi_index_t level;
		} usage;
		struct {
			udi_cb_alloc_call_t *callback;
			udi_index_t cb_idx;
			udi_channel_t default_channel;
		} cb_alloc;
		struct {
			udi_ubit8_t enum_level;
		} enu;
		struct {
			udi_mem_alloc_call_t *callback;
			udi_size_t size;
			udi_ubit8_t flags;
		} mem_alloc;
		struct {
			udi_ubit8_t mgmt_op;
			udi_ubit8_t parent_ID;
		} mgmt;
		struct {
			udi_cb_alloc_call_t *callback;
			udi_index_t cb_idx;
			udi_channel_t default_channel;
		} cb_alloc_batch;
	} op_param;
} _udi_MA_op_q_entry_t;


extern udi_MA_ops_t _udi_MA_ops;
extern void _udi_MA_util_init(void);
extern void _udi_MA_util_deinit(void);
extern void _udi_devmgmt_req(_udi_dev_node_t *child_node,
			     udi_ubit8_t mgmt_op,
			     udi_ubit8_t parent_ID,
			     _udi_devmgmt_context_t *context);
extern void _udi_MA_add_cb_to_pool(void);

#if _UDI_PHYSIO_SUPPORTED
extern void _udi_MA_pio_trans(_udi_unbind_context_t *context,
			      _udi_channel_t *channel,
			      udi_pio_handle_t pio_handle,
			      udi_index_t start_label,
			      udi_buf_t *buf,
			      void *mem_ptr);
#endif

void _udi_MA_send_op(_udi_MA_op_q_entry_t *op);
void _udi_MA_return_cb(udi_MA_cb_t *mcb);
void _udi_channel_close(udi_channel_t channel);

#endif /* _UDI_MA_UTIL_H */
