
/*
 * File: env/common/udi_alloc.h
 *
 * UDI marshalling for asynchronous service calls definitions.
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


/* This is #included into udi_env.h, but was broken out for legibility. */

#ifndef _UDI_ALLOC_H
#define _UDI_ALLOC_H

/*
 * Special states for alloc control blocks.
 */
typedef enum {
	_UDI_ALLOC_IDLE,		/* Control Block is idle (in driver) */
	_UDI_ALLOC_PENDING,		/* Allocation pending */
	_UDI_ALLOC_BUSY,		/* Allocation in progress (in daemon) */
	_UDI_ALLOC_DONE,		/* Allocation complete (queued to region) */
	_UDI_ALLOC_IMMED_DONE,		/* Immed callback (queued to region) */
	_UDI_ALLOC_TIMER,		/* Pending timer request */
	_UDI_ALLOC_TIMER_DONE,		/* Fired timer request (queued to region) */
	_UDI_ALLOC_CANCEL		/* Cancelling a pending allocation */
} _udi_alloc_state_t;

/*
 * Asynchronous call types, to select appropriate callback prototype.
 * These are arbitrary unique numbers and are not used as array indices.
 */
#define _UDI_CT_CANCEL		 	 0	/* udi_cancel */
#define _UDI_CT_CB_ALLOC		 1	/* udi_cb_alloc */
#define _UDI_CT_MEM_ALLOC		 2	/* udi_mem_alloc */
#define _UDI_CT_BUF_PATH_ALLOC	 	 3	/* udi_dma_buf_path_alloc */
#define _UDI_CT_BUF_READ		 4	/* udi_buf_read */
#define _UDI_CT_BUF_COPY		 5	/* udi_buf_copy */
#define _UDI_CT_BUF_WRITE		 6	/* udi_buf_write */
#define _UDI_CT_BUF_XINIT		 7	/* udi_buf_write */
#define _UDI_CT_BUF_TAG			 8	/* udi_buf_tag_set and
						 * udi_buf_tag_apply */
#define _UDI_CT_INSTANCE_ATTR_GET	 9	/* udi_instance_attr_get */
#define _UDI_CT_INSTANCE_ATTR_SET	10	/* udi_instance_attr_set */
#define _UDI_CT_CHANNEL_ANCHOR		11	/* udi_channel_anchor */
#define _UDI_CT_CHANNEL_SPAWN		12	/* udi_channel_spawn */
#ifdef _UDI_PHYSIO_SUPPORTED
#define _UDI_CT_CONSTRAINTS_ATTR_SET	13	/* udi_dma_constraints_attr_set */
#define _UDI_CT_DMA_PREPARE	 	14	/* udi_dma_prepare */
#define _UDI_CT_DMA_BUF_MAP		15	/* udi_dma_buf_map */
#define _UDI_CT_DMA_MEM_ALLOC		16	/* udi_dma_mem_alloc */
#define _UDI_CT_DMA_SYNC		17	/* udi_dma_sync */
#define _UDI_CT_DMA_SCGTH_SYNC		18	/* udi_dma_scgth_sync */
#define _UDI_CT_DMA_MEM_TO_BUF		19	/* udi_dma_mem_to_buf */
#define _UDI_CT_PIO_MAP			20	/* udi_pio_map */
#define _UDI_CT_PIO_TRANS		21	/* udi_pio_trans */
#define _UDI_CT_PIO_PROBE		22	/* udi_pio_probe */
#endif /* _UDI_PHYSIO_SUPPORTED */

/* Additional callback types that don't go through the alloc daemon: */
#define _UDI_CT_TIMER_START		23	/* udi_timer_start */
#define _UDI_CT_TIMER_START_REPEATING	24	/* udi_timer_start_repeating */
#define _UDI_CT_LOG_WRITE               25	/* udi_log_write */

/* queue a generic function to a region */
#define _UDI_CT_QUEUE_TO_REGION         26	/* _UDI_QUEUE_TO_REGION callback */

