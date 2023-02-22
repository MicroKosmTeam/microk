
/*
 * File: env/common/udi_pio.c
 *
 * UDI Physical I/O management routines.
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

#include <udi_env.h>

#if _UDI_PHYSIO_SUPPORTED

STATIC void
_udi_pio_map_complete(_udi_alloc_marshal_t *allocm,
		      void *new_mem)
{
	_udi_pio_handle_t *pio_handle = new_mem;
	udi_ubit32_t x = 0x01020304;
	udi_boolean_t little_endian = *(char *)&x == 4;
	udi_status_t prescan;
	_udi_cb_t *cb = allocm->_u.pio_map_request.orig_cb;
	_udi_dev_node_t *node = _UDI_CB_TO_CHAN(cb)->chan_region->reg_node;
	udi_ubit32_t attributes = allocm->_u.pio_map_request.attributes;

	pio_handle->alloc_hdr.ah_type = _UDI_ALLOC_HDR_PIO_MAP;
	_udi_add_alloc_to_tracker(_UDI_CB_TO_CHAN(cb)->chan_region,
				  &pio_handle->alloc_hdr);
	pio_handle->flags = 0;
	pio_handle->regset_idx = allocm->_u.pio_map_request.regset_idx;
	pio_handle->length = allocm->_u.pio_map_request.length;
	pio_handle->offset = allocm->_u.pio_map_request.offset;
	pio_handle->pace = allocm->_u.pio_map_request.pace;
	pio_handle->trans_list = allocm->_u.pio_map_request.trans_list;
	pio_handle->trans_length = allocm->_u.pio_map_request.trans_length;
	pio_handle->access_timestamp = 0;
	pio_handle->scanned_trans = (_udi_pio_trans_t *)
		((char *)new_mem + sizeof (*pio_handle));
	pio_handle->trans_entry_pt[0] = pio_handle->scanned_trans;
	pio_handle->dev_node = node;

	_OSDEP_ASSERT(node != NULL);
	_OSDEP_ASSERT(allocm->_u.pio_map_request.serialization_domain <
		      node->n_pio_mutexen);
	pio_handle->pio_mutex =
		&node->pio_mutexen[allocm->_u.pio_map_request.
				   serialization_domain];

	/*
	 * The 1.0 spec says we default to neverswap if endianness of mapping
	 * is unspecified.   Enforce that here so the trans scanner can 
	 * enforce that no multibyte PIO accesses are present in the list. 
	 */
	if (0 == (attributes & (UDI_PIO_BIG_ENDIAN | UDI_PIO_LITTLE_ENDIAN))) {
		pio_handle->flags |= _UDI_PIO_NEVERSWAP;
	}

	if (!(attributes & UDI_PIO_NEVERSWAP) &&
	    (little_endian ^ (UDI_PIO_LITTLE_ENDIAN ==
			      (attributes & UDI_PIO_LITTLE_ENDIAN))))
		pio_handle->flags |= _UDI_PIO_DO_SWAP;

	if (attributes & UDI_PIO_UNALIGNED) {
		pio_handle->flags |= _UDI_PIO_UNALIGNED;
	}

	_OSDEP_GET_PIO_MAPPING(pio_handle, node);

	/*
	 * Get any information that's expensive to compute done 
	 * here (once per mapping) instead of at run-time.
	 */
	UDI_QUEUE_INIT(&pio_handle->label_list_head);
	prescan = _udi_pio_trans_prescan(pio_handle);
	_OSDEP_ASSERT(UDI_OK == prescan);

	allocm->_u.pio_map_result.new_handle = pio_handle;
}

STATIC void
_udi_pio_map_discard(_udi_alloc_marshal_t *allocm)
{
	udi_pio_unmap(allocm->_u.pio_map_result.new_handle);
}

void
udi_pio_unmap(udi_pio_handle_t pio_handle)
{
	if (pio_handle != UDI_NULL_PIO_HANDLE) {
		_udi_pio_handle_t *handle = pio_handle;

		_udi_rm_alloc_from_tracker(&handle->alloc_hdr);
		_OSDEP_RELEASE_PIO_MAPPING(handle);
		_OSDEP_MEM_FREE(handle);
	}

}


STATIC _udi_alloc_ops_t _udi_pio_map_alloc_ops = {
	_udi_pio_map_complete,
	_udi_pio_map_discard,
	_udi_alloc_no_incoming,
	_UDI_ALLOC_COMPLETE_MIGHT_BLOCK
};


