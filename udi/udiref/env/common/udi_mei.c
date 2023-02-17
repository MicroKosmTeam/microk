
/*
 * File: env/common/udi_mei.c
 *
 * UDI MEI Metalanguage-to-Environment Interface Routines
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
#include "udi_mgmt_MA.h"


/* Internal debugging to force (non-mgmt) cb's to be reallocated.  Must
 * be a C expression that evaluates to true or false.
 */
#if !defined (_OSDEP_FORCE_CB_REALLOC)
#  define _OSDEP_FORCE_CB_REALLOC 0
#endif


/*
 * Initialize the control block properties structures.
 */
_udi_mei_cb_template_t *
_udi_mei_cb_init(udi_index_t meta_idx,
		 udi_index_t cb_group,
		 udi_index_t cb_idx,	/* from driver */

		 udi_size_t scratch_requirement,	/* from driver */

		 udi_size_t inline_size,
		 udi_layout_t *inline_layout)
{
	_udi_mei_cb_template_t *cb_template;
	_udi_inline_info_t *cbinfop;
	udi_size_t size;

	_OSDEP_ASSERT(cb_idx != 0 || meta_idx == 0);
	_OSDEP_ASSERT(scratch_requirement <= UDI_MAX_SCRATCH);

	/*
	 * Initialize the data structures
	 */
	size = sizeof *cb_template;
	if (inline_size != 0)
		size += sizeof (_udi_inline_info_t);
	cb_template = _OSDEP_MEM_ALLOC(size, 0, UDI_WAITOK);
	cb_template->meta_idx = meta_idx;
	cb_template->cb_group = cb_group;
	cb_template->cb_idx = cb_idx;

	/*
	 * This may be grown in _udi_post_init() if we have to expand it
	 * for marshal area.
	 */
	cb_template->cb_scratch_size = scratch_requirement;

	/*
	 * Only create an inline info if inline_size is non-zero.
	 * If inline_size is zero, there will be no inline memory
	 * except for udi_cb_alloc_dynamic, which does its own thing),
	 * even if the CB has an inline pointer.
	 */
	if (inline_size != 0) {
		cbinfop = (void *)(cb_template + 1);
		cb_template->cb_inline_info = cbinfop;
		cbinfop->inline_size = inline_size;
		cbinfop->inline_layout = inline_layout;
	} else {
		UDIENV_CHECK(non_null_inline_layout_specified_with_zero_size,
			     inline_layout == NULL);
	}

	/*
	 * Let caller write into drivers's control block properties array
	 */
	return cb_template;
}

/*
 * Initialize the interface data structures.
 */