/*
 * Allocation state management structure used in control blocks to keep track
 * of parameters and internal state during deferred allocations.
 */
typedef struct _udi_alloc_marshal {
	_udi_alloc_state_t alloc_state;	/* State of control block */
	const struct _udi_alloc_ops *alloc_ops;	/* Environment-provided 
						 * allocation function 
						 * ops vector */
	/*
	 * Parameter and result marshalling for various alloc types 
	 */
	/* *INDENT-OFF* */
	union {
	     /* Size of memory needed for any request */
		udi_size_t req_size;

	     /*
	      * Control Block Request Structures.
	      * 
	      * If a request can't be satisfied in the driver's context the 
	      * request parameters are marshalled into the call_type's 
	      * request structure in the control block and queued to the alloc 
	      * daemon for handling.
	      *
	      * When the request has been satisfied, the results (the
	      * callback parameters) are marshalled into the result structure
	      * in the control block and queued to the driver's region.
	      *
	      * Additionally, even if the request was satisfied in the
	      * driver's context, the current callback depth may exceed the
	      * environment's internal limit, in which case the results are
	      * also marshalled into the result structure and queued to the
	      * driver's region for handling when the callback stack unwinds.
	      * 
	      * For easy tracking, these are listed by call_type in the order
	      * they're listed in the call type definitions above. The
	      * request structures are all listed in sequence first, followed
	      * by the result structures. If no corresponding structure is
	      * needed for a given call_type, the call_type is listed with a 
	      * comment to that effect.
	      *
	      * Note that in many cases, the structs that make up members of
	      * this union contain fields that are overlaid on fields in other
	      * union members. This is done in cases where the overlaid values
	      * are still needed after specific fields in the new member are
	      * set; overlaying them allows the values to be retained.
	      */
	     /*
	      * -------------------------------
	      * Control block request structures.
	      * -------------------------------
	      */
	     /* _UDI_CT_CANCEL - no specific request arguments */
	     /* _UDI_CT_CB_ALLOC request */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			struct _udi_cb *orig_cb;
			struct _udi_mei_cb_template *cb_template;
			udi_index_t cb_idx;
			udi_channel_t target_channel; 
		} cb_alloc_request;
	     /* _UDI_CT_MEM_ALLOC - no additional request */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			udi_ubit8_t flags;
		} mem_alloc_request;
	     /* _UDI_CT_BUF_PATH_ALLOC request */
	     /* _UDI_CT_BUF_READ request */
		/* Currently, there are no cases of delayed udi_buf_read */
	     /* _UDI_CT_BUF_WRITE request */
	     /* _UDI_CT_BUF_COPY request */
		struct _buf_wrcp_request {
			udi_size_t _req_size;	/* overlaid on req_size */
			udi_size_t data_size;	/* (pre-computed) */
#if _UDI_PHYSIO_SUPPORTED
			_udi_dma_constraints_t *constraints;
#endif /* _UDI_PHYSIO_SUPPORTED */
			_udi_buf_path_t	*buf_path;
			void *src;
			udi_size_t src_off;
			udi_size_t src_len;
			struct _udi_buf *dst_buf;
			udi_size_t dst_off;
			udi_size_t dst_len;
			udi_ubit16_t cont_type;
			udi_ubit16_t req_type;
			udi_size_t hdr_alloc;
			udi_size_t mem_alloc;
			udi_size_t phy_mem;
			udi_size_t vir_mem;
			udi_size_t bh_alloc;
			udi_size_t bt_alloc;
			void *fmem_alloc;
		} buf_wrcp_request;
	     /* _UDI_CT_BUF_XINIT request */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			void *src_mem;
			udi_size_t src_len;
			_udi_xbops_t *xbops;
			void *xbops_context;
			udi_buf_path_t buf_path;
		} buf_xinit_request;
	     /* _UDI_CT_BUF_TAG request */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			struct _udi_buf *buf;
			udi_buf_tag_t *tag;
			udi_tagtype_t tag_type;
			udi_ubit8_t req_type;
			udi_ubit16_t remain;
		} buf_tag_request;
	     /* _UDI_CT_INSTANCE_ATTR_GET request */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			_udi_dev_node_t *node;
			udi_instance_attr_get_call_t *callback;
			const char *name;
			void *value;
			udi_size_t length;
		} instance_attr_get_request;
	     /* _UDI_CT_INSTANCE_ATTR_SET request */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			_udi_dev_node_t *node;
			udi_instance_attr_set_call_t *callback;
			const char *name;
			const void *value;
			udi_size_t length;
			udi_ubit8_t attr_type;
			udi_boolean_t persistent;
		} instance_attr_set_request;
	     /* _UDI_CT_CHANNEL_ANCHOR request */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			_udi_channel_t *channel;
			udi_index_t ops_idx;
			void *channel_context;
		} channel_anchor_request;
	     /* _UDI_CT_CHANNEL_SPAWN request */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			_udi_channel_t *channel;
			udi_index_t spawn_idx;
			udi_index_t ops_idx;
			void *channel_context;
		} channel_spawn_request;