void
udi_pio_map(udi_pio_map_call_t *callback,
	    udi_cb_t *gcb,
	    udi_ubit32_t regset_idx,
	    udi_ubit32_t base_offset,
	    udi_ubit32_t length,
	    udi_pio_trans_t *trans_list,
	    udi_ubit16_t list_length,
	    udi_ubit16_t pio_attributes,
	    udi_ubit32_t pace,
	    udi_index_t serialization_domain)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;

	allocm->_u.pio_map_request.regset_idx = regset_idx;
	allocm->_u.pio_map_request.offset = base_offset;
	allocm->_u.pio_map_request.length = length;
	allocm->_u.pio_map_request.trans_list = trans_list;
	allocm->_u.pio_map_request.trans_length = list_length;
	allocm->_u.pio_map_request.attributes = pio_attributes;
	allocm->_u.pio_map_request.pace = pace;
	allocm->_u.pio_map_request.serialization_domain = serialization_domain;
	allocm->_u.pio_map_request.orig_cb = cb;

	/*
	 * The spec says a trans list must be in memory. It is certainly an
	 * error for the driver writer to pass in a NULL trans_list pointer.
	 */
	_OSDEP_ASSERT(trans_list != NULL);
	/*
	 * The spec says a trans list must end in a END, END_IMM or BRANCH
	 * instruction. Therefore, the list must be of length != 0.
	 */
	_OSDEP_ASSERT(list_length != 0);

	/*
	 * The spec says that trans_lists must end with one of these opcodes.
	 */
	_OSDEP_ASSERT((trans_list[list_length - 1].pio_op == UDI_PIO_END) ||
		      (trans_list[list_length - 1].pio_op == UDI_PIO_END_IMM)
		      || (trans_list[list_length - 1].pio_op ==
			  UDI_PIO_BRANCH));

	/*
	 * Make sure the driver called udi_bus_bind_req before getting a pio
	 * mapping.
	 * If the driver has been bound to the bridge driver, the bind channel
	 * in its dev_node will be set.
	 */
	_OSDEP_ASSERT((_UDI_GCB_TO_CHAN(gcb)->chan_region->reg_node->
		       bind_channel != NULL) &&
		      "You must call udi_bus_bind_req before udi_pio_map!");
	/*
	 * Allocate one _udi_pio_handle_t and a _udi_pio_trans_t array
	 * that's sized identically to the incoming list.   The latter
	 * is used to hold info that we can compute at map-time instead
	 * of at run-time.
	 *
	 * We intentionally do NOT speculatively allocate this here, 
	 * choosing instead to always defer it to the allocation daemon.
	 * This is becuase it's legal to call udi_pio_map from, say, a 
	 * timer callback or or an interrupt event notification and the 
	 * osdep_get_pio_mapping code may have to do Insanely Expensive 
	 * things to build the required mappings that cannot be done 
	 * at such interrupt context.
	 *
	 * Yes, this means we always take an LWP switch on a PIO mapping.
	 * the host OS could look at its current context to see if it is
	 * allowed to block to potentially avoid this, but describing 
	 * this in an OSDEP manner is too icky.
	 */
	_UDI_ALLOC_QUEUE(cb, _UDI_CT_PIO_MAP, callback,
			 sizeof (_udi_pio_handle_t) +
			 sizeof (_udi_pio_trans_t) * list_length,
			 &_udi_pio_map_alloc_ops);
}

