
/*
 * File: env/common/udi_cb.c
 *
 * UDI Control Block Routines
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
#include "udi_MA_util.h"

/*
 * _udi_alloc_cb_complete is the callback when we have successfully
 * allocated (potentially immediately) to hold a new control block.
 * It initializes the control block and issues a callback to the 
 * caller.
 */
STATIC void
_udi_alloc_cb_complete(_udi_alloc_marshal_t *allocm,
		       void *new_mem)
{
	_udi_cb_t *orig_cb;
	_udi_mei_cb_template_t *cb_template;
	_udi_cb_t *cb;
	udi_cb_t *gcb;
	_udi_inline_info_t *iip;

	/*
	 * Get to the beginning of the _udi_cb_t for the allocm 
	 */
	orig_cb = allocm->_u.cb_alloc_request.orig_cb;
	/*
	 * Obtain the properties info 
	 */
	cb_template =
		(_udi_mei_cb_template_t *)allocm->_u.cb_alloc_request.
		cb_template;
	cb = (_udi_cb_t *)new_mem;
	gcb = _UDI_HEADER_TO_CB(cb);

	/*
	 * Set the allocated area sizes 
	 */
	cb->cb_size = cb_template->cb_alloc_size;
	cb->scratch_size = cb_template->cb_scratch_size;
	cb->cb_allocm.alloc_state = _UDI_ALLOC_IDLE;
	cb->cb_flags = 0;
	cb->new_cb = cb;
	cb->abort_chan = NULL;
	cb->old_cb = NULL;
	cb->complete_op_idx = cb->complete_op_num = 0;
	cb->except_op_idx = cb->except_op_num = 0;

	cb->cb_inline_info = cb_template->cb_inline_info;

	/*
	 * Initialize the in-line pointer 
	 */
	iip = cb_template->cb_inline_info;
	if (iip && iip->inline_size != 0) {
		*(void **)((char *)gcb + iip->inline_ptr_offset) =
			((char *)cb + iip->inline_offset);
	}

	/*
	 * Set up udi_cb_t 
	 */
	gcb->context = ((udi_cb_t *)_UDI_HEADER_TO_CB(orig_cb))->context;
	/*
	 * scratch/marshal space 
	 */
	gcb->scratch = (void *)((char *)cb + cb->cb_size - cb->scratch_size);
	_OSDEP_ASSERT(_UDI_IS_NATURALLY_ALIGNED(gcb->scratch));
	gcb->channel = allocm->_u.cb_alloc_request.target_channel;
	gcb->origin = ((udi_cb_t *)_UDI_HEADER_TO_CB(orig_cb))->origin;

	SET_RESULT_UDI_CT_CB_ALLOC(allocm, gcb);
}

/*
 * Special version of udi_cb_alloc() used internally when it's
 * OK for us to block.
 */
udi_cb_t *
_udi_cb_alloc_waitok(_udi_cb_t *orig_cb,
		     _udi_mei_cb_template_t *cb_template,
		     udi_channel_t default_channel)
{

	_udi_alloc_marshal_t *allocm = &orig_cb->cb_allocm;
	void *new_mem;

	new_mem = _OSDEP_MEM_ALLOC(cb_template->cb_alloc_size, 0, UDI_WAITOK);

	allocm->_u.cb_alloc_request.orig_cb = orig_cb;
	allocm->_u.cb_alloc_request.cb_template = cb_template;
	allocm->_u.cb_alloc_request.target_channel = default_channel;
	_udi_alloc_cb_complete(allocm, new_mem);

	return allocm->_u.cb_alloc_result.new_cb;
}

/*
 * _udi_alloc_cb_discard
 */
STATIC void
_udi_alloc_cb_discard(_udi_alloc_marshal_t *allocm)
{
	_OSDEP_MEM_FREE(allocm->_u.cb_alloc_result.new_cb);
}

STATIC _udi_alloc_ops_t _udi_alloc_cb_ops = {
	_udi_alloc_cb_complete,
	_udi_alloc_cb_discard,
	_udi_alloc_no_incoming,
	_UDI_ALLOC_COMPLETE_WONT_BLOCK
};

/*
 * Allocates a new control block.
 */