#ifdef _UDI_PHYSIO_SUPPORTED
	     /* _UDI_CT_CONSTRAINTS_ATTR_SET request */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			_udi_dma_constraints_t *src_constr;
			const udi_dma_constraints_attr_spec_t *attr_list;
			udi_ubit16_t list_length;
			udi_ubit8_t flags;
		} constraints_attr_set_request;
	     /* _UDI_CT_DMA_PREPARE request */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			_udi_dma_constraints_t *constraints;
			udi_ubit8_t flags;
			udi_cb_t *orig_cb;
		} dma_prepare_request;
	     /* _UDI_CT_DMA_BUF_MAP request */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			_udi_dma_handle_t *dmah;
			udi_ubit8_t state;
		} dma_buf_map_request;
	     /* _UDI_CT_DMA_MEM_ALLOC request */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			_udi_dma_constraints_t *constraints;
			udi_ubit8_t flags;
			udi_ubit16_t nelements;
			udi_size_t element_size;
			udi_cb_t *gcb;
		} dma_mem_alloc_request;
	     /* _UDI_CT_DMA_SYNC request */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			udi_dma_handle_t dma_handle;
			udi_size_t offset;
			udi_size_t length;
			udi_ubit8_t flags;
		} dma_sync_request;
	     /* _UDI_CT_DMA_SCGTH_SYNC request */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			udi_dma_handle_t dma_handle;
		} dma_scgth_sync_request;
	     /* _UDI_CT_DMA_MEM_TO_BUF request */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			_udi_dma_handle_t *dmah;
			udi_size_t src_off;
			udi_size_t src_len;
			udi_ubit8_t type;
		} dma_mem_to_buf_request;

	     /* _UDI_CT_PIO_MAP request */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			struct _udi_cb *orig_cb;
			udi_ubit32_t regset_idx;
			udi_ubit32_t offset;
			udi_ubit32_t length;
			udi_pio_trans_t *trans_list;
			udi_ubit16_t trans_length;
			udi_ubit16_t attributes;
			udi_ubit32_t pace;
			udi_index_t serialization_domain;
		} pio_map_request;
	     /*  _UDI_CT_PIO_TRANS request */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			udi_pio_trans_call_t *callback;
			struct _udi_cb *orig_cb;
			_udi_pio_handle_t *handle;
			udi_buf_t *buf;
			void *mem_ptr;
		} pio_trans_request;
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			udi_pio_probe_call_t *callback;
			struct _udi_cb *orig_cb;
			_udi_pio_handle_t *handle;
			void *mem_ptr;
			udi_ubit32_t pio_offset;
			udi_ubit8_t tran_size;
			udi_ubit8_t direction;
		} pio_probe_request;
