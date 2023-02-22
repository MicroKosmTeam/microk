
/*
 * File: env/common/udi_MA_util.c
 *
 * Utilities for UDI Management Agents.
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
#include <udi_mgmt_MA.h>
#include <udi_MA_util.h>

/* Function prototypes for MA ops entry points. */
STATIC udi_usage_res_op_t _udi_MA_usage_res;
STATIC udi_devmgmt_ack_op_t _udi_devmgmt_ack;
extern udi_enumerate_ack_op_t _udi_MA_enumerate_ack;
STATIC udi_final_cleanup_ack_op_t _udi_MA_final_cleanup_ack;
extern void _udi_unbind4(_udi_unbind_context_t *context);
extern void _udi_unbind5(_udi_unbind_context_t *context);


/* MA ops vector, interface handle. */

udi_MA_ops_t _udi_MA_ops = {
	_udi_MA_usage_res,
	_udi_MA_enumerate_ack,
	_udi_devmgmt_ack,
	_udi_MA_final_cleanup_ack
};

/* a cb pool and queue to service channel ops from the MA */
struct {
	_OSDEP_MUTEX_T lock;
	udi_queue_t cb_q;
	udi_queue_t op_q;
} _udi_MA_cb_pool;

STATIC void
_udi_MA_use_cb_op(udi_MA_cb_t *mcb,
		  _udi_MA_op_q_entry_t *op)
{

	udi_cb_t *mcb_gcb = UDI_GCB(&mcb->v.usg);
	udi_cb_t *op_gcb = UDI_GCB(&op->op_cb.usage);

	UDIENV_CHECK(Channel_Must_Not_Be_NULL, op_gcb->channel != NULL);
	mcb_gcb->channel = op_gcb->channel;
	mcb_gcb->initiator_context = op_gcb->initiator_context;
	if (op->scratch_size) {
		udi_memcpy(mcb_gcb->scratch, op_gcb->scratch,
			   op->scratch_size);
		_OSDEP_MEM_FREE(op_gcb->scratch);
	}
	switch (op->op_type) {
	case _UDI_MA_USAGE_OP:
		mcb->v.usg.trace_mask = op->op_cb.usage.trace_mask;
		mcb->v.usg.meta_idx = op->op_cb.usage.meta_idx;
		udi_usage_ind(&mcb->v.usg, op->op_param.usage.level);
		break;
	case _UDI_MA_CLEANUP_OP:
		udi_final_cleanup_req(&mcb->v.m);
		break;
	case _UDI_MA_CB_ALLOC_OP:
		udi_cb_alloc(op->op_param.cb_alloc.callback,
			     UDI_GCB(&mcb->v.m),
			     op->op_param.cb_alloc.cb_idx,
			     op->op_param.cb_alloc.default_channel);
		break;
	case _UDI_MA_ENUM_OP:
		udi_memcpy((udi_ubit8_t *)&mcb->v.en + sizeof (udi_cb_t),
			   (udi_ubit8_t *)&op->op_cb.mgmt +
			   sizeof (udi_cb_t),
			   sizeof (udi_enumerate_cb_t) - sizeof (udi_cb_t));

		udi_enumerate_req(&mcb->v.en, op->op_param.enu.enum_level);
		break;
	case _UDI_MA_EV_OP:
		mcb->v.ce.event = op->op_cb.chan_ev.event;
		mcb->v.ce.params = op->op_cb.chan_ev.params;
		udi_channel_event_ind(&mcb->v.ce);
		break;
	case _UDI_MA_MEM_ALLOC_OP:
		udi_mem_alloc(op->op_param.mem_alloc.callback,
			      UDI_GCB(&mcb->v.m),
			      op->op_param.mem_alloc.size,
			      op->op_param.mem_alloc.flags);
		break;
	case _UDI_MA_DEVMGMT_OP:
		udi_devmgmt_req(&mcb->v.m, op->op_param.mgmt.mgmt_op,
				op->op_param.mgmt.parent_ID);
		break;
	case _UDI_MA_CB_ALLOC_BATCH_OP:
		udi_cb_alloc(op->op_param.cb_alloc_batch.callback,
			     UDI_GCB(&mcb->v.m),
			     op->op_param.cb_alloc_batch.cb_idx,
			     op->op_param.cb_alloc_batch.default_channel);
		break;
	default:
		_OSDEP_ASSERT(0);
	}
	_OSDEP_MEM_FREE(op);
}