void
udi_cb_alloc(udi_cb_alloc_call_t *callback,
	     udi_cb_t *gcb,
	     udi_index_t cb_idx,
	     udi_channel_t default_channel)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	_udi_mei_cb_template_t *cb_template;
	_udi_channel_t *chan = (_udi_channel_t *)default_channel;
	void *new_mem;

	/*
	 * Point to the appropriate control block properties
	 * which also contains the complete control block
	 * size in cb_alloc_size.
	 */
	cb_template =
		_UDI_GCB_TO_CHAN(gcb)->chan_region->
		reg_driver->drv_cbtemplate[cb_idx];
	_OSDEP_ASSERT(cb_template != NULL);

	/*
	 * CORE E20000418.1 says that if a NULL_CHANNEL is passed as
	 * the default channel argument that we should use the channel
	 * member of the incoming cb.
	 */
	if (chan == UDI_NULL_CHANNEL) {
		chan = default_channel = gcb->channel;
	}

	UDIENV_CHECK(_cb_alloc_attempted_with_mismatched_cb_idx_and_channel,
		     chan->chan_region->reg_driver->drv_cbtemplate[cb_idx]->
		     cb_idx == cb_idx);

	/*
	 * Allocate space for a new control block 
	 */

	/*
	 * Note that we are allocating one large block, containing the
	 * scratch/marshalling space.  If further on, a larger scratch
	 * is required, we will need to realloc the whole thing,
	 * including the _udi_cb_t structure.  Time will tell whether
	 * this way, or requiring 2 separate allocs here, is best.
	 */
	new_mem = _OSDEP_MEM_ALLOC(cb_template->cb_alloc_size, 0, UDI_NOWAIT);
	allocm->_u.cb_alloc_request.orig_cb = cb;
	allocm->_u.cb_alloc_request.cb_template = cb_template;
	allocm->_u.cb_alloc_request.cb_idx = cb_idx;
	allocm->_u.cb_alloc_request.target_channel = default_channel;

	if (new_mem != NULL) {
		/*
		 * We got the memory immediately! 
		 */
		_udi_alloc_cb_complete(allocm, new_mem);
		_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_CB_ALLOC,
				      &_udi_alloc_cb_ops,
				      callback, 1,
				      (allocm->_u.cb_alloc_result.new_cb));
		return;
	}

	/*
	 * We need to wait for the memory, so queue it up 
	 */
	_UDI_ALLOC_QUEUE(cb, _UDI_CT_CB_ALLOC, callback,
			 cb_template->cb_alloc_size, &_udi_alloc_cb_ops);
}

/*
 * Dynamic inline version of udi_cb_alloc.
 */
void
udi_cb_alloc_dynamic(udi_cb_alloc_call_t *callback,
		     udi_cb_t *gcb,
		     udi_index_t cb_idx,
		     udi_channel_t default_channel,
		     udi_size_t inline_size,
		     udi_layout_t *inline_layout)
{
	UDIENV_CHECK(not_yet_implemented, FALSE);
}

void
udi_cb_free(udi_cb_t *cb)
{
	_udi_cb_t *cb_head;

	/*
	 * The 1.0 specification says that udi_cb_free(NULL) is a NOP. 
	 */
	if (cb == NULL) {
		return;
	}

	cb_head = _UDI_CB_TO_HEADER(cb);

	UDIENV_CHECK(_alloc_not_pending_for_CB,
		     (cb_head->cb_flags & _UDI_CB_ALLOC_PENDING) == 0);

	if (cb_head->old_cb) {
		_OSDEP_ASSERT(cb_head->old_cb->old_cb == NULL);
		_OSDEP_MEM_FREE(cb_head->old_cb);
	}
	_OSDEP_MEM_FREE(cb_head);
}

/*
 * Reallocate a control block, preserving the data that we should.
 * This may be called on an MEI call when we the CB enters a region
 * with a larger scratch than our own.
 */
void
_udi_cb_realloc(udi_cb_t **gcbp,
		udi_size_t new_scratch_size,
		_udi_inline_info_t *iip)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(*gcbp);
	udi_size_t new_size;
	void *new_mem;
	_udi_cb_t *new_cb;

	DEBUG3(3052, 2, (cb->scratch_size, new_scratch_size));

	/*
	 * Compute new control block size. 
	 */
	new_size = cb->cb_size - cb->scratch_size + new_scratch_size;

	/*
	 * Allocate new control block memory. 
	 */