#endif /* _UDI_PHYSIO_SUPPORTED */

	     /* _UDI_CT_TIMER request,
		_UDI_CT_TIMER_REPEATING request */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			udi_ubit32_t interval_ticks;
			udi_sbit32_t ticks_left; /* May go negative. */
			udi_ubit32_t start_time;
		} timer_request;
	     /* _UDI_CT_CB_ALLOC request */
		struct cb_alloc_batch_request_t{
			udi_size_t _req_size;	/* overlaid on req_size */
			struct _udi_cb *orig_cb;
			struct _udi_mei_cb_template *cb_template;
			udi_index_t cb_idx;
			udi_channel_t target_channel; 
			udi_cb_alloc_batch_call_t *callback;
			udi_index_t count;
			udi_boolean_t with_buf;
			udi_size_t buf_size;
			udi_buf_path_t path_handle;
		} cb_alloc_batch_request;

	     /*
	      * --------------------------------------
	      * Control block result structures.
	      * --------------------------------------
	      */
	     /* _UDI_CT_CANCEL - no specific result arguments */
	     /* _UDI_CT_CB_ALLOC result */
		struct {
			udi_cb_t *new_cb;
		} cb_alloc_result;
	     /* _UDI_CT_MEM_ALLOC result */
		struct {
			void *new_mem;
		} mem_alloc_result;
	     /* _UDI_CT_CONSTRAINTS_PROPAGATE*/
		struct {
			udi_buf_path_t new_buf_path;
		} buf_path_alloc_result;
	     /* _UDI_CT_BUF_COPY result,
	        _UDI_CT_BUF_WRITE result,
		_UDI_CT_BUF_XINIT result */
		struct {
			udi_buf_t *new_buf;
		} buf_result;
	     /* _UDI_CT_BUF_TAG result */
		struct {
			udi_buf_t *new_buf;
		} buf_tag_result;
	     /* _UDI_CT_INSTANCE_ATTR_GET result */
		struct {
			udi_instance_attr_type_t attr_type;
			udi_size_t actual_length;
		} instance_attr_get_result;
	     /* _UDI_CT_INSTANCE_ATTR_SET result */
		struct {
			udi_status_t status;
		} instance_attr_set_result;
	     /* _UDI_CT_CHANNEL_ANCHOR result,
	        _UDI_CT_CHANNEL_SPAWN result */
		struct {
			_udi_channel_t *new_channel;
		} channel_result;

#ifdef _UDI_PHYSIO_SUPPORTED
	     /* _UDI_CT_CONSTRAINTS_ATTR_SET result */
		struct {
			udi_dma_constraints_t new_constraints;
			udi_status_t status;
			udi_boolean_t new_memory; /* internal flag */
		} constraints_attr_set_result;
	     /* _UDI_CT_DMA_PREPARE result */
		struct {
			udi_dma_handle_t new_dma_handle;
		} dma_prepare_result;
	     /* _UDI_CT_DMA_BUF_MAP result */
		struct {
			/* 
			 * The next 2 fields are here so we can get at
			 * the dma_handle if we need to discard associated
			 * resources, even after storing results. */
			udi_size_t _req_size;	/* overlaid on req_size */
			_udi_dma_handle_t *dmah; /* overlaid on
						   dma_buf_map_request.dmah */
			udi_ubit8_t state;	/* overlaid on
						   dma_buf_map_request.state */
			udi_scgth_t *scgth;
			udi_boolean_t complete;
			udi_status_t status;
		} dma_buf_map_result;
	     /* _UDI_CT_DMA_MEM_ALLOC result */
		struct {
			udi_dma_handle_t new_dma_handle;
			void *mem_ptr;
			udi_size_t actual_gap;
			udi_boolean_t single_element;
			udi_scgth_t *scgth;
			udi_boolean_t must_swap;
		} dma_mem_alloc_result;
	     /* _UDI_CT_DMA_SYNC - no specific result arguments */
	     /* _UDI_CT_DMA_SCGTH_SYNC - no specific result arguments */
	     /* _UDI_CT_DMA_MEM_TO_BUF result */
		struct {
			udi_buf_t *new_buf;
		} dma_mem_to_buf_result;

	     /* _UDI_CT_PIO_MAP result */
		struct {
			udi_pio_handle_t new_handle;
		} pio_map_result;
	     /* _UDI_CB_PIO_TRANS result */
		struct {
			udi_buf_t *new_buf;
			udi_status_t status;
			udi_ubit16_t result;
		} pio_trans_result;
	     /* _UDI_CT_PIO_PROBE result */
		struct {
			udi_status_t status;
		} pio_probe_result;