_udi_interface_t *
_udi_mei_ops_init(udi_index_t meta_idx,
		  const char *meta_name,
		  udi_index_t ops_num,
		  udi_index_t ops_idx,	/* from driver */

		  udi_ops_vector_t *ops,	/* from driver */

		  const udi_ubit8_t *op_flags,	/* op_flags from driver */

		  udi_size_t chan_context_size,
		  udi_index_t n_ops_templates,
		  udi_mei_init_t *meta_info)
{
	udi_mei_ops_vec_template_t *this_ops_template;
	_udi_interface_t *ifp;
	udi_size_t if_size;
	udi_index_t n_ops;
	udi_mei_op_template_t *fast_op_template_list;

	_OSDEP_ASSERT(ops_idx != 0 || meta_idx == 0);
	_OSDEP_ASSERT(n_ops_templates != 0);
	_OSDEP_ASSERT(ops_num > 0);
	_OSDEP_ASSERT(chan_context_size <= UDI_MIN_ALLOC_LIMIT);

	this_ops_template = &(meta_info->ops_vec_template_list[ops_num - 1]);

	if (!ops) {
		_udi_env_log_write(_UDI_TREVENT_ENVERR,
				   UDI_LOG_ERROR, 0,
				   UDI_STAT_NOT_UNDERSTOOD,
				   1009, meta_name, ops_num, ops_idx);
		return NULL;
	}

	/*
	 * Initialize the data structures.
	 * First, we see how many op templates we have.
	 * While we're roaming through it, we verify the user hasn't
	 * handed us a table containing any null function pointers.
	 */
	for (n_ops = 0; this_ops_template->op_template_list[n_ops].op_name;
	     n_ops++)
		if (!ops[n_ops]) {
			_udi_env_log_write(_UDI_TREVENT_ENVERR,
					   UDI_LOG_ERROR, 0,
					   UDI_STAT_NOT_UNDERSTOOD,
					   1010, meta_name, ops_num, ops_idx,
					   n_ops);
			return NULL;
		}

	fast_op_template_list =
		_OSDEP_MEM_ALLOC((1 + n_ops) *
				 (sizeof
				  (udi_mei_op_template_t)), 0, UDI_WAITOK);

	/*
	 * Allocate enough space for an interface structure plus
	 * an array of scratch sizes (udi_size_t) per op.
	 */
	if_size = sizeof *ifp + (1 + n_ops) * sizeof (*ifp->if_op_data);

	ifp = _OSDEP_MEM_ALLOC(if_size, 0, UDI_WAITOK);

	ifp->if_ops = ops;
	ifp->op_flags = op_flags;
	ifp->if_num_ops = n_ops;
	ifp->chan_context_size = chan_context_size;

	/*
	 * FIXME: should use _udi_MA_meta_find() 
	 */
	ifp->if_meta = _OSDEP_MEM_ALLOC(sizeof (*ifp->if_meta), 0, UDI_WAITOK);

	ifp->if_meta->meta_idx = meta_idx;
	ifp->if_meta->meta_name = meta_name;
	ifp->if_meta->meta_n_ops_templates = n_ops_templates;
	ifp->if_meta->meta_info = meta_info;

	ifp->if_ops_template = this_ops_template;

	/*
	 * Keep a pointer to the per-op array for easy access. 
	 */
	ifp->if_op_data = (void *)(ifp + 1);
	ifp->if_op_template_list = fast_op_template_list;

	/*
	 * Now we copy the op_template list into the freshly allocated
	 * ifp, taking advantage of the chance to insert the evend indicator
	 * template into slot zero for all non-mgmt (non-zero meta_idx) metas.
	 * This costs us a few dozen bytes of memory per meta but allows
	 * simpler indexing of these at runtime.
	 * Note the casts to void * to cast away the constness of these 
	 * decls which have the 'const' qualifier in them.
	 */
	if (meta_idx) {
		udi_memcpy((void *)&ifp->if_op_template_list[0],
			   &_udi_channel_event_ind_template,
			   sizeof (udi_mei_op_template_t));
		udi_memcpy((void *)&ifp->if_op_template_list[1],
			   ifp->if_ops_template->op_template_list,
			   n_ops * sizeof (udi_mei_op_template_t));
	} else {
		udi_memcpy((void *)ifp->if_op_template_list,
			   ifp->if_ops_template->op_template_list,
			   n_ops * sizeof (udi_mei_op_template_t));
	}

	/*
	 * Let caller write this interface in the per-driver interface array.
	 */
	return ifp;
}

STATIC void
_udi_op_abort_q_remove(_udi_cb_t *cb)
{

	_udi_region_t *region = cb->abort_chan->chan_region;
	_udi_cb_t *event_cb = _UDI_CB_TO_HEADER(&_udi_op_abort_cb->v.ce);
	udi_queue_t *first;
	_udi_cb_t *first_cb;

	_OSDEP_SIMPLE_MUTEX_LOCK(&_udi_op_abort_cb_mutex);
	first = UDI_FIRST_ELEMENT(&_udi_op_abort_cb->cb_queue);
	first_cb = UDI_BASE_STRUCT(first, _udi_cb_t,
				   abort_link);

	first_cb = first_cb->new_cb;
	if (cb == first_cb) {
		/*
		 * being first on the op_abort_cb queue means that the
		 * cb is being used, and since we are still in this region
		 * the cb is not in this region, so just yank if from the
		 * region queue 
		 */
		_OSDEP_SIMPLE_MUTEX_UNLOCK(&_udi_op_abort_cb_mutex);
		REGION_LOCK(region);
		_UDI_REGION_Q_REMOVE(region, &event_cb->cb_qlink);
		REGION_UNLOCK(region);
		/*
		 * move onto the next channel in need of op_abort_cb 
		 */
		udi_channel_event_complete(&_udi_op_abort_cb->v.ce, UDI_OK);
		return;
	}
	UDI_QUEUE_REMOVE(&first_cb->abort_link);
	_OSDEP_SIMPLE_MUTEX_UNLOCK(&_udi_op_abort_cb_mutex);
	cb->abort_chan = NULL;
}