#if 0
	new_mem = _OSDEP_MEM_ALLOC(new_size, UDI_MEM_NOZERO, UDI_NOWAIT);
	/*
	 * TODO: Handle deferred case 
	 *
	 * There are some tricky things about deferring this. 
	 * In ordre to maintain channel ordering, we need to set a 
	 * flag on this interface to queue/defer  all subsequent
	 * channel ops until this allocation unblocks.   Upon the
	 * completion, we can then unblock this interface and deliver
	 * the remaining channel ops.
	 */
	_OSDEP_ASSERT(new_mem);
#else
	/*
	 * Until that is TODONE, we'll let the allocator block us if we 
	 * have to.  This is wrong becuase we may be called from a 
	 * non-blocking context (i.e. interrupt time, init time, etc.)
	 * but the reality is that given the rarity of a small allocation
	 * like this having to block during the small number of times we
	 * are calling ie (remember, a cb will only grow over time and 
	 * only upon the first entry into the end of a channel with th e
	 * larger cb requirement) we get away with it...
	 */
	new_mem = _OSDEP_MEM_ALLOC(new_size, UDI_MEM_NOZERO, UDI_WAITOK);
#endif

	/*
	 * Copy most of the original control block to the new memory.
	 * Scratch space and marshalling space don't need to be preserved
	 * at this point.
	 */
	_OSDEP_ASSERT(new_mem);
	new_cb = (_udi_cb_t *)new_mem;
	*gcbp = _UDI_HEADER_TO_CB(new_cb);
	udi_memcpy(new_cb, cb, cb->cb_size - cb->scratch_size);
	cb->new_cb = new_cb->new_cb = new_cb;
	/* Don't leak reallocated CBs. */
	if (cb->old_cb) {
		_OSDEP_MEM_FREE(cb->old_cb);
	}
	new_cb->old_cb = cb;

	/*
	 * Set the allocated area sizes 
	 */
	new_cb->cb_size = new_size;
	new_cb->scratch_size = new_scratch_size;
	_OSDEP_ASSERT(new_cb->cb_allocm.alloc_state == _UDI_ALLOC_IDLE);

	/*
	 * Initialize the in-line pointer 
	 */
	iip = cb->cb_inline_info;
	if (iip && iip->inline_size != 0)
		*(void **)((char *)*gcbp + iip->inline_ptr_offset) =
			((char *)new_cb + iip->inline_offset);

	/*
	 * Set up scratch/marshal space pointer. 
	 */
	(*gcbp)->scratch = (void *)
		((char *)new_cb + new_cb->cb_size - new_cb->scratch_size);
	_OSDEP_ASSERT(_UDI_IS_NATURALLY_ALIGNED((*gcbp)->scratch));
	/*
	 * free old cb when the channel op has completed, channel op abort
	 * depends on the old cb being around 
	 */

}

/*
 * Note on the ordering of these fields. there is a three way connection here. 
 * context - cb_alloc_batch_request - cb_alloc_request
 * cb_alloc_batch_request is a subset of context so that it is easier to
 * copy info from allocm to context (see _udi_batch_alloc2)
 * cb_alloc_batch_request is a superset of cb_alloc_request so that
 * we can sort-of udi_cb_alloc the first cb and still carry the info from
 * the argument list into (eventually) the context (see udi_cb_alloc_batch)
 */
typedef struct {
	udi_size_t _req_size;		/* overlaid on req_size */
	_udi_cb_t *orig_cb;
	_udi_mei_cb_template_t *cb_template;
	udi_index_t cb_idx;
	udi_channel_t target_channel;
	udi_cb_alloc_batch_call_t *callback;
	udi_index_t count;
	udi_boolean_t with_buf;
	udi_size_t buf_size;
	udi_buf_path_t path_handle;
	udi_cb_t *first_cb;
	udi_layout_t *layout_ptr;	/* current position in the cb layout */
	char *buf_base;			/* starting point to offset from for next buf */
	udi_size_t buf_offset;		/* buf pointer is this much from new_start */
	udi_boolean_t visible;		/* working on the visible part of the cb */
} _udi_cb_alloc_batch_context_t;