#endif /* _UDI_PHYSIO_SUPPORTED */

	     /* _UDI_CT_TIMER - no specific result arguments */
	     /* _UDI_CT_TIMER_REPEATING result */
		struct {
			udi_size_t _req_size;	/* overlaid on req_size */
			udi_ubit32_t interval_ticks; /* from timer_req */
		} timer_result;
	     /* _UDI_CT_QUEUE_TO_REGION */
	        struct {
		        void *param;
		} generic_param;
	} _u;
	/* *INDENT-ON* */
} _udi_alloc_marshal_t;

/* 
 * --------------------------------------------------------------------------
 * The following CALLBACK_ARGS and SET_RESULT macros are used together in
 * _UDI_IMMED_CALLBACK to set the appropriate result values in the control
 * block in the case where we're queuing to the region because callback depth
 * has been exceeded.
 * --------------------------------------------------------------------------
 */

/*
 * CALLBACK_ARGS macros. These turn a list of arguments that were passed
 * to an outer macro as a single argument (and are thus in the form of a
 * comma-separated list enclosed in parentheses) into a list of discrete
 * arguments (i.e. no parentheses).
 */
#define _UDI_CALLBACK_ARGS_0
#define _UDI_CALLBACK_ARGS_1(a)			a
#define _UDI_CALLBACK_ARGS_2(a,b)		a,b
#define _UDI_CALLBACK_ARGS_3(a,b,c)		a,b,c
#define _UDI_CALLBACK_ARGS_4(a,b,c,d)		a,b,c,d
#define _UDI_CALLBACK_ARGS_5(a,b,c,d,e)		a,b,c,d,e
#define _UDI_CALLBACK_ARGS_6(a,b,c,d,e,f)	a,b,c,d,e,f
#define _UDI_CALLBACK_ARGS_7(a,b,c,d,e,f,g)	a,b,c,d,e,f,g

/* 
 * CALLBACK_ARGLIST macros expand as CALL_BACK_ARGS, but attach a comma 
 * to the initial token making them suitable for vararg-like argument
 * lists.  These will expand a comma separated list of arguments into
 * a list of discrete arguments (think preprocessing tokens) with a 
 * leading comma if appropriate.
 */
#define _UDI_CALLBACK_ARGLIST_0(a)
#define _UDI_CALLBACK_ARGLIST_1(a)                 ,a
#define _UDI_CALLBACK_ARGLIST_2(a,b)               ,a,b
#define _UDI_CALLBACK_ARGLIST_3(a,b,c)             ,a,b,c
#define _UDI_CALLBACK_ARGLIST_4(a,b,c,d)           ,a,b,c,d
#define _UDI_CALLBACK_ARGLIST_5(a,b,c,d,e)         ,a,b,c,d,e
#define _UDI_CALLBACK_ARGLIST_6(a,b,c,d,e,f)       ,a,b,c,d,e,f
#define _UDI_CALLBACK_ARGLIST_7(a,b,c,d,e,f,g)     ,a,b,c,d,e,f,g

/*
 * SET_RESULT macros. These macros set the appropriate result values in
 *  the allocation marshalling structure for each call type.
 * 
 * These will always be called with at least two arguments, even though
 * the second argument my be empty for the case where we have no results
 * to set.   This is to eliminate further obfuscation in the  
 *  _UDI_IMMEDIATE_CALLBACK macro while still allowing that argument to
 * be preprocessed away and thus generating zero code.
 */
#define SET_RESULT_UDI_CT_CANCEL(allocm, ignored) \
					/* No result to set */