void
_udi_MA_send_op(_udi_MA_op_q_entry_t *op)
{

	udi_MA_cb_t *mcb;
	udi_queue_t *elem;


	_OSDEP_MUTEX_LOCK(&_udi_MA_cb_pool.lock);
	if (UDI_QUEUE_EMPTY(&_udi_MA_cb_pool.cb_q)) {
		UDI_ENQUEUE_TAIL(&_udi_MA_cb_pool.op_q, &op->link);
		_OSDEP_MUTEX_UNLOCK(&_udi_MA_cb_pool.lock);
	} else {
		elem = UDI_DEQUEUE_HEAD(&_udi_MA_cb_pool.cb_q);
		_OSDEP_MUTEX_UNLOCK(&_udi_MA_cb_pool.lock);
		mcb = UDI_BASE_STRUCT(elem, udi_MA_cb_t,
				      cb_queue);

		_udi_MA_use_cb_op(mcb, op);
	}
}

void
_udi_MA_return_cb(udi_MA_cb_t *mcb)
{

	_udi_MA_op_q_entry_t *op;
	udi_queue_t *elem;

	_OSDEP_MUTEX_LOCK(&_udi_MA_cb_pool.lock);
	if (UDI_QUEUE_EMPTY(&_udi_MA_cb_pool.op_q)) {
		UDI_ENQUEUE_TAIL(&_udi_MA_cb_pool.cb_q, &mcb->cb_queue);
		_OSDEP_MUTEX_UNLOCK(&_udi_MA_cb_pool.lock);
	} else {
		elem = UDI_DEQUEUE_HEAD(&_udi_MA_cb_pool.op_q);
		_OSDEP_MUTEX_UNLOCK(&_udi_MA_cb_pool.lock);
		op = UDI_BASE_STRUCT(elem, _udi_MA_op_q_entry_t,
				     link);

		_udi_MA_use_cb_op(mcb, op);
	}
}

void
_udi_MA_add_cb_to_pool(void)
{

	udi_MA_cb_t *cb = _udi_MA_cb_alloc();

	UDI_ENQUEUE_HEAD(&_udi_MA_cb_pool.cb_q, &cb->cb_queue);
}

STATIC void
_udi_MA_free_cb_pool(void)
{
	udi_queue_t *elem, *tmp;
	udi_MA_cb_t *cb;

	/*
	 * at this point we are in big trouble is the op_q is non empty
	 */
	_OSDEP_MUTEX_LOCK(&_udi_MA_cb_pool.lock);
	_OSDEP_ASSERT(UDI_QUEUE_EMPTY(&_udi_MA_cb_pool.op_q));
	UDI_QUEUE_FOREACH(&_udi_MA_cb_pool.cb_q, elem, tmp) {
		cb = UDI_BASE_STRUCT(elem, udi_MA_cb_t,
				     cb_queue);

		udi_cb_free(UDI_GCB(&cb->v.m));
	}
	_OSDEP_MUTEX_UNLOCK(&_udi_MA_cb_pool.lock);
	_OSDEP_MUTEX_DEINIT(&_udi_MA_cb_pool.lock);
}

void
_udi_MA_util_init(void)
{
	UDI_QUEUE_INIT(&_udi_MA_cb_pool.cb_q);
	UDI_QUEUE_INIT(&_udi_MA_cb_pool.op_q);
	_OSDEP_MUTEX_INIT(&_udi_MA_cb_pool.lock, "_udi_MA_cb_pool.lock");
}