STATIC udi_cb_alloc_call_t _udi_cb_alloc_batch2;
STATIC udi_cb_alloc_call_t _udi_cb_alloc_batch2a;
STATIC udi_buf_write_call_t _udi_cb_alloc_batch4;
STATIC void _udi_cb_alloc_batch3(_udi_cb_alloc_batch_context_t *context,
				 udi_cb_t *gcb);
STATIC void _udi_cb_alloc_batch5(_udi_cb_alloc_batch_context_t *context,
				 udi_cb_t *gcb);


void
udi_cb_alloc_batch(udi_cb_alloc_batch_call_t *callback,
		   udi_cb_t *gcb,
		   udi_index_t cb_idx,
		   udi_index_t count,
		   udi_boolean_t with_buf,
		   udi_size_t buf_size,
		   udi_buf_path_t path_handle)
{

	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	_udi_mei_cb_template_t *cb_template;
	_udi_channel_t *chan = (_udi_channel_t *)gcb->channel;

	/*
	 * Point to the appropriate control block properties
	 * which also contains the complete control block
	 * size in cb_alloc_size.
	 */
	cb_template =
		_UDI_GCB_TO_CHAN(gcb)->chan_region->
		reg_driver->drv_cbtemplate[cb_idx];
	_OSDEP_ASSERT(cb_template != NULL);

	UDIENV_CHECK(_cb_alloc_attempted_with_mismatched_cb_idx_and_channel,
		     chan->chan_region->reg_driver->drv_cbtemplate[cb_idx]->
		     cb_idx == cb_idx);

	allocm->_u.cb_alloc_batch_request.orig_cb = cb;
	allocm->_u.cb_alloc_batch_request.cb_template = cb_template;
	allocm->_u.cb_alloc_batch_request.cb_idx = cb_idx;
	allocm->_u.cb_alloc_batch_request.target_channel = gcb->channel;
	allocm->_u.cb_alloc_batch_request.callback = callback;
	allocm->_u.cb_alloc_batch_request.count = count;
	allocm->_u.cb_alloc_batch_request.with_buf = with_buf;
	allocm->_u.cb_alloc_batch_request.buf_size = buf_size;
	allocm->_u.cb_alloc_batch_request.path_handle = path_handle;
	_UDI_ALLOC_QUEUE(cb, _UDI_CT_CB_ALLOC, _udi_cb_alloc_batch2,
			 cb_template->cb_alloc_size, &_udi_alloc_cb_ops);
}

void
_udi_cb_alloc_batch2(udi_cb_t *gcb,
		     udi_cb_t *new_cb)
{

	_udi_cb_alloc_batch_context_t *context;
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	struct cb_alloc_batch_request_t *dummy;

	context = _OSDEP_MEM_ALLOC(sizeof (_udi_cb_alloc_batch_context_t),
				   0, UDI_WAITOK);
	/*
	 * beware!!! keep the field order the same between
	 * context and allocm->_u.cb_alloc_batch_request
	 */
	dummy = (struct cb_alloc_batch_request_t *)context;
	*dummy = allocm->_u.cb_alloc_batch_request;

	new_cb->initiator_context = context;
	context->first_cb = new_cb;
	context->count--;
	_udi_cb_alloc_batch3(context, new_cb);
}


void
_udi_cb_alloc_batch2a(udi_cb_t *gcb,
		      udi_cb_t *new_cb)
{

	_udi_cb_alloc_batch_context_t *context = gcb->initiator_context;
	_udi_cb_t *_new_cb = _UDI_CB_TO_HEADER(new_cb);
	udi_size_t cb_offset = 0;
	udi_layout_t *end;

	new_cb->initiator_context = gcb->initiator_context;
	/*
	 * attach new cb to previous cb. either by visible part
	 * of cb, inline, or initiator_context
	 */
	if (_udi_get_layout_offset(context->cb_template->cb_visible_layout,
				   &end, &cb_offset, UDI_DL_CB)) {
		*(udi_cb_t **)((char *)gcb + sizeof (udi_cb_t) + cb_offset) =
			new_cb;
	} else if (_new_cb->cb_inline_info &&
		   _udi_get_layout_offset(_new_cb->cb_inline_info->
					  inline_layout, &end, &cb_offset,
					  UDI_DL_CB)) {
		*(udi_cb_t **)((char *)gcb +
			       _new_cb->cb_inline_info->inline_ptr_offset +
			       cb_offset) = new_cb;
	} else {
		gcb->initiator_context = new_cb;
	}
	context->count--;
	_udi_cb_alloc_batch3(context, new_cb);
}