#define SET_RESULT_UDI_CT_CB_ALLOC(allocm, _new_cb) \
	    (allocm)->_u.cb_alloc_result.new_cb = _new_cb
#define SET_RESULT_UDI_CT_BUF_PATH_ALLOC(allocm, _new_buf_path) \
	    (allocm)->_u.buf_path_alloc_result.new_buf_path = _new_buf_path
#define SET_RESULT_UDI_CT_BUF_COPY(allocm, _new_buf) \
	    (allocm)->_u.buf_result.new_buf = _new_buf
#define SET_RESULT_UDI_CT_BUF_WRITE(allocm, _new_buf) \
	    (allocm)->_u.buf_result.new_buf = _new_buf
#define SET_RESULT_UDI_CT_BUF_XINIT(allocm, _new_buf) \
	    (allocm)->_u.buf_result.new_buf = _new_buf
#define SET_RESULT_UDI_CT_BUF_TAG(allocm, _new_buf) \
	    (allocm)->_u.buf_result.new_buf = _new_buf
#define SET_RESULT_UDI_CT_MEM_ALLOC(allocm, _new_mem) \
	    (allocm)->_u.mem_alloc_result.new_mem = _new_mem
#define SET_RESULT_UDI_CT_CHANNEL_ANCHOR(allocm, anchored_channel) \
	    (allocm)->_u.channel_result.new_channel = anchored_channel
#define SET_RESULT_UDI_CT_CHANNEL_SPAWN(allocm, _new_channel) \
	    (allocm)->_u.channel_result.new_channel = _new_channel
#define SET_RESULT_UDI_CT_INSTANCE_ATTR_GET(allocm, _attr_type, _attr_length) \
	    (allocm)->_u.instance_attr_get_result.attr_type = _attr_type; \
	    (allocm)->_u.instance_attr_get_result.actual_length = _attr_length
#define SET_RESULT_UDI_CT_INSTANCE_ATTR_SET(allocm, _status) \
	    (allocm)->_u.instance_attr_set_result.status = _status
#ifdef _UDI_PHYSIO_SUPPORTED
#define SET_RESULT_UDI_CT_CONSTRAINTS_ATTR_SET(allocm, _new_constr, \
			_status) \
	    (allocm)->_u.constraints_attr_set_result.new_constraints = \
			_new_constr; \
	    (allocm)->_u.constraints_attr_set_result.status = _status
#define SET_RESULT_UDI_CT_DMA_PREPARE(allocm, _new_dma_handle) \
	    (allocm)->_u.dma_prepare_result.new_dma_handle = _new_dma_handle
#define SET_RESULT_UDI_CT_DMA_BUF_MAP(allocm, _scgth, _complete, \
					_status) \
	    (allocm)->_u.dma_buf_map_result.scgth = _scgth; \
	    (allocm)->_u.dma_buf_map_result.complete = _complete; \
	    (allocm)->_u.dma_buf_map_result.status = _status
#define SET_RESULT_UDI_CT_DMA_MEM_ALLOC(allocm, _new_dma_handle, _mem_ptr, \
					_actual_gap, _single_elmnt, _scgth, \
					_must_swap) \
	    (allocm)->_u.dma_mem_alloc_result.new_dma_handle = \
							_new_dma_handle; \
	    (allocm)->_u.dma_mem_alloc_result.mem_ptr = _mem_ptr; \
	    (allocm)->_u.dma_mem_alloc_result.actual_gap = _actual_gap; \
	    (allocm)->_u.dma_mem_alloc_result.single_element = _single_elmnt; \
	    (allocm)->_u.dma_mem_alloc_result.scgth = _scgth; \
	    (allocm)->_u.dma_mem_alloc_result.must_swap = _must_swap;
#define SET_RESULT_UDI_CT_DMA_SYNC(allocm, ignored) \
					/* No result to set */
#define SET_RESULT_UDI_CT_DMA_SCGTH_SYNC(allocm, ignored) \
					/* No result to set */
#define SET_RESULT_UDI_CT_DMA_MEM_TO_BUF(allocm, _new_buf) \
	    (allocm)->_u.dma_mem_to_buf_result.new_buf = _new_buf