void
_udi_MA_util_deinit(void)
{
	_udi_MA_free_cb_pool();
}

STATIC void
_udi_MA_usage_res(udi_usage_cb_t *cb)
{
	_udi_bind_context_t *context = UDI_GCB(cb)->initiator_context;

	(*context->next_step) (context);
	_udi_MA_return_cb(UDI_MCB(cb, udi_MA_cb_t));
}

STATIC void
_udi_MA_final_cleanup_ack(udi_mgmt_cb_t *cb)
{
	_udi_unbind_context_t *context = UDI_GCB(cb)->initiator_context;

	/*
	 * We need to return the cb first, otherwise we have a chance of
	 * executing udi_MA_deinit3, which destroy the cb pool underneath
	 * us.
	 */
	_udi_MA_return_cb(UDI_MCB(cb, udi_MA_cb_t));

	_udi_unbind4(context);
}

STATIC void
_udi_devmgmt_ack(udi_mgmt_cb_t *cb,
		 udi_ubit8_t flags,
		 udi_status_t status)
{

	_udi_devmgmt_context_t *context = UDI_GCB(cb)->initiator_context;

	context->flags = flags;
	context->status = status;
	(*context->callback) (context);
	_udi_MA_return_cb(UDI_MCB(cb, udi_MA_cb_t));
}

/* ------- */

void
_udi_devmgmt_req(_udi_dev_node_t *node,
		 udi_ubit8_t mgmt_op,
		 udi_ubit8_t parent_ID,
		 _udi_devmgmt_context_t *context)
{

	_udi_MA_op_q_entry_t *op = NULL;

	op = _OSDEP_MEM_ALLOC(sizeof (_udi_MA_op_q_entry_t), 0, UDI_WAITOK);
	op->op_cb.mgmt.gcb.channel = (udi_channel_t)node->node_mgmt_channel;
	op->op_cb.mgmt.gcb.initiator_context = context;
	op->op_type = _UDI_MA_DEVMGMT_OP;
	op->op_param.mgmt.mgmt_op = mgmt_op;
	op->op_param.mgmt.parent_ID = parent_ID;
	_udi_MA_send_op(op);
}

void
_udi_MA_final_cleanup(_udi_dev_node_t *node,
		      _udi_unbind_context_t *context)
{

	_udi_MA_op_q_entry_t *op = NULL;

	op = _OSDEP_MEM_ALLOC(sizeof (_udi_MA_op_q_entry_t), 0, UDI_WAITOK);
	op->op_cb.mgmt.gcb.channel = (udi_channel_t)node->node_mgmt_channel;
	op->op_cb.mgmt.gcb.initiator_context = (void *)context;
	op->op_type = _UDI_MA_CLEANUP_OP;
	_udi_MA_send_op(op);
}



/*
 * After an unenumeration by the MA, destroy a driver.    This destroys
 * a _driver_ and not a driver instance and should therefore probably only 
 * be called by the osdep code when preparing to unload a module.
 */