void
udi_mei_call(udi_cb_t *gcb,
	     udi_mei_init_t *meta_info,
	     udi_index_t ops_num,
	     udi_index_t op_idx,
	     ...)
{
	va_list arg;
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb)->new_cb;
	udi_channel_t tgt_channel = gcb->channel;
	_udi_channel_t *channel = ((_udi_channel_t *)tgt_channel)->other_end;
	_udi_region_t *region = channel->chan_region;
	_udi_interface_t *ifp = channel->chan_interface;
	_udi_interface_op_data_t *opdatap;
	_udi_inline_info_t *iip;
	udi_mei_op_template_t *op_template;
	udi_op_t *op_func;
	udi_size_t op_scratch_size;
	udi_ubit16_t op_chain_offset;
	udi_cb_t *prev;
	udi_boolean_t do_defer;

	/*
	 * If this control block is currently being used for a 
	 * callback, we're doomed.   Someone probably mismanaged
	 * a cb pointer or didn't adequately mark it as idle when
	 * they released it.
	 */
	_OSDEP_ASSERT((cb->cb_flags & _UDI_CB_CALLBACK) == 0);

	UDIENV_CHECK(mei_call_attempted_with_bad_meta_idx_for_this_cb,
		     (meta_info == ifp->if_meta->meta_info));

	/*
	 * If target end is closed, blow away the cb.
	 * FIXME: (low priority) Loop over the layout of this cb, 
	 * deallocating any buffers or chained cb's that might be attached
	 * to it.  
	 * also do not blow away the cb if it is part of the MA's pool of
	 * cbs, instead, give it back to the MA via _udi_MA_return_cb()
	 */
	if (channel->status == _UDI_CHANNEL_CLOSED) {
		udi_cb_free(gcb);
		return;
	}

	op_template = &ifp->if_op_template_list[op_idx];
	opdatap = &ifp->if_op_data[op_idx];

	op_scratch_size = opdatap->if_op_data_scratch_size;
	op_chain_offset = opdatap->opdata_chain_offset;
	iip = opdatap->inline_info;

	_OSDEP_ASSERT(ifp->if_ops != NULL);
	op_func = ifp->if_ops[op_idx];

	/*
	 * state management for abortable ops 
	 */
	if (cb->cb_flags & _UDI_CB_ABORTABLE &&
	    ((cb->except_op_idx == op_idx &&
	      cb->except_op_num == ops_num) ||
	     (cb->complete_op_idx == op_idx &&
	      cb->complete_op_num == ops_num))) {

		/*
		 * channel op completes 
		 */
		if (cb->abort_chan != NULL) {
			_udi_op_abort_q_remove(cb);
		}
		cb->cb_flags &= ~_UDI_CB_ABORTABLE;
	}
	if (ifp->if_op_template_list[op_idx].op_flags & UDI_MEI_OP_ABORTABLE) {
		/*
		 * starting the channel op 
		 */
		cb->complete_op_idx =
			ifp->if_op_template_list[op_idx].completion_vec_idx;
		cb->complete_op_num =
			ifp->if_op_template_list[op_idx].completion_ops_num;
		cb->cb_flags |= _UDI_CB_ABORTABLE;
	}

	for (prev = NULL;;) {
		/*
		 * Fill in the control block's context pointer so the callee 
		 * can access members of it.
		 */
		gcb->context = channel->chan_context;

		/*
		 * Transfer ownership of the control block to the new region
		 * by changing its channel pointer to the other end of the
		 * target channel (in the target region).
		 */
		gcb->channel = (udi_channel_t)channel;

		/*
		 * The new region may have different scratch requirements for
		 * this control block. If the scratch requirement increases,
		 * reallocate the control block.
		 */
		if (_OSDEP_FORCE_CB_REALLOC ||
		    (op_scratch_size > _UDI_CB_TO_HEADER(gcb)->scratch_size)) {
			_udi_cb_realloc(&gcb, op_scratch_size, iip);
			if (!(_UDI_CB_TO_HEADER(gcb)->cb_flags &
			      _UDI_CB_ABORTABLE)) {
				_OSDEP_MEM_FREE(_UDI_CB_TO_HEADER(gcb)->
						old_cb);
				_UDI_CB_TO_HEADER(gcb)->old_cb = NULL;
			}


			if (prev != NULL) {
				*(udi_cb_t **)((char *)prev +
					       op_chain_offset) = gcb;
			} else
				cb = _UDI_CB_TO_HEADER(gcb);
		}

		/*
		 * If this operation supports chained control blocks,
		 * loop to handle all the CBs in the chain.
		 */
		if (op_chain_offset == 0)
			break;

		prev = gcb;
		gcb = *(udi_cb_t **)((char *)gcb + op_chain_offset);
		if (gcb == NULL) {
			gcb = _UDI_HEADER_TO_CB(cb);
			break;
		}
	}

	va_start(arg, op_idx);

	/*
	 * See if the target region is busy or we are otherwise unable 
	 * to issue the channel op at this time.  If so, we place this call
	 * on the region queue of the source region.
	 */
	REGION_LOCK(region);
	do_defer = _OSDEP_DEFER_MEI_CALL(region, ifp->op_flags[op_idx]);
	if (do_defer ||
	    _OSDEP_FASTQ_MEI_CALL(region, ifp->op_flags[op_idx])) {

		/*
		 * Preserve the operation index in this control block 
		 */
		cb->op_idx = op_idx;

		/*
		 * Marshal parameters into gcb->scratch 
		 */
		_udi_marshal_params(op_template->marshal_layout,
				    gcb->scratch, arg);

		/*
		 * Add this control block onto the region's queue
		 * of cb's that need serviced as soon as it can.
		 * Before the region goes non-busy, this queue will
		 * be drained. 
		 */
#if UDI_DEBUG
		region->reg_queued_calls++;
		if (region->reg_queued_calls > 0xfffffffd) {
			region->reg_queued_calls = region->reg_direct_calls =
				0;
		}
#endif

		_UDI_REGION_Q_APPEND(region, &cb->cb_qlink);

		if (do_defer) {
		/*
		 * Notify the OS that this region (which, if 
		 * currently non-busy) needs to be scheduled.
		 * REGION_SCHEDULE is responsible for unlocking 
		 * the region.
		 */
			if (!REGION_IS_ACTIVE(region) &&
			    !REGION_IN_SCHED_Q(region)) {
			_OSDEP_REGION_SCHEDULE(region);
		} else {
			REGION_UNLOCK(region);
		}
		} else {
			_OSDEP_FASTQ_SCHED(region);
			REGION_UNLOCK(region);
		}

		va_end(arg);
		return;
	}
	(region)->reg_active = TRUE;
	REGION_UNLOCK(region);

	_OSDEP_ASSERT(op_template && (op_template->direct_stub != NULL));
	_OSDEP_ASSERT(op_func != NULL);

	/*
	 * We are preparing to leave the current region and enter a
	 * new one.   If that region is an interrupt region, inhibit
	 * interrupts, do the deed, turn them back on.
	 */
	if (region->reg_is_interrupt) {
		_OSDEP_MASK_INTR();
		(*op_template->direct_stub) (op_func, gcb, arg);
		_OSDEP_UNMASK_INTR();
	} else {
		/*
		 * Call the direct stub for this channel operation.
		 * The arguments passed are the operation for this template
		 * (i.e. the corresponding interface op for the other end of 
		 * this channel), the control block, and a varying length list
		 * of arguments that are of type defined by this interface op.
		 */
		(*op_template->direct_stub) (op_func, gcb, arg);
	}

	va_end(arg);