#define SET_RESULT_UDI_CT_PIO_MAP(allocm, pio_handle) \
	    (allocm)->_u.pio_map_result.new_handle = pio_handle
#define SET_RESULT_UDI_CT_PIO_TRANS(allocm, new_buf, status, result) \
	    (allocm)->_u.pio_trans_result.new_buf = new_buf; \
	    (allocm)->_u.pio_trans_result.status = status; \
	    (allocm)->_u.pio_trans_result.result = result
#define SET_RESULT_UDI_CT_PIO_PROBE(allocm, status) \
	    (allocm)->_u.pio_probe_result.status = status;
#endif /* _UDI_PHYSIO_SUPPORTED */

/*
 * _UDI_DO_CALLBACK macro.
 * 
 * Paramaters:
 *   callback         - function pointer
 *   cb               - (_udi_cb_t *) internal control block
 *   callback_args    - varying length list of arguments to callback.  This
 *                      list must include the leading comma if there is a 
 *                      non-zero number of callback_args.
 */

#define _UDI_DO_CALLBACK(callback, cb, callback_args) \
	(*callback) (_UDI_HEADER_TO_CB(cb) callback_args)

/*
 * _UDI_SET_RESULT macro.
 *
 * Paramaters:
 * call_type		- pre-expanded SET_RESULT_UDI_CT_xxx macro.
 * arg1			- where to store results.  Typically allocm.
 * callback_args	- comma separated list of arguments to pass 
 *                        to SET_RESULT_UDI_CT_xxx.
 * 
 * Yes, this looks rather silly, but the idea is to defer tokenization
 * so that the preprocessor won't helpfully retokenize the expanded 
 * argstostore argument.
 */
#define _UDI_SET_RESULT(call_type, arg1, argstostore) \
	call_type (arg1, argstostore)

/* 
 * _UDI_IMMED_CALLBACK macro.
 *
 * Parameters:
 *   cb			-  (_udi_cb_t *) control block
 *   call_type		-  (udi_index_t) must be a mnemonic constant
 *   callback		-  function pointer (of the specific callback type)
 *   callback_args	-  (list of arguments to the callback)
 *   argc		-  number of arguments in callback_args
 * 
 * Synchronization note:  we can only attempt an "immediate" callback 
 * when the region is active (ie. we're in a UDI service call on the 
 * driver's stack), which allows the reg_callback_depth to be modified
 * here (and only here) without the need for any locking around it.
 */