void
_udi_MA_destroy_driver(_udi_driver_t *driver)
{
	udi_queue_t *elem, *tmp;
	int i;

	for (i = 0; i < 256; i++) {
		_udi_interface_t *ifp = driver->drv_interface[i];
		_udi_mei_cb_template_t *cbtp = driver->drv_cbtemplate[i];

		/*
		 * Release any channel operations structures. 
		 */
		if (ifp) {
			if (ifp->if_meta) {
				_OSDEP_MEM_FREE(ifp->if_meta);
			}
			if (ifp->if_op_template_list) {
				_OSDEP_MEM_FREE((void *)
						ifp->if_op_template_list);
			}
			_OSDEP_MEM_FREE(ifp);
		}

		/*
		 * Release any control block templates. 
		 */
		if (cbtp) {
			_OSDEP_MEM_FREE(cbtp);
		}
	}

	/*
	 * Free all drv_meta_Q entries 
	 */
	UDI_QUEUE_FOREACH(&driver->drv_meta_Q, elem, tmp) {
		UDI_QUEUE_REMOVE(elem);
		_OSDEP_MEM_FREE(elem);
	}
	if (driver->drv_child_interfaces)
		_OSDEP_MEM_FREE(driver->drv_child_interfaces);
	if (driver->drv_parent_interfaces)
		_OSDEP_MEM_FREE(driver->drv_parent_interfaces);

	/*
	 * TODO: Free constraints, too. 
	 */
	/*
	 * should drv->node_list be freed here ?? 
	 */

	_udi_osdep_sprops_deinit(driver);
	/*
	 * Free all drv_rprops entries 
	 */
	UDI_QUEUE_FOREACH(&driver->drv_rprops, elem, tmp) {
		UDI_QUEUE_REMOVE(elem);
		_OSDEP_MEM_FREE(elem);
	}
	_OSDEP_EVENT_DEINIT(&driver->MA_event);
	_OSDEP_MEM_FREE(driver);
}

/* ------- */

/*
 * Put udi_channel_event_complete here since it ties into MA state.
 */
void
udi_channel_event_complete(udi_channel_event_cb_t *evcb,
			   udi_status_t status)
{
	udi_MA_cb_t *cb = UDI_MCB(UDI_GCB(evcb), udi_MA_cb_t);
	udi_queue_t *next;
	_udi_cb_t *next_cb;
	_udi_bind_context_t *context = UDI_GCB(evcb)->initiator_context;


	switch (evcb->event) {
	case UDI_CHANNEL_BOUND:
		*context->status = status;
		if (evcb->params.parent_bound.path_handles != NULL) {
			_udi_cb_t *_cb;

			_cb = _UDI_CB_TO_HEADER(UDI_GCB(evcb));
			_OSDEP_MEM_FREE(_cb->cb_inline_info);
			udi_cb_free(UDI_GCB(evcb));
			(*context->next_step) (context);
		} else {
			(*context->next_step) (context);
			_udi_MA_return_cb(UDI_MCB(evcb, udi_MA_cb_t));
		}
		break;
	case UDI_CHANNEL_CLOSED:
		_udi_MA_return_cb(UDI_MCB(evcb, udi_MA_cb_t));

		break;
	case UDI_CHANNEL_OP_ABORTED:
		_OSDEP_ASSERT(cb == _udi_op_abort_cb);

		/*
		 * If we have any other channel event inds queued on the
		 * global op_abort_cb, issue the next one now.
		 * the next will be issued when the previous completes
		 */
		_OSDEP_SIMPLE_MUTEX_LOCK(&_udi_op_abort_cb_mutex);
		next = UDI_DEQUEUE_HEAD(&_udi_op_abort_cb->cb_queue);
		next_cb = UDI_BASE_STRUCT(next, _udi_cb_t,
					  abort_link);

		next_cb = next_cb->new_cb;
		next_cb->abort_chan = NULL;
		next_cb->cb_flags &= ~_UDI_CB_ABORTABLE;
		if (!UDI_QUEUE_EMPTY(&_udi_op_abort_cb->cb_queue)) {
			next = UDI_FIRST_ELEMENT(&_udi_op_abort_cb->cb_queue);
			next_cb = UDI_BASE_STRUCT(next, _udi_cb_t,
						  abort_link);

			_OSDEP_SIMPLE_MUTEX_UNLOCK(&_udi_op_abort_cb_mutex);
			next_cb = next_cb->new_cb;
			UDI_GCB(evcb)->channel = next_cb->abort_chan;
			evcb->event = UDI_CHANNEL_OP_ABORTED;
			evcb->params.orig_cb = _UDI_HEADER_TO_CB(next_cb);
			udi_channel_event_ind(evcb);
		} else {
			_OSDEP_SIMPLE_MUTEX_UNLOCK(&_udi_op_abort_cb_mutex);
		}

		break;
	}

	/*
	 * NOTE: Don't deallocate the CB; the caller of udi_channel_event 
	 *       will do this when it wakes up from the signal.
	 */
}