void
udi_pio_trans(udi_pio_trans_call_t *callback,
	      udi_cb_t *gcb,
	      udi_pio_handle_t handle,
	      udi_index_t start_label,
	      udi_buf_t *buf,
	      void *mem_ptr)
{
	extern udi_buf_t *_udi_buf_compact(udi_buf_t *src_buf);
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_pio_handle_t *pio_handle = handle;
	udi_buf_t *new_buf;
	udi_status_t status;
	udi_ubit16_t result;

	/*
	 * Find the appropriate mutex for this transaction.  
	 * (Currently a global, could be per bridge.  This should be the
	 * only setting in this file that has to change if you decide to 
	 * move it.)
	 */

	_udi_pio_mutex_t *pio_mutex = pio_handle->pio_mutex;

#if TRACKER
	if (mem_ptr) {
		_udi_alloc_hdr_t *ah;

		ah = _udi_find_alloc_in_tracker(_UDI_CB_TO_CHAN(cb)->
						chan_region, mem_ptr);
		if ((ah == NULL) || (0 == (ah->ah_flags & UDI_MEM_MOVABLE))) {
			if (pio_handle->movable_warning_issued == FALSE) {
				pio_handle->movable_warning_issued = TRUE;
				_udi_env_log_write(_UDI_TREVENT_ENVERR,
						   UDI_LOG_ERROR, 0,
						   UDI_STAT_NOT_UNDERSTOOD,
						   925, pio_handle, mem_ptr);
			}
		}
	}
#endif
	new_buf = _udi_buf_compact(buf);

	/*
	 * If this one is busy, preserve call state in this control block,
	 * queue it onto this pio queue, and return quickly.
	 * The locks are held only whild doing the test if it's busy and
	 * while adding it to the list.
	 */
#if 0

/* Turned off while bugs are worked out. */
	_OSDEP_SIMPLE_MUTEX_LOCK(&pio_mutex->lock);

	if (pio_mutex->active) {
		cb->cb_allocm._u.pio_trans_request.callback = callback;
		cb->cb_allocm._u.pio_trans_request.handle = pio_handle;
		cb->cb_allocm._u.pio_trans_request.buf = buf;
		cb->cb_allocm._u.pio_trans_request.mem_ptr = mem_ptr;
		UDI_ENQUEUE_TAIL(&pio_mutex->pio_queue, &cb->cb_qlink);
		_OSDEP_ASSERT(pio_mutex->active);
		_OSDEP_SIMPLE_MUTEX_UNLOCK(&pio_mutex->lock);
		return;
	}

	pio_mutex->active = 1;
	_OSDEP_SIMPLE_MUTEX_UNLOCK(&pio_mutex->lock);
#endif

	_OSDEP_MUTEX_LOCK(&pio_mutex->lock);
	status = _udi_execute_translist(pio_mutex->registers, &result,
					pio_handle, new_buf, mem_ptr,
					gcb->scratch, start_label);
	pio_mutex->active = 0;
	_OSDEP_MUTEX_UNLOCK(&pio_mutex->lock);

	_UDI_IMMED_CALLBACK(cb, _UDI_CT_PIO_TRANS,
			    &_udi_alloc_ops_nop,
			    callback, 3, (new_buf, status, result));

	/*
	 * If any additional work was enqueued for us while we were
	 * executing that translist, we now loop until those are done.
	 */
#if 0
	_OSDEP_SIMPLE_MUTEX_LOCK(&pio_mutex->lock);
	while (!UDI_QUEUE_EMPTY(&pio_mutex->pio_queue)) {

#define tr cb->cb_allocm._u.pio_trans_request
		udi_cb_t *gcb;
		void *qelem;
		_udi_callback_func_t *callback;

		qelem = UDI_DEQUEUE_HEAD(&pio_mutex->pio_queue);
		cb = UDI_BASE_STRUCT(qelem, _udi_cb_t,
				     cb_qlink);

		_OSDEP_SIMPLE_MUTEX_UNLOCK(&pio_mutex->lock);

		gcb = _UDI_HEADER_TO_CB(cb);
		new_buf = tr.buf;
		status = _udi_execute_translist(&result,
						tr.handle, new_buf, tr.mem_ptr,
						gcb->scratch, start_label);

#if 0
		callback = (_udi_callback_func_t *)tr.callback;
		cb->cb_flags |= _UDI_CB_CALLBACK;
		cb->op_idx = _UDI_CT_PIO_TRANS;
		cb->cb_allocm.alloc_ops = &_udi_alloc_ops_nop;
		SET_RESULT_UDI_CT_PIO_TRANS(&cb->cb_allocm,
					    new_buf, status, result);
		cb->cb_func = callback;
		cb->cb_allocm.alloc_state = _UDI_ALLOC_IMMED_DONE;
#endif

		_UDI_IMMED_CALLBACK(cb, _UDI_CT_PIO_TRANS,
				    &_udi_alloc_ops_nop,
				    tr.callback, 3, (new_buf, status, result));

		_OSDEP_SIMPLE_MUTEX_LOCK(&pio_mutex->lock);
	}
#endif


/*	_OSDEP_SIMPLE_MUTEX_UNLOCK(&pio_mutex->lock); */
}

void
udi_pio_abort_sequence(udi_pio_handle_t pio_handle,
		       udi_size_t scratch_requirement)
{
	_udi_pio_handle_t *pio = pio_handle;

	/*
	 * If the user is replacing an existing abort sequence, 
	 * release the old handle.
	 */
	if (!UDI_HANDLE_IS_NULL(pio_handle, udi_pio_handle_t))
		  udi_pio_unmap(pio->dev_node->pio_abort_handle);

	pio->dev_node->pio_abort_handle = pio;
	pio->dev_node->pio_abort_scratch_size = scratch_requirement;

}

/****

udi_status_t 
udi_pio_probe(
	udi_pio_probe_call_t *callback,
	udi_cb_t *gcb,
	udi_pio_handle_t pio_handle,
	void *mem_ptr,
	udi_ubit32_t pio_offset,
	udi_ubit8_t tran_size_idx,
	udi_ubit8_t direction)    
{
       This intentionally not implemented in this common file.   The 
       interface to the common code is too icky to represent in an osdep
       way.   Just provide that function in your osdep_pio.c file.
}
****/

udi_ubit32_t
udi_pio_atomic_sizes(udi_pio_handle_t pio_handle)
{
	/*
	 * TODO/FIXME: Eventually, we should tie this to the (proposed, but
	 * unblessed and unimplemented Bus/Environment Interface)  to get
	 * the correct atomicity in light of intervening bridge controllers.
	 * Until that time, we know that we're PCI-only so we know the answer
	 * always "seven".
	 */

	return 7;
}
#endif /* _UDI_PHYSIO_SUPPORTED */