void
_udi_cb_alloc_batch3(_udi_cb_alloc_batch_context_t *context,
		     udi_cb_t *gcb)
{

	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);

	/*
	 * do we need to allocate buffer(s) ?
	 */
	if (!context->with_buf) {
		_udi_cb_alloc_batch5(context, gcb);
		return;
	}
	UDIENV_CHECK(no_layout_to_search_for_UDI_DL_BUF,
		     context->cb_template->cb_visible_layout ||
		     (cb->cb_inline_info &&
		      cb->cb_inline_info->inline_layout));
	/*
	 * take care of udi_buf_t* in visible part of cb
	 */
	context->visible = TRUE;
	if (_udi_get_layout_offset(context->cb_template->cb_visible_layout,
				   &context->layout_ptr, &context->buf_offset,
				   UDI_DL_BUF)) {

		context->layout_ptr += 3;	/* not interested in those bytes */
		context->buf_base = (char *)gcb + sizeof (udi_cb_t);

		UDI_BUF_ALLOC(_udi_cb_alloc_batch4, gcb, NULL,
			      context->buf_size, context->path_handle);
		return;
	}
}

/*
 * loop thru all udi_buf_t * in visible part of cb then
 * move on to inline
 */
void
_udi_cb_alloc_batch4(udi_cb_t *gcb,
		     udi_buf_t *new_buf)
{

	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_cb_alloc_batch_context_t *context = gcb->initiator_context;
	udi_layout_t *end;

	/*
	 * assign the buffer pointer to point to new_buf
	 */
	*(udi_buf_t **)(context->buf_base + context->buf_offset) = new_buf;
	/*
	 * move the base forward
	 */
	context->buf_base += (context->buf_offset + sizeof (udi_buf_t *));

	/*
	 * more buf? search for next occurance of DL_BUF
	 */
	if (_udi_get_layout_offset(context->layout_ptr, &end,
				   &context->buf_offset, UDI_DL_BUF)) {

		context->layout_ptr = end + 3;
		UDI_BUF_ALLOC(_udi_cb_alloc_batch4, gcb, NULL,
			      context->buf_size, context->path_handle);
		return;
	}
	if (context->visible && cb->cb_inline_info &&
	    _udi_get_layout_offset(cb->cb_inline_info->inline_layout,
				   &context->layout_ptr, &context->buf_offset,
				   UDI_DL_BUF)) {

		context->buf_base =
			(char *)gcb + cb->cb_inline_info->inline_ptr_offset;
		context->visible = FALSE;
		context->layout_ptr = end + 3;
		UDI_BUF_ALLOC(_udi_cb_alloc_batch4, gcb, NULL,
			      context->buf_size, context->path_handle);
		return;
	}
	_udi_cb_alloc_batch5(context, gcb);

}


void
_udi_cb_alloc_batch5(_udi_cb_alloc_batch_context_t *context,
		     udi_cb_t *gcb)
{

	udi_cb_alloc_batch_call_t *callback;
	_udi_cb_t *orig_cb;
	udi_cb_t *first_cb;

	/*
	 * are more cbs needed?
	 */
	if (context->count) {
		udi_cb_alloc(_udi_cb_alloc_batch2a, gcb,
			     context->cb_idx, gcb->channel);
		return;
	}
	gcb->initiator_context = NULL;
	callback = context->callback;
	orig_cb = context->orig_cb;
	first_cb = context->first_cb;
	_OSDEP_MEM_FREE(context);
	(*callback) (_UDI_HEADER_TO_CB(orig_cb), first_cb);
}