#if _UDI_PHYSIO_SUPPORTED
STATIC void
_udi_pio_abort_cb(udi_cb_t *gcb,
		  udi_buf_t *new_buf,
		  udi_status_t status,
		  udi_ubit16_t result)
{

	_udi_unbind_context_t *context = gcb->initiator_context;

	udi_cb_free(gcb);
	udi_pio_unmap(context->node->pio_abort_handle);
	context->node->pio_abort_handle = UDI_NULL_PIO_HANDLE;
	_udi_unbind5(context);
}

void
_udi_MA_pio_trans(_udi_unbind_context_t *context,
		  _udi_channel_t *channel,
		  udi_pio_handle_t pio_handle,
		  udi_index_t start_label,
		  udi_buf_t *buf,
		  void *mem_ptr)
{

	_udi_cb_t orig_cb;
	_udi_mei_cb_template_t cb_template;
	udi_size_t scratch_size_delta;
	udi_cb_t *gcb;

	cb_template = *_udi_MA_driver->drv_cbtemplate[UDI_MA_MGMT_CB_IDX];

	scratch_size_delta = context->node->pio_abort_scratch_size -
		cb_template.cb_scratch_size;
	cb_template.cb_alloc_size += scratch_size_delta;
	cb_template.cb_scratch_size += scratch_size_delta;

	gcb = _udi_cb_alloc_waitok(&orig_cb, &cb_template,
				   (udi_channel_t)channel);
	gcb->initiator_context = (void *)context;
	udi_pio_trans(&_udi_pio_abort_cb, gcb, pio_handle, start_label,
		      buf, mem_ptr);
}
#endif /* _UDI_PHYSIO_SUPPORTED */

void
_udi_channel_close(udi_channel_t channel)
{

	_udi_MA_op_q_entry_t *op =
		_OSDEP_MEM_ALLOC(sizeof (_udi_MA_op_q_entry_t), 0, UDI_WAITOK);

	op->op_type = _UDI_MA_EV_OP;
	UDI_GCB(&op->op_cb.chan_ev)->channel = channel;
	op->op_cb.chan_ev.event = UDI_CHANNEL_CLOSED;
	_udi_MA_send_op(op);
}



/*
 * synchronize the devmgmt_req routine.  should only be used from blockable
 * osdep code.
 */
void
_udi_sync_devmgmt_req_done(_udi_devmgmt_context_t *context)
{

	_OSDEP_EVENT_T *event = (_OSDEP_EVENT_T *)context->unbind_context;

	_OSDEP_EVENT_SIGNAL(&(*event));
}

void
_udi_sync_devmgmt_req(_udi_dev_node_t *node,
		      _OSDEP_EVENT_T *event,
		      udi_status_t *status,
		      udi_ubit8_t *flags,
		      udi_ubit8_t mgmt_op,
		      udi_ubit8_t parent_ID)
{

	_udi_devmgmt_context_t *context =
		_OSDEP_MEM_ALLOC(sizeof (_udi_devmgmt_context_t), 0,

				 UDI_WAITOK);

	context->callback = _udi_sync_devmgmt_req_done;
	context->unbind_context = (_udi_unbind_context_t *)event;
	_udi_devmgmt_req(node, mgmt_op, parent_ID, context);
	_OSDEP_EVENT_WAIT(&(*event));
	*flags = context->flags;
	*status = context->status;
	_OSDEP_MEM_FREE(context);
}

void
_udi_operation_done(void *arg)
{

	_OSDEP_EVENT_T *ev = (_OSDEP_EVENT_T *)arg;

	_OSDEP_EVENT_SIGNAL(&(*ev));
}