#define _UDI_IMMED_CALLBACK(cb, call_type, ops, callback, argc, callback_args) {  \
        _udi_region_t *_rp = _UDI_CB_TO_CHAN(cb)->chan_region;               \
        _udi_alloc_marshal_t *allocm = &(cb)->cb_allocm;                     \
        _OSDEP_ASSERT(allocm->alloc_state == _UDI_ALLOC_IDLE);               \
        if (_rp->reg_callback_depth < _OSDEP_CALLBACK_LIMIT) {               \
            _rp->reg_callback_depth++;                                       \
            _UDI_DO_CALLBACK(callback, cb,				     \
			_UDI_CALLBACK_ARGLIST_##argc callback_args );	     \
            _rp->reg_callback_depth--;                                       \
        } else {                                                             \
            /* _OSDEP_PRINTF("IMMED_CALLBACK: call-back depth exceeded.\n"); */ \
	    (cb)->cb_flags |= _UDI_CB_CALLBACK;				\
            (cb)->op_idx = call_type;					\
            (cb)->cb_allocm.alloc_ops = (ops);				\
	    _UDI_SET_RESULT(SET_RESULT##call_type ,allocm, 		\
                        _UDI_CALLBACK_ARGS_##argc callback_args);	\
            (cb)->cb_func = (_udi_callback_func_t *)callback;		\
            allocm->alloc_state = _UDI_ALLOC_IMMED_DONE;		\
            _udi_queue_callback(cb);					\
        }                                                               \
}

/* 
 * States for udi_alloc_block flag.  (Essentially boolean, but implicit '0'
 * is invalid.)
 */
typedef enum {
	_UDI_ALLOC_COMPLETE_MIGHT_BLOCK = 1,
	_UDI_ALLOC_COMPLETE_WONT_BLOCK = 2
} _udi_alloc_block_t;

/*
 * Type-specific allocation function ops vector.
 * These ops are called to handle various state transitions during processing
 * and/or cancellation of a deferred asynchronous service call.
 */
typedef const struct _udi_alloc_ops {
	void (*alloc_complete) (_udi_alloc_marshal_t *,
				void *new_mem);
	void (*alloc_discard) (_udi_alloc_marshal_t *);
	void (*alloc_discard_incoming) (_udi_alloc_marshal_t *);
	_udi_alloc_block_t alloc_may_block;
} _udi_alloc_ops_t;

#define _UDI_ALLOC_COMPLETE(allocm, new_mem) \
	(*(allocm)->alloc_ops->alloc_complete) (allocm, new_mem)
#define _UDI_ALLOC_DISCARD(allocm) \
	(*(allocm)->alloc_ops->alloc_discard) (allocm)
#define _UDI_ALLOC_DISCARD_INCOMING(allocm) \
	(*(allocm)->alloc_ops->alloc_discard_incoming) (allocm)

void _udi_alloc_no_incoming(_udi_alloc_marshal_t *allocm);

extern _udi_alloc_ops_t _udi_alloc_ops_nop;

/*
 * Queue an async service call on the alloc queue for deferred processing.
 */
#define _UDI_ALLOC_QUEUE(cb, call_type, callback, size, ops) { \
        (cb)->op_idx = (call_type); \
        (cb)->cb_func = (_udi_callback_func_t *) (callback); \
        (cb)->cb_allocm._u.req_size = (size); \
        (cb)->cb_allocm.alloc_ops = (ops); \
        _OSDEP_ASSERT(((ops)->alloc_may_block == \
				_UDI_ALLOC_COMPLETE_MIGHT_BLOCK) || \
            ((ops)->alloc_may_block == _UDI_ALLOC_COMPLETE_WONT_BLOCK)); \
        _udi_alloc_queue(cb); \
}

/*
 * Queue a generic callback to a region
 *
 *  xxcb is the cb to be used for the callback,
 *  xxcallback is the callback function,
 *  xxparam is a void* parameter to the callback function,
 *  xxdefer if TRUE will cause the callback to be called from the region
 *  daemon rather than directly via _udi_queue_callback
 */
#define _UDI_QUEUE_TO_REGION(xxcb, xxcallback, xxparam, xxdefer) do { \
	(xxcb)->op_idx = _UDI_CT_QUEUE_TO_REGION; \
	(xxcb)->cb_flags |= _UDI_CB_CALLBACK; \
	(xxcb)->cb_func = (_udi_callback_func_t *)(xxcallback); \
	(xxcb)->cb_allocm._u.generic_param.param = (xxparam); \
	if (xxdefer) { \
		_udi_region_t *region = _UDI_CB_TO_CHAN(xxcb)->chan_region; \
		REGION_LOCK(region); \
		_UDI_REGION_Q_APPEND(region, &xxcb->cb_qlink); \
		if (!REGION_IS_ACTIVE(region) && !REGION_IN_SCHED_Q(region)) { \
			_OSDEP_REGION_SCHEDULE(region); \
		} else { \
			REGION_UNLOCK(region); \
		} \
	} else { \
		_udi_queue_callback(xxcb); \
	} \
} while(0);

void _udi_alloc_queue(struct _udi_cb *cb);

void _udi_alloc_init(void);

udi_boolean_t _udi_alloc_daemon_work(void);

udi_cb_t *_udi_cb_alloc_waitok(struct _udi_cb *orig_cb,
			       _udi_mei_cb_template_t *cb_template,
			       udi_channel_t default_channel);

#endif /* _UDI_ALLOC_H */