#if UDI_DEBUG
	/*
	 * Rip up the scratch space for this CB and stomp on it. 
	 */
	udi_memset(gcb->scratch, 0x33,
		   ifp->if_op_data[op_idx].if_op_data_scratch_size);
#endif

	REGION_LOCK(region);
#if UDI_DEBUG
	region->reg_direct_calls++;
	if (region->reg_direct_calls > 0xfffffffd)
		region->reg_queued_calls = region->reg_direct_calls = 0;
	region->reg_num_exits++;
	region->reg_queue_depth_sum += region->reg_queue_depth;
	if (region->reg_queue_depth_sum > 0xfffffffd)
		region->reg_queue_depth_sum = region->reg_num_exits = 0;
#endif
	UDI_RUN_REGION_QUEUE(region);
	if (region->reg_self_destruct) {
		REGION_UNLOCK(region);
		_udi_do_region_destroy(region);
	} else {
		REGION_UNLOCK(region);
	}
}

void
udi_mei_driver_error(udi_cb_t *gcb,
		     udi_mei_init_t *meta_info,
		     udi_index_t meta_ops_num,
		     udi_index_t vec_idx)
{
	/*
	 * FIXME: should be more ambitious, but this is actually 
	 * legal within the letter of the specification.
	 */
	return;
}


/* 
 * Stubs and template for udi_channel_event_ind().
 * This channel operation is special because it's used as the first op
 * in every metalanguage (except mgmt).
 */

_UDI_MEI_STUBS(udi_channel_event_ind, udi_channel_event_cb_t,
	       0, (), (), (),
	       0, 0,
	       ((_udi_channel_t *)cb->gcb.channel)->chan_interface->if_meta->
	       meta_info)

	udi_mei_op_template_t _udi_channel_event_ind_template = {
		"udi_channel_event_ind", UDI_MEI_OPCAT_IND, 0, 0,
		0,			/* completion_ops_num */
		0,			/* completion_vec_index */
		0,			/* exception_ops_num */
		0,			/* exception_vec_idx */
		udi_channel_event_ind_direct,
		udi_channel_event_ind_backend,
		NULL, NULL
	};
